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
#include <atomic>
#include <memory>
#include <mutex>
#include <condition_variable>

namespace ftl
{

template<class T, class Alloc = std::allocator<T>>
class BlockingQueue
{
public:
    using Mutex = std::mutex;
    using Lock = std::unique_lock<std::mutex>;
    BlockingQueue( size_t nCap, const Alloc &alloc = Alloc() ) : m_alloc( alloc ), m_data( m_alloc.allocate( nCap + 1 ) ), m_bufsize( nCap + 1 )
    {
    }
    ~BlockingQueue()
    {
        if ( m_data )
        {
            clear();
            m_alloc.deallocate( m_data, m_bufsize );
            m_data = nullptr;
        }
    }

    // return false when enqueue fails due to stopped.
    bool push( const T &val )
    {
        if ( m_stopping )
            return false;
        Lock locked( m_mut );
        m_condFull.wait( locked, [&] { return !isFull() || m_stopping.load(); } );

        if ( !m_stopping && !isFull() )
        {
            pushBack( val );
        }
        else
        { // stopping
            popAll();
            m_condEmpty.notify_one();
            return false;
        }

        locked.unlock();
        m_condEmpty.notify_one();
        return true;
    }

    bool pop( T *val )
    {
        if ( m_stopping )
            return false;
        Lock locked( m_mut );
        m_condEmpty.wait( locked, [&] { return !isEmpty() || m_stopping.load(); } );

        if ( !m_stopping && !isEmpty() )
        {
            popFront( val );
        }
        else
        { // stopping
            popAll();
            m_condEmpty.notify_one();
            return false;
        }

        locked.unlock();
        m_condEmpty.notify_one();
        return true;
    }

    /// @brief signal stop. All elments in queue will be cleared on stop.
    void stop()
    {
        m_stopping = true;
        m_condFull.notify_all();
        m_condEmpty.notify_all();
    }

    /// @brief signal stop and wait for full stop.
    void clear()
    {
        m_stopping = true;
        m_condFull.notify_all();
        m_condEmpty.notify_all();

        {
            Lock locked( m_mut );
            popAll();
        }
    }

    bool empty()
    {
        Lock locked( m_mut );
        return isEmpty();
    }

    bool stopping() const
    {
        return m_stopping;
    }

    size_t size() const
    {
        return m_end > m_begin ? m_end - m_begin : ( m_end + m_bufsize - m_begin );
    }


protected:
    bool isFull() const
    {
        return m_begin == m_end + 1 || ( m_begin == 0 && m_end == m_bufsize - 1 ); // end - bufsize +1 = begin, when begin and end always increment.
    }
    bool isEmpty() const
    {
        return m_begin == m_end;
    }
    void pushBack( const T &val )
    {
        assert( !isFull() );
        new ( &m_data[m_end] ) T( val );
        if ( ++m_end == m_bufsize )
            m_end = 0;
    }
    void popFront( T *val )
    {
        assert( !isEmpty() );
        if ( val )
            new ( val ) T( std::move( m_data[m_begin] ) ); // or copy construct
        m_data[m_begin].~T();
        if ( ++m_begin == m_bufsize )
            m_begin = 0;
    }
    void popAll()
    {
        while ( !isEmpty() )
            popFront( nullptr );
    }

    Alloc m_alloc;
    T *m_data = nullptr;
    size_t m_bufsize = 0, m_begin = 0, m_end = 0;
    std::atomic<bool> m_stopping{false};
    Mutex m_mut;
    std::condition_variable m_condFull, m_condEmpty;
};
} // namespace ftl
