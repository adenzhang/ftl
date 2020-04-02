#pragma once
#include <thread>
#include <functional>
#include <atomic>
#include <list>
#include <unordered_set>
#include <mutex>

namespace ftl
{

/// Threads are dynamicly created by start_thread and closed by stop_thread
/// @note Thread object is not removable.
template<class Task, class TaskQ, class ThreadData = void *, class SharedData = void *>
class ThreadList
{
public:
    using this_type = ThreadList;
    using IdleTask = std::function<void( size_t )>;

    enum Status
    {
        INIT,
        WORKING,
        STOPPED,
        STOPPING,
    };

    struct ThreadInfo
    {
        ThreadData data;
        TaskQ taskQ;
        std::atomic<Status> status;
        IdleTask idleTask;
        std::thread thread;
    };


    using ThreadInfoList = std::list<ThreadInfo>;
    using ThreadIndex = typename ThreadInfoList::iterator;

    template<class Iter>
    struct hash_deref_address
    {
        size_t operator()( const Iter &i ) const
        {
            return std::hash<std::ptrdiff_t>()( &*i );
        }
    };

    struct NoopTask
    {
        void operator()( ThreadIndex ) const
        {
        }
    };

    using ThreadInfoSet = std::unordered_set<ThreadIndex>;

public:
    ThreadList() = default;

    ThreadIndex start_thread( ThreadData &&tdata = ThreadData{}, TaskQ &&taskQ = TaskQ{}, IdleTask &&idleTask = NoopTask{} )
    {
        std::scoped_lock<Mutex> lock( mAllThreadsMutex );
        mThreadInfo.emplace_back( ThreadInfo{std::move( tdata ), std::move( taskQ ), INIT, std::move( idleTask )} );
        auto threadInfoIter = --mThreadInfo.end();
        threadInfoIter->thread = std::move( std::thread( [this, me = this, threadInfoIter] {
            threadInfoIter->status = WORKING;
            mActiveThreads.fetch_add( 1 ); // if mActiveThreads != mThreadInfo.size() , thread is pending for work or close.
            auto &taskQ = threadInfoIter->taskQ;
            auto &status = threadInfoIter->status;
            while ( true )
            {
                for ( auto pTask = taskQ.top(); pTask; pTask = taskQ.top() )
                {
                    ( *pTask )( threadInfoIter );
                    taskQ.pop();
                }
                if ( status.load() == STOPPING )
                    break;
                mIdleTask( threadInfoIter );
            }
            mActiveThreads.fetch_sub( 1 );
        } ) );
        mAllThreadInfo.insert( threadInfoIter );
        return threadInfoIter;
    }

    // remove from list when thread is stopped.
    bool stop_thread( ThreadIndex threadIdx, bool bSync = false )
    {
        {
            std::scoped_lock<Mutex> lock( mAllThreadsMutex );
            if ( !mAllThreadInfo.count( threadIdx ) )
                return true;
        }
        threadIdx->status = STOPPING;
        if ( bSync )
        {
            std::scoped_lock<Mutex> lock( mAllThreadsMutex );
            threadIdx->thread.join();
            mAllThreadInfo.erase( threadIdx );
            return true;
        }
        return false;
    }
    bool thread_exists( ThreadIndex threadIdx ) const
    {
        std::scoped_lock<Mutex> lock( mAllThreadsMutex );
        return mAllThreadInfo.count( threadIdx );
    }

    // @return true if stopped.
    bool stop( bool bSync = false )
    {
        {
            std::scoped_lock<Mutex> lock( mAllThreadsMutex );
            if ( !mAllThreadInfo.size() )
                return true;
        }
        if ( bSync )
        {
            {
                std::scoped_lock<Mutex> lock( mAllThreadsMutex );
                for ( auto &info : mAllThreadInfo ) // signal all threads to stop
                    info.thread.status = STOPPING;
            }
            while ( true )
            {
                ThreadIndex threadIdx;
                {
                    std::scoped_lock<Mutex> lock( mAllThreadsMutex );
                    if ( mAllThreadInfo.empty() )
                        return true;
                    threadIdx = mAllThreadInfo.begin();
                }
                stop_thread( threadIdx, true );
            }
            return true;
        }
        return false;
    }

    // @brief Put task onto thread queue.
    bool put_task( ThreadIndex threadIdx, const Task &task )
    {
        return threadIdx->taskQ.push( task );
    }

    bool safe_put_task( ThreadIndex threadIdx, const Task &task )
    {
        std::scoped_lock<Mutex> lock( mAllThreadsMutex );
        if ( thread_exists( threadIdx ) )
            return threadIdx->taskQ.push( task );
        else
            return false;
    }

    // @brief Put task onto thread queue.
    template<class... Args>
    bool emplace_task( ThreadIndex threadIdx, Args &&... args )
    {
        return threadIdx->taskQ.emplace( std::forward<Args>( args )... );
    }
    // @brief Put task onto thread queue.
    template<class... Args>
    bool safe_emplace_task( ThreadIndex threadIdx, Args &&... args )
    {
        std::scoped_lock<Mutex> lock( mAllThreadsMutex );
        if ( thread_exists( threadIdx ) )
            return threadIdx->taskQ.emplace( std::forward<Args>( args )... );
        else
            false;
    }

    const std::thread &thread( ThreadIndex threadIdx ) const
    {
        return threadIdx->thread;
    }
    ThreadData &thread_data( ThreadIndex threadIdx )
    {
        return threadIdx->data;
    }

    ThreadData &shared_data()
    {
        return mSharedData;
    }

    size_t size() const
    {
        return mThreadInfo.size();
    }

    ThreadIndex begin()
    {
        std::scoped_lock<Mutex> lock( mAllThreadsMutex );
        return mAllThreadInfo.begin();
    }
    ThreadIndex end()
    {
        std::scoped_lock<Mutex> lock( mAllThreadsMutex );
        return mAllThreadInfo.end();
    }

protected:
    using Mutex = std::mutex;

    size_t mThreadCount{0};
    ThreadInfoList mThreadInfo;
    ThreadInfoSet mAllThreadInfo;
    Mutex mAllThreadsMutex;
    SharedData mSharedData;
    std::atomic<unsigned> mActiveThreads{0};
}; // namespace ftl
} // namespace ftl
