/*
 * This file is part of the ftl (Fast Template Library) distribution (https://github.com/adenzhang/ftl).
 * Copyright (c) 2020 Aden Zhang.
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

#include <ftl/spsc_queue.h>
#include <ftl/mpsc_bounded_queue.h>
#include <ftl/spmc_bounded_queue.h>
#include <ftl/mem_pool.h>

namespace ftl
{

/// \brief Chan sends objects from producer threads to consumer threads.
template<class T, class Queue = ftl::SPSCRingQueue<T>>
class Chan
{
protected:
    Queue m_queue;

public:
    constexpr static bool support_multiple_producer_threads = Queue::support_multiple_producer_threads,
                          support_multiple_consumer_threads = Queue::support_multiple_consumer_threads;

    /// \brief default construct but not inited. init() must be called before in use.
    Chan() = default;

    /// \brief init with queSize.
    Chan( std::size_t queSize ) : m_queue( queSize )
    {
    }
    bool init( std::size_t queSize )
    {
        return m_queue.init( queSize );
    }

    template<class... Args>
    bool send( Args &&... args )
    {
        return m_queue.emplace( std::forward<Args>( args )... );
    }

    /// \brief peek function exists only if it does NOT support_multiple_consumer_threads.
    template<bool Enable = !support_multiple_consumer_threads>
    std::enable_if_t<Enable, T *> peek()
    {
        return m_queue.top();
    }
    bool recv( T *pObj )
    {
        m_queue.pop( pObj );
    }

    bool empty() const
    {
        return m_queue.empty();
    }
};

/// \brief PooledChan FIFO queue with object pool. The Chan sends pointers to objects from producer threads to consumer threads.
template<class T, class FIFOQueue, class PoolT = ftl::ObjectPool<T *>>
struct PooledChan
{
    using Msg = T;
    using Pool = PoolT;

    struct Deleter
    {
        PooledChan &del;

        void operator()( T *p )
        {
            if ( p )
            {
                p->~T();
                del.Release( p );
            }
        }
    };
    using MsgPtr = std::unique_ptr<T, Deleter>;

    /// \brief default construct but not inited. init() must be called before in use.
    PooledChan() = default;

    /// @param queSize Buffer/Queue size.
    /// @param initialAllocates preallocated count of objects.
    PooledChan( size_t queSize, size_t initialAllocates ) : m_queue( queSize ), m_pool( initialAllocates )
    {
    }

    bool init( size_t queSize, size_t initialAllocates )
    {
        return m_queue.init( queSize ) && m_pool.init( initialAllocates );
    }

    /// \brief create an object to send.
    template<class... Args>
    T *create( Args &&... args )
    {
        return m_pool.create( std::forward<Args>( args )... );
    }

    template<class... Args>
    std::unique_ptr<T, Deleter> create_unique( Args &&... args )
    {
        return std::unique_ptr<T, Deleter>( m_pool.create( std::forward<Args>( args )... ), Deleter{*this} );
    }

    /// Buffers must be sent in the order of allocation.
    bool send( T *p )
    {
        return p ? m_queue.push( p ) : false;
    }

    bool send( std::unique_ptr<T, Deleter> p )
    {
        auto ret = p ? m_queue.push( p.get() ) : false;
        p.release();
        return ret;
    }

    /// \brief peek is only applicable only for single-consumer mode. recv() must be called after peek, in order to consume.
    template<bool IsSingleConsumer = !FIFOQueue::support_multiple_consumer_threads>
    std::enable_if_t<IsSingleConsumer, T *> peek()
    {
        return m_queue.top();
    }

    /// \brief recv an object that must be released.
    T *recv()
    {
        T *res;
        if ( m_queue.pop( res ) )
            return res;
        return nullptr;
    }

    /// \brief recv a unique_ptr that auto releases object to pool.
    std::unique_ptr<T, Deleter> recv_unique()
    {
        return std::unique_ptr<T, Deleter>( recv(), Deleter{*this} );
    }

    /// \param p previously obtained by calling recv or peek.
    void release( T *p )
    {
        if ( !p )
            return;
        p->~T();
        m_pool.deallocate( p );
    }

    /// \brief test whether queue is empty.
    bool empty() const
    {
        return m_queue.empty();
    }

    /// \brief non-thread-safe clear the queue and return all queued objects to pool.
    void clear_queue()
    {
        while ( auto p = recv() )
            release( p );
    }

    /// \brief non-thread-safe clear_queue and return all objects (in/out queue) to pool.
    void clear()
    {
        clear_queue();
        m_pool.clear();
    }

protected:
    Pool m_pool;
    FIFOQueue m_queue;
};

template<class T>
using SPMCPooledChan = PooledChan<T, ftl::SPMCBoundedQueue<T>>;

template<class T>
using MPSCPooledChan = PooledChan<T, ftl::MPSCBoundedQueue<T>>;

/// \brief Objects are sent in the same order of object creation.
template<class T>
struct SPSCOrderedChan
{
    using Pool = ftl::SPSCRingQueue<T>;

    Pool m_bufPool;
    T *pCurrSend = nullptr, *pCurrRecv = nullptr; // only used for check.

    SPSCOrderedChan() = default;
    SPSCOrderedChan( std::size_t nBufferSize ) : m_bufPool( nBufferSize )
    {
    }
    bool init( std::size_t nBufferSize )
    {
        return m_bufPool.init( nBufferSize );
    }

    //-- Prepare for Producing.
    // allocate new buffer for sending.
    template<class... Args>
    T *create( Args &&... args )
    {
        pCurrSend = m_bufPool.prepare_push();
        if ( pCurrSend )
            new ( pCurrSend ) T( std::forward<Args>( args )... );
        return pCurrSend;
    }

    //-- Produce.
    /// objects must be sent in the order of allocation.
    /// \param pObj is the object that fetched by create().
    bool send( T *pObj )
    {
        assert( pCurrSend && pObj == pCurrSend && pCurrSend == m_bufPool.prepare_push() );
        m_bufPool.commit_push();
        pCurrSend = nullptr;
        return true;
    }

    //-- Peek / Consume.
    /// \note that recv is same as peek. Release(T*) will consume and release objects.
    T *peek()
    {
        return pCurrRecv = m_bufPool.top();
    }
    T *recv()
    {
        return peek();
    }

    /// \param pObj must be obtained by calling PeekRecv.
    void release( T *pObj )
    {
        assert( pCurrRecv && pObj == pCurrRecv && pCurrRecv == m_bufPool.top() );
        pObj->~T();
        m_bufPool.pop();
        pCurrRecv = nullptr;
    }
};

} // namespace ftl
