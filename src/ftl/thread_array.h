#pragma once
#include <thread>
#include <functional>
#include <atomic>
#include <list>

namespace ftl
{

template<class Task>
struct ThreadTaskClient
{
    std::size_t threadIdx = 0;

    bool put_task( Task &&task ) const
    {
        return ( m_pPutTask )( m_ownerObj, threadIdx, std::move( task ) );
    }

    bool put_task( std::size_t idx, Task &&task ) const
    {
        return ( m_pPutTask )( m_ownerObj, idx, std::move( task ) );
    }

    std::size_t count_threads() const
    {
        return ( m_pCountThreads )( m_ownerObj );
    }

    void *m_ownerObj = nullptr;

    bool ( *m_pPutTask )( void *obj, std::size_t, Task && );
    std::size_t ( *m_pCountThreads )( void *obj );
};

/// Task : std::invocable<task, size_t threadIdx>
/// TaskQ, MPSCLockFreeQueue<T> concept =
///    - bool push(const T&); // return false if Q is full
///    - bool emplace(Args&&... args);
///    - T *top(); // get top element, return nullptr if empty()
///    - bool pop(T* pMem=nullptr);  // return false if empty. if pMem != nullptr, call  ::new (pMem) T(T&&) to construct object T. if
///    pMem==nullptr, only pop.
///    - size_t size() const;
///    - bool empty() const;
///---
/// IdleTask : call onidle(threadIdx, ThreadArray) after each event is processed.
template<class Task, class TaskQ, class ThreadData = void *, class SharedData = void *>
class ThreadArray
{
public:
    using task_type = Task;
    using task_queue_type = TaskQ;

    using ThreadIndex = size_t;
    using this_type = ThreadArray;
    using IdleTask = std::function<void( ThreadIndex )>;
    struct NoopTask
    {
        void operator()( ThreadIndex ) const
        {
        }
    };

    enum Status
    {
        INIT,
        WORKING,
        STOPPED,
        STOPPING,
    };

public:
    /// @brief if nThread == 0, thread will not be created.
    /// if nThread>0, call start(nThread)
    ThreadArray( size_t nThread = 0, size_t nTaskQCapacity = 0, IdleTask &&idleTask = NoopTask{} ) : mIdleTask( std::move( idleTask ) )
    {
        mStatus = INIT;
        if ( nThread )
            start( nThread, nTaskQCapacity, ThreadData{} );
    }

    bool start( size_t nThread, size_t nTaskQCapacity, const ThreadData &tdata = ThreadData{} )
    {
        if ( !nThread )
            return true;
        mThreadInfo.clear();
        mThreadInfo.reserve( nThread );
        mStatus = WORKING;
        for ( size_t i = 0; i < nThread; ++i )
        {
            auto &info = mThreadInfo.emplace_back( ThreadInfo{tdata} );
            if ( !info.taskQ.init( nTaskQCapacity ) )
                return false;
            info.thread = std::thread( [this, me = this, threadIdx = i] {
                mActiveThreads.fetch_add( 1 );
                auto &taskQ = mThreadInfo[threadIdx].taskQ;
                while ( true )
                {
                    for ( auto pTask = taskQ.top(); pTask; pTask = taskQ.top() )
                    {
                        ( *pTask )( threadIdx );
                        taskQ.pop();
                    }
                    if ( mStatus.load() == STOPPING )
                        break;
                    mIdleTask( threadIdx );
                }
                mActiveThreads.fetch_sub( 1 );
            } );
        }
        return true;
    }

    // @return true if stopped.
    bool stop( bool bSync = false )
    {
        auto status = mStatus.load();
        if ( status != WORKING && status != STOPPING )
            return true;
        mStatus = STOPPING;
        if ( bSync )
        {
            join();
            return true;
        }
        else
        {
            if ( mActiveThreads.load() == 0 )
                join();
            return false;
        }
    }

    bool thread_exists( ThreadIndex threadIdx ) const
    {
        return mThreadInfo.size() > threadIdx;
    }

    // @brief Put task onto thread queue.
    bool put_task( ThreadIndex threadIdx, Task &&task )
    {
        return mThreadInfo[threadIdx].taskQ.push( std::move( task ) );
    }

    // @brief Put task onto thread queue.
    template<class... Args>
    bool emplace_task( ThreadIndex threadIdx, Args &&... args )
    {
        return mThreadInfo[threadIdx].taskQ.emplace( std::forward<Args>( args )... );
    }

    const std::thread &thread( ThreadIndex threadIdx ) const
    {
        return mThreadInfo[threadIdx].thread;
    }
    ThreadData &thread_data( ThreadIndex threadIdx )
    {
        return mThreadInfo[threadIdx].data;
    }

    ThreadData &shared_data()
    {
        return mSharedData;
    }

    /// \brief thread count.
    size_t size() const
    {
        return mThreadInfo.size();
    }
    ThreadIndex begin()
    {
        return 0;
    }
    ThreadIndex end()
    {
        return mThreadInfo.size();
    }

    ThreadTaskClient<Task> get_task_client( std::size_t threadIdx ) const
    {
        return ThreadTaskClient<Task>{threadIdx, this, &this_type::PutTask, &this_type::size};
    }

protected:
    void join()
    {
        for ( auto &tinfo : mThreadInfo )
        {
            tinfo.thread.join();
        }
        mStatus = STOPPED;
    }

    struct ThreadInfo
    {
        ThreadData data;
        TaskQ taskQ;
        std::thread thread;
    };


    static bool PutTask( void *thisObj, std::size_t idx, Task &&task )
    {
        return reinterpret_cast<this_type *>( thisObj )->put_task( idx, std::move( task ) );
    }

    IdleTask mIdleTask;
    std::vector<ThreadInfo> mThreadInfo;
    SharedData mSharedData;
    std::atomic<unsigned> mActiveThreads{0};
    std::atomic<Status> mStatus{INIT};
};


} // namespace ftl
