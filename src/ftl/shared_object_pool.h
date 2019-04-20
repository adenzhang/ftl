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

#ifndef _SHAREDOBJECTPOOL_H_
#define _SHAREDOBJECTPOOL_H_

#include <ftl/gen_itertor.h>
#include <ftl/intrusive_list.h>
#include <ftl/circular_queue.h>

#include <memory>
#include <cassert>
#include <iostream>


namespace ftl
{


template<typename T>
class safe_enable_shared_from_this : public std::enable_shared_from_this<T>
{
public:
    using shared_pointer = ::std::shared_ptr<T>;
    template<typename... _Args>
    static ::std::shared_ptr<T> create_me( _Args &&... p_args )
    {
        return ::std::allocate_shared<T>( Alloc(), std::forward<_Args>( p_args )... );
    }

protected:
    struct Alloc : std::allocator<T>
    {
        template<typename _Up, typename... _Args>
        void construct( _Up *__p, _Args &&... __args )
        {
            ::new ( static_cast<void *>( __p ) ) _Up( std::forward<_Args>( __args )... );
        }
    };
    safe_enable_shared_from_this() = default;
    safe_enable_shared_from_this( const safe_enable_shared_from_this & ) = delete;
    safe_enable_shared_from_this &operator=( const safe_enable_shared_from_this & ) = delete;
};

template<typename ChildT, typename Object, typename ObjectAlloc = std::allocator<Object>, typename QueueT = ftl::intrusive_singly_list<Object>>
class shared_object_pool_base
{
public:
    typedef shared_object_pool_base this_type;
    // should be protected
    shared_object_pool_base( const ObjectAlloc &alloc = ObjectAlloc() ) : objectAlloc( alloc ), allocatedCount( 0 )
    {
    }

    static const size_t obj_size = sizeof( Object );

    friend ChildT;

protected:
    typedef QueueT ObjectQ;
    ObjectAlloc objectAlloc;
    ObjectQ freeQ;
    size_t allocatedCount = 0;
    size_t freeCount = 0; // for debug only
public:
    struct object_destroy;
    friend object_destroy;
    typedef std::shared_ptr<Object> shared_object_ptr;
    typedef std::unique_ptr<Object, object_destroy> unique_object_ptr;
    typedef std::shared_ptr<ChildT> Ptr;

    struct default_destroy
    {
        void operator()( Object *p )
        {
            if ( p )
                p->~Object();
        }
    };

    struct object_destroy
    {
        Ptr pPool; // ensure pool existence
        object_destroy( const Ptr &p = nullptr ) : pPool( p )
        {
        }
        object_destroy( const object_destroy &d ) : pPool( d.pPool )
        {
            //            std::cout << "return_object(return_object&)" << std::endl;
        }
        object_destroy( object_destroy &&d ) : pPool( std::move( d.pPool ) )
        {
        }
        object_destroy &operator=( const object_destroy &d )
        {
            pPool = d.pPool;
            return *this;
        }
        object_destroy &operator=( object_destroy &&d )
        {
            pPool = std::move( d.pPool );
            return *this;
        }

        void operator()( Object *p )
        {
            assert( p && pPool || !p );
            // return it to freeQ
            if ( p )
            { // todo lock
                ++pPool->freeCount;
                default_destroy destroy;
                destroy( p );
                pPool->freeQ.push( *p );
            }
            //            std::cout << "destroy Object: allocated:" << pPool->allocatedCount << ", free size:" << pPool->freeQ.size() << std::endl;
        }
    };

    template<bool bOneByOne = true>
    void destroy()
    {
        assert( allocatedCount == freeCount );
        //        std::cout << "~shared_object_pool_base, allocated size:" << allocatedCount << " == freeQ size:" << freeQ.size() << std::endl;
        if ( !freeQ.empty() )
        { // todo lock
            while ( !freeQ.empty() )
            {
                objectAlloc.deallocate( reinterpret_cast<Object *>( &freeQ.top() ), 1 );
                freeQ.pop();
            }
        }
    }

    template<typename... Args>
    shared_object_ptr create_shared( Args &&... args )
    {
        if ( auto p = static_cast<ChildT *>( this )->get_or_allocate() )
        {
            new ( p ) Object( std::forward<Args>( args )... );
            return {p, object_destroy( static_cast<ChildT *>( this )->me_ptr() )};
        }
        else
            return {};
    }

    template<typename... Args>
    unique_object_ptr create_unique( Args &&... args )
    {
        if ( auto p = static_cast<ChildT *>( this )->get_or_allocate() )
        {
            new ( p ) Object( std::forward<Args>( args )... );
            return {p, object_destroy( static_cast<ChildT *>( this )->me_ptr() )};
        }
        else
            return {nullptr, {}};
    }

