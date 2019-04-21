/*
 * This file is part of the ftl (Fast Template Library) distribution (https://github.com/adenzhang/ftl).
 * Copyright (c) 2018 Aden Zhang.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, version 3.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program. If not, see <http://www.gnu.org/licenses/>.
 */

#pragma once

#include <memory>
#include <mutex>
#include <atomic>
#include <cassert>
#include <thread>
#include <vector>
#include <ftl/spsc_queue.h>
#include <condition_variable>
#include <pthread.h>
#include <iostream>

namespace ftl
{
void SetThreadAffinity( std::thread &th, size_t i )
{
    cpu_set_t cpuset;
    CPU_ZERO( &cpuset );
    CPU_SET( i, &cpuset );
    int rc = pthread_setaffinity_np( th.native_handle(), sizeof( cpu_set_t ), &cpuset );
}

// Each thread has a ring queue
// for low-latency usage
template<class TaskT>
class ThreadArray
{
public:
    using Task = TaskT;
    using TaskQueue = ftl::SPSCRingQueue<Task>;
    using QueuePtr = std::unique_ptr<TaskQueue>;

protected:
    std::vector<QueuePtr> mQueues;
    std::vector<std::thread> mThreads; // per queue thread
    std::atomic<int> mActiveThreads{0};
    std::atomic<bool> mStop{false}; // 1 for stop
    std::atomic<size_t> mTaskCountInQueue{0}, mTasksProcessed{0};

public:
    bool Init( size_t nQueues, size_t nQueueSizePerThread )
    {
        mQueues.reserve( nQueues );
        std::allocator<Task> alloc;
        for ( size_t i = 0; i < nQueues; ++i )
        {
            if ( auto p = alloc.allocate( nQueueSizePerThread ) )
            {
                mQueues.push_back( QueuePtr( new TaskQueue( nQueueSizePerThread, p ) ) );
                mThreads.push_back( std::thread( [&, qid = i] {
                    auto &q = *mQueues[qid];
                    mActiveThreads.fetch_add( 1 );
                    for ( ;; )
                    {
                        Task task;
                        if ( !q.pop( &task ) ) // empty
                        {
                            if ( mStop.load() )
                                break;
                            std::this_thread::yield();
                        }
                        else
                        {
                            task(); // todo: catch exception
                            mTaskCountInQueue.fetch_sub( 1 );
                            mTasksProcessed.fetch_add( 1 );
                        }
                    }
                    mActiveThreads.fetch_sub( 1 );
                } ) );
                SetThreadAffinity( mThreads[i], i );
            }
            else
                return false;
        }
        return true;
    }
    bool Inited() const
    {
        return mActiveThreads.load() == CountThreads();
    }
    size_t CountThreads() const
    {
        return mThreads.size();
    }
    bool Put( TaskT &&task, size_t qid = 0 )
    {
        if ( mQueues[qid]->push( std::move( task ) ) )
        {
            mTaskCountInQueue.fetch_add( 1 );
            return true;
        }
        return false;
    }
    size_t CountTasksInQueue() const
    {
        return mTaskCountInQueue.load();
    }
    size_t CountTasksProcessed() const
    {
        return mTasksProcessed.load();
    }

    bool Stop( bool bWait = true )
    {
        mStop.store( true );
        if ( bWait )
        {
            for ( auto &t : mThreads )
            {
                t.join();
            }
            assert( 0 == mActiveThreads.load() );
            assert( 0 == CountTasksInQueue() );
        }
        return 0 == mActiveThreads.load();
    }
};

// one task queue shared by all threads
// for high throughput usage
template<class TaskT>
class ThreadPool
{
public:
    using Task = TaskT;
    using TaskQueue = ftl::SPSCRingQueue<Task>;
    using QueuePtr = std::unique_ptr<TaskQueue>;

protected:
    QueuePtr mQueue;
    std::mutex mMut;
    std::condition_variable mCondEmpty, mCondFull;
    std::vector<std::thread> mThreads; // per queue thread
    std::atomic<int> mActiveThreads{0};
    std::atomic<bool> mStop{false}; // 1 for stop
    std::atomic<int> mTasksProcessed{0};

public:
    bool Init( size_t nQueues, size_t nQueueSize )
    {
        std::allocator<Task> alloc;
        auto p = alloc.allocate( nQueueSize );
        if ( !p )
            return false;
        mQueue.reset( new TaskQueue( nQueueSize, p ) );
        for ( size_t i = 0; i < nQueues; ++i )
        {
            mThreads.push_back( std::thread( [&] {
                auto &q = *mQueue;
                mActiveThreads.fetch_add( 1 );
                bool bStopping = false;
                for ( ; !bStopping; )
                {
                    Task task;
                    {
                        std::unique_lock<std::mutex> lock( mMut );
                        while ( !q.pop( &task ) ) // empty
                        {
                            if ( mStop.load() ) // empty and stopping, then force stop
                            {
                                bStopping = true;
                                break;
                            }
                            mCondEmpty.wait( lock );
                        }
                    }
                    mCondFull.notify_one();

                    if ( !bStopping )
                    {
                        task(); // todo: catch exception
                        mTasksProcessed.fetch_add( 1 );
                    }
                }
                mActiveThreads.fetch_sub( 1 );
            } ) );
            SetThreadAffinity( mThreads[i], i );
        }
        return true;
    }
    bool Inited() const
    {
        return mActiveThreads.load() == CountThreads();
    }
    size_t CountThreads() const
    {
        return mThreads.size();
    }

    // block until non-full
    // return false if stopped.
    bool Put( const TaskT &task, size_t threadHint = 0 )
    {
        std::unique_lock<std::mutex> lock( mMut );
        while ( !mQueue->push( task ) ) // full
        {
            if ( mStop.load() )
                return false;
            mCondFull.wait( lock );
        }
        mCondEmpty.notify_one();
        return true;
    }

    size_t CountTasksInQueue() const
    {
        return mQueue->size();
    }
    size_t CountTasksProcessed() const
    {
        return mTasksProcessed.load();
    }
    bool Stop( bool bWait = true )
    {
        mStop.store( true );
        mCondFull.notify_all();
        mCondEmpty.notify_all();

        if ( bWait )
        {
            for ( auto &t : mThreads )
            {
                t.join();
            }
            assert( 0 == mActiveThreads.load() );
            assert( 0 == CountTasksInQueue() );
        }
        return 0 == mActiveThreads.load();
    }
};

/***************************************************
struct Task
{
    int id;
    void operator()()
    {
        ++id;
    }
};
template<class ThreadPoolT = ftl::ThreadArray<Task>>
void testPerf_ThreadArray( size_t nThreads = 10, size_t queueSize = 128, size_t nMsgs = 1024 )
{
    ThreadPoolT threads;
    using Task = typename ThreadPoolT::Task;
    threads.Init( nThreads, queueSize );

    auto tsStart = std::chrono::steady_clock::now();
    for ( size_t i = 0; i < nMsgs; ++i )
    {
        if ( !threads.Put( Task{size_t( i )}, i % nThreads ) )
        {
            std::cout << "Failded sending msg: " << i << std::endl;
        }
        //        std::this_thread::sleep_for( std::chrono::milliseconds( 500 ) );
    }
    threads.Stop();
    auto tsStart1 = std::chrono::steady_clock::now();
    assert( threads.CountTasksProcessed() == nMsgs );
    std::cout << "time elapsed:" << std::chrono::duration_cast<std::chrono::milliseconds>( tsStart1 - tsStart ).count() << "ms" << std::endl;
}
 */
} // namespace ftl
