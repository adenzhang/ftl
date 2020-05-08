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

namespace ftl
{

// todo: cache line optimization, alignment
template<class T, class AllocT = std::allocator<T>>
class SPMCBoundedQueue
{
    struct Node
    {
        std::atomic<bool> flag; // true for having value
        T val;
    };

public:
    constexpr static bool support_multiple_producer_threads = false, support_multiple_consumer_threads = true;
    using Alloc = typename std::allocator_traits<AllocT>::template rebind_alloc<Node>;

    SPMCBoundedQueue( size_t nCap = 0, const Alloc &alloc = Alloc() ) : m_alloc( alloc )
    {
        if ( nCap )
            init( nCap );
    }

    ~SPMCBoundedQueue()
    {
        if ( m_data )
        {
            clear();
            m_alloc.deallocate( m_data, m_bufsize );
            m_data = nullptr;
        }
    }

    bool init( size_t nCap )
    {
        m_data = m_alloc.allocate( nCap + 1 );
        m_bufsize = nCap + 1;
        for ( size_t i = 0; i < m_bufsize; ++i )
            m_data[i].flag.store( false );
        return m_data;
    }

    /// @return false when full. Note: the front element mayb be in process of dequeue.
    template<class... Args>
    bool emplate( Args &&... args )
    {
        auto iEnd = m_end.load( std::memory_order_acquire ) % m_bufsize;
        auto iNext = ( iEnd + 1 ) % m_bufsize;
        if ( m_data[iNext].flag.load( std::memory_order_acquire ) ) // full
            return false;
        assert( m_data[iEnd].flag.load( std::memory_order_acquire ) == false );
        new ( &m_data[iEnd].val ) T( std::forward<Args>( args )... );
        assert( !m_data[iEnd].flag );
        m_data[iEnd].flag.store( true, std::memory_order_release );
        m_end.fetch_add( 1 );
        return true;
    }

    bool push( const T &val )
    {
        return emplace( val );
    }

    bool pop( T *val )
    {
        size_t ibegin = m_begin.load( std::memory_order_acquire );
        do
        {
            if ( ibegin == m_end.load( std::memory_order_acquire ) ) // emtpy
                return false;
        } while ( !m_begin.compare_exchange_weak( ibegin, ibegin + 1 ) ); // std::memory_order_acquire

        // alreay acquired the ownership.
        ibegin %= m_bufsize;
        auto &data = m_data[ibegin];
        assert( data.flag.load( std::memory_order_acquire ) );

        if ( val )
            new ( val ) T( std::move( data.val ) );
        data.val.~T();
        data.flag.store( false, std::memory_order_release );
        return true;
    }

    size_t size() const
    {
        return m_end.load() - m_begin.load();
    }

    bool empty() const
    {
        return m_begin.load() == m_end.load();
    }

    /// weak full. dequeue may be in process.
    bool full() const
    {
        return m_begin.load() + ( m_bufsize - 1 ) == m_end.load();
    }

    void clear()
    {
        while ( pop( nullptr ) )
            ;
    }

protected:
    Alloc m_alloc;
    Node *m_data = nullptr;
    size_t m_bufsize = 0;
    std::atomic<size_t> m_begin{0}, m_end{0};
};

} // namespace ftl