    size_t size() const
    {
        return freeQ.size();
    }

    size_t allocate( size_t n = 1 )
    {
        for ( size_t i = 0; i < n; ++i )
        {
            if ( auto p = objectAlloc.allocate( 1 ) )
                freeQ.push( *p );
            else
                return i;
        }
        return n;
    }

    void clear()
    {
        while ( !freeQ.empty() )
        {
            objectAlloc.deallocate( &freeQ.top(), 1 );
            freeQ.pop();
        }
    }
};


template<typename ChildT,
         typename Object,
         size_t CapacityT,
         typename ObjectAlloc = std::allocator<Object>,
         typename QueueT = ftl::intrusive_singly_list<Object>>
class fixed_shared_pool_base : public shared_object_pool_base<ChildT, Object, ObjectAlloc, QueueT>
{
protected:
    Object *mObjects;

public:
    using this_type = fixed_shared_pool_base;
    using base_type = shared_object_pool_base<ChildT, Object, ObjectAlloc, QueueT>;

    static const size_t capacity = CapacityT;

    // should be protected
    fixed_shared_pool_base( const ObjectAlloc &alloc = ObjectAlloc() ) : base_type( alloc )
    {
        mObjects = base_type::objectAlloc.allocate( CapacityT, nullptr );
        for ( size_t i = 0; i < CapacityT; ++i )
            base_type::freeQ.push( *( mObjects + i ) );
    }

    ~fixed_shared_pool_base()
    {
        //        assert( base_type::freeQ.size() == CapacityT );
        //        std::cout << "~fixed_shared_pool_base, allocated size:" << base_type::allocatedCount << " == freeQ size:" <<
        //        base_type::freeQ.size()
        //                  << std::endl;
        if ( mObjects )
        { // todo lock
            base_type::objectAlloc.deallocate( mObjects, CapacityT );
            mObjects = nullptr;
        }
    }

    size_t size() const
    {
        return base_type::freeQ.size();
    }

    size_t allocate( size_t )
    {
        return 0;
    }
};

template<typename Object, typename ObjectAlloc = std::allocator<Object>>
class shared_object_pool : public shared_object_pool_base<shared_object_pool<Object, ObjectAlloc>, Object, ObjectAlloc>,
                           public safe_enable_shared_from_this<shared_object_pool<Object, ObjectAlloc>>
{
public:
    using this_type = shared_object_pool;
    using base_type = shared_object_pool_base<this_type, Object, ObjectAlloc>;

    friend struct safe_enable_shared_from_this<this_type>::Alloc;

    shared_object_pool( const ObjectAlloc &alloc = ObjectAlloc() ) : base_type( alloc )
    {
    }
    ~shared_object_pool()
    {
        static_cast<base_type *>( this )->destroy();
    }

    auto me_ptr()
    {
        return this->shared_from_this();
    }

    Object *get_or_allocate()
    {
        // todo lock
        Object *p = nullptr;
        if ( !base_type::freeQ.empty() )
        {
            p = &base_type::freeQ.top();
            base_type::freeQ.pop();
        }
        else
        {
            // if freeQ empty
            ++base_type::allocatedCount;
            p = base_type::objectAlloc.allocate( 1, nullptr );
        }
        return p;
    }
};

template<typename Object, size_t CapacityT, typename ObjectAlloc = std::allocator<Object>>
class fixed_shared_pool : public fixed_shared_pool_base<fixed_shared_pool<Object, CapacityT, ObjectAlloc>, Object, CapacityT, ObjectAlloc>,
                          public safe_enable_shared_from_this<fixed_shared_pool<Object, CapacityT, ObjectAlloc>>
{
public:
    using this_type = fixed_shared_pool;
    using base_type = fixed_shared_pool_base<this_type, Object, CapacityT, ObjectAlloc>;

    friend struct safe_enable_shared_from_this<this_type>::Alloc;

    fixed_shared_pool( const ObjectAlloc &alloc = ObjectAlloc() ) : base_type( alloc )
    {
    }

    auto me_ptr()
    {
        return this->shared_from_this();
    }

    Object *get_or_allocate()
    {
        // todo lock
        Object *p = nullptr;
        if ( !base_type::freeQ.empty() )
        {
            //            using OBJECTPTR = Object*;
            //            auto &r = base_type::freeQ.top(); //reinterpret_cast<Object*>(base_type::freeQ.top<Object*>());
            //            auto &rr = reinterpret_cast<OBJECTPTR&>(r);
            p = &base_type::freeQ.top();
            base_type::freeQ.pop();
        }
        return p;
    }
};
} // namespace ftl
#endif // _SHAREDOBJECTPOOL_H_
