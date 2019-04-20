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
#include <exception>
#include <ftl/intrusive_list.h>
#include <memory>

namespace ftl
{

// allocate/dealloate only top of stack
template<class T, size_t N>
struct inline_stack_allocator
{
    using value_type = T;
    using pointer_type = T *;

    static constexpr size_t BUFFERSIZE = sizeof( T ) * N;

    inline_stack_allocator()
    {
        mCurr = mBuf;
    }

    // only copy constuct empty object is allowed
    inline_stack_allocator( const inline_stack_allocator &a )
    {
        if ( a.mBuf == a.mCurr )
        {
            mCurr = mBuf;
        }
        else
        {
            throw std::bad_alloc();
        }
    }

    T *allocate( size_t n, void *hint = nullptr )
    {
        T *p = reinterpret_cast<T *>( mCurr );
        auto newP = p + n;
        if ( newP <= reinterpret_cast<T *>( mBuf + BUFFERSIZE ) )
        {
            mCurr = reinterpret_cast<char *>( newP );
            return p;
        }
        else
            return nullptr;
    }
    void deallocate( T *p, size_t n )
    {
        assert( p + n == reinterpret_cast<T *>( mCurr ) );
        mCurr = reinterpret_cast<char *>( p );
    }
    size_t free_size() const
    {
        return reinterpret_cast<T *>( mBuf + BUFFERSIZE ) - mCurr;
    }

    constexpr size_t capacity() const
    {
        return N;
    }
    char mBuf[BUFFERSIZE];
    char *mCurr = nullptr; // points to free memory
};

template<typename T, typename ObjectAlloc = std::allocator<T>>
class circular_queue
{
protected:
    size_t mCapacity = 0;
    ObjectAlloc mAlloc;
    T *mBuf = nullptr;
    T *pFront = nullptr; // points to first available
    T *pEnd = nullptr; // points to free memory block
    size_t mSize = 0;

public:
    using this_type = circular_queue;

    //- iterator support

    using value_type = T;
    using node_type = T *;

    using iterator = gen_iterator<this_type, false>;
    using const_iterator = gen_iterator<this_type, true>;

    bool is_end( const node_type &n ) const
    {
        return !n;
    }
    bool next( node_type &n ) const
    {
        if ( !n )
            return false;
        n = inc( n );
        if ( n == pEnd || n == pFront ) // the end
            n = node_type();
        return true;
    }
    bool prev( node_type &n ) const
    {
        if ( !n )
            return false;
        n = dec( n );
        if ( n == pEnd || n == pFront ) // the end
            n = node_type();
        return true;
    }
    value_type *get_value( const node_type &n ) const
    {
        return n;
    }

    //--
    iterator begin()
    {
        if ( empty() )
            return iterator();
        return iterator( pFront, this );
    }
    iterator end()
    {
        return iterator();
    }

    const_iterator begin() const
    {
        return cbegin();
    }
    const_iterator end() const
    {
        return const_iterator();
    }
    const_iterator cbegin() const
    {
        if ( empty() )
            return const_iterator();
        return const_iterator( pFront, this );
    }
    const_iterator cend() const
    {
        return const_iterator();
    }

    T &operator[]( size_t i )
    {
        assert( i < mSize );
        return *inc( pFront, i );
    }
    const T &operator[]( size_t i ) const
    {
        assert( i < mSize );
        return *inc( pFront, i );
    }

public:
    circular_queue( size_t cap = 0, const ObjectAlloc &alloc = ObjectAlloc() )
        : mCapacity( cap ),
          mAlloc( alloc ),
          mBuf( mCapacity ? mAlloc.allocate( mCapacity, nullptr ) : nullptr ),
          pFront( mBuf ),
          pEnd( mBuf ),
          mSize( 0 )
    {
        if ( mCapacity > 0 && !mBuf )
            throw std::bad_alloc();
    }
    circular_queue( circular_queue &&a ) : mCapacity( a.mCapacity ), mAlloc( std::move( a.mAlloc ) )
    {
        mBuf = a.mBuf;
        pFront = a.pFront;
        pEnd = a.pEnd;
        mSize = a.mSize;

        a.mBuf = nullptr;
        a.pFront = nullptr;
        a.pEnd = nullptr;
        a.mSize = 0;
    }
    // exactly clone
    circular_queue( const circular_queue &a )
        : mCapacity( a.mCapacity ), mAlloc( a.mAlloc ), mBuf( mCapacity ? nullptr : mAlloc.allocate( mCapacity, nullptr ) )
    {
        pFront = mBuf + ( a.pFront - a.mBuf );
        pEnd = mBuf + ( a.pEnd - a.mBuf );
        mSize = a.mSize;
        for ( auto p = pFront, q = a.pFront; q != a.pEnd; ( p = inc( p ) ), ( q = q.inc( q ) ) )
        {
            new ( p ) T( *q );
        }
    }
    // copy contents, with mSize capacity
    circular_queue &operator=( const circular_queue &a )
    {
        clear();
        reserve( a.size() );
        mSize = a.size();
        pFront = mBuf;
        pEnd = mBuf + mSize;
        for ( auto p = pFront, q = a.pFront; q != a.pEnd; ( p = inc( p ) ), ( q = a.inc( q ) ) )
        {
            new ( p ) T( *q );
        }
        return *this;
    }

