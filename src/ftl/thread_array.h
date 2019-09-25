#pragma once
#include <thread>
#include <functional>
#include <atomic>

namespace ftl
{
/// Task : std::invocable<task, size_t threadIdx, ThreadArray&>
/// TaskQ, MPSCLockFreeQueue<T> concept =
///    - bool push(const T&); // return false if Q is full
///    - bool emplace(Args&&... args);
///    - T *top(); // get top element, return nullptr if empty()
///    - bool pop(T* pMem=nullptr);  // return false if empty. if pMem != nullptr, call  ::new (pMem) T(T&&) to construct object T. if pMem==nullptr,
///    only pop.
///    - size_t size() const;
///    - bool empty() const;
///---
/// IdleTask : call onidle(threadIdx, ThreadArray) after each event is processed.
template<class Task, class TaskQ, class ThreadData = void *, class SharedData = void *>
class ThreadArray
{
public:
    using this_type = ThreadArray;
    using IdleTask = std::function<void( size_t )>;
    struct NoopTask
    {
        void operator()( size_t ) const
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
    ThreadArray( size_t nThread = 0, const TaskQ &taskQ = TaskQ{}, IdleTask &&idleTask = NoopTask{} ) : mIdleTask( std::move( idleTask ) )
    {
        mStatus = INIT;
        if ( nThread )
            start( nThread, ThreadData{}, taskQ );
    }

    void start( size_t nThread, const ThreadData &tdata = ThreadData{}, const TaskQ &taskQ = TaskQ{} )
    {
        mThreadInfo.clear();
        mThreadInfo.reserve( nThread );
        mStatus = WORKING;
        for ( size_t i = 0; i < nThread; ++i )
        {
            mThreadInfo.emplace_back( ThreadInfo{tdata, taskQ, std::thread( [this, me = this, threadIdx = i] {
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
                                                 } )} );
        }
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

    // @brief Put task onto thread queue.
    bool put_task( size_t threadIdx, const Task &task )
    {
        return mThreadInfo[threadIdx].taskQ.push( task );
    }

    // @brief Put task onto thread queue.
    template<class... Args>
    bool emplace_task( size_t threadIdx, Args &&... args )
    {
        return mThreadInfo[threadIdx].taskQ.emplace( std::forward<Args>( args )... );
    }

    const std::thread &thread( size_t threadIdx ) const
    {
        return mThreadInfo[threadIdx].thread;
    }
    ThreadData &thread_data( size_t threadIdx ) const
    {
        return mThreadInfo[threadIdx].data;
    }

    ThreadData &shared_data()
    {
        return mSharedData;
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

    IdleTask mIdleTask;
    std::vector<ThreadInfo> mThreadInfo;
    SharedData mSharedData;
    std::atomic<unsigned> mActiveThreads{0};
    std::atomic<Status> mStatus{INIT};
};
} // namespace ftl