    circular_queue &operator=( circular_queue &&a )
    {
        ~circular_queue();
        //        mAlloc = std::move(a.mAlloc);
        mBuf = a.mBuf;
        pFront = a.pFront;
        pEnd = a.pEnd;
        mSize = a.mSize;

        a.mBuf = nullptr;
        a.pFront = nullptr;
        a.pEnd = nullptr;
        a.mSize = 0;
    }

    ~circular_queue()
    {
        clear();
        if ( mBuf )
            mAlloc.deallocate( mBuf, mCapacity );
        mBuf = nullptr;
    }

    bool reserve( size_t cap )
    {
        if ( cap <= mCapacity )
            return true;
        auto size = mSize;
        auto newBuf = mAlloc.allocate( mCapacity, nullptr );
        if ( !newBuf )
            return false;

        for ( auto p = newBuf, q = pFront; q != pEnd; ++p, ( q = inc( q ) ) )
        {
            new ( p ) T( *q );
        }
        ~circular_queue();
        mCapacity = cap;
        mBuf = newBuf;
        pFront = mBuf;
        pEnd = pFront + size;
        mSize = size;
        return true;
    }

    template<class... Args>
    T &emplace_back( Args &&... args )
    {
        if ( full() )
        {
            throw std::bad_alloc();
        }
        T *p = new ( static_cast<void *>( pEnd ) ) T( std::forward<Args>( args )... );

        ++mSize;
        pEnd = inc( pEnd );
        return *p;
    }
    template<class... Args>
    T &emplace_front( Args &&... args )
    {
        if ( full() )
        {
            throw std::bad_alloc();
        }
        pFront = dec( pFront );
        T *p = new ( static_cast<void *>( pFront ) ) T( std::forward<Args>( args )... );

        ++mSize;
        return *p;
    }
    bool push_back( const T &v )
    {
        if ( full() )
            return false;
        emplace_back( v );
        return true;
    }
    bool push_front( const T &v )
    {
        if ( full() )
            return false;
        emplace_front( v );
        return true;
    }

    template<typename Iterator>
    size_t insert( Iterator it, Iterator end )
    {
        size_t i = 0;
        for ( ; it != end && push_back( *it ); ++i, ++it )
            ;
        return i;
    }

    bool pop_front()
    {
        if ( empty() )
            return false;
        reinterpret_cast<T *>( pFront )->~T();

        --mSize;
        pFront = inc( pFront );
        return true;
    }

    bool pop_back()
    {
        if ( empty() )
            return false;
        pEnd = dec( pEnd );
        reinterpret_cast<T *>( pEnd )->~T();

        --mSize;
        return true;
    }
    T &front()
    {
        return *reinterpret_cast<T *>( pFront );
    }
    T &back()
    {
        return *reinterpret_cast<T *>( dec( pEnd ) );
    }
    bool empty() const
    {
        return mSize == 0;
    }

    bool full() const
    {
        return mSize == mCapacity;
    }
    void clear()
    {
        while ( !empty() )
            pop_front();
    }
    size_t size() const
    {
        return mSize;
    }

public:
    node_type inc( node_type p, size_t step = 1 ) const
    {
        p += step;
        if ( p >= mBuf + mCapacity )
            p -= mCapacity;
        return p;
    }
    node_type dec( node_type p, size_t step = 1 ) const
    {
        p -= step;
        if ( p < mBuf )
            p += mCapacity;
        return p;
    }
    T *allocate()
    {
        assert( !mBuf );
        mBuf = mAlloc.allocate( mCapacity, nullptr );
        pFront = mBuf;
        pEnd = mBuf;
        return reinterpret_cast<T *>( mBuf );
    }
};

template<class T, size_t N>
struct inline_circular_queue : public circular_queue<T, inline_stack_allocator<T, N>>
{
    using this_type = inline_circular_queue;
    using base_type = circular_queue<T, inline_stack_allocator<T, N>>;

    using base_type::back;
    using base_type::begin;
    using base_type::cbegin;
    using base_type::cend;
    using base_type::clear;
    using base_type::emplace_back;
    using base_type::emplace_front;
    using base_type::empty;
    using base_type::end;
    using base_type::front;
    using base_type::full;
    using base_type::insert;
    using base_type::push_back;
    using base_type::push_front;
    using base_type::reserve;
    using base_type::size;
    using base_type::operator[];

    // iterator
    using base_type::const_iterator;
    using base_type::is_end;
    using base_type::iterator;
    using base_type::next;
    using base_type::node_type;
    using base_type::value_type;

    inline_circular_queue( const inline_circular_queue & ) = delete;
    inline_circular_queue &operator=( inline_circular_queue & ) = delete;

    inline_circular_queue() : base_type( N )
    {
    }
    bool reserve( size_t n )
    {
        return n <= N;
    }
};
} // namespace ftl
