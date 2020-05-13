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
#include <utility>
#include <cstdint>
#include <cstdlib>
#include <cassert>
#include <stdexcept>

namespace ftl
{

////////////////////////////////////////////////////////////////////////
///
///     Object Storage
///
////////////////////////////////////////////////////////////////////////

template<std::size_t N, std::size_t Align>
struct InplaceStorage
{
    static constexpr auto inplace_size = N;
    void *ptr() const
    {
        return &m_data;
    }

private:
    std::aligned_storage_t<N, Align> m_data;
};



template<std::size_t Align>
struct InplaceStorage<0, Align>
{
    static constexpr std::size_t inplace_size = 0;
    constexpr void *ptr() const
    {
        return nullptr;
    }
};

template<bool bUseDynamicAlloc>
struct AlignedDynamicStorage
{
    void *allocate( std::size_t N, std::size_t Align )
    {
        assert( !m_ptr );
        return m_ptr = std::aligned_alloc( Align, N );
    }

    void deallocate()
    {
        if ( m_ptr )
        {
            std::free( m_ptr );
            m_ptr = nullptr;
        }
    }
    void *ptr() const
    {
        return m_ptr;
    }
    void set_ptr( void *p )
    {
        assert( !m_ptr );
        m_ptr = p;
    }

protected:
    void *m_ptr = nullptr;
};

template<>
struct AlignedDynamicStorage<false>
{
    void *allocate( std::size_t, std::size_t )
    {
        return nullptr;
    }
    void deallocate()
    {
    }
    void *ptr() const
    {
        return nullptr;
    }
};

// allocate only once
template<std::size_t InplaceSize, std::size_t Align, bool bUseDynamicAlloc>
struct BoxedStorageBase : private InplaceStorage<InplaceSize, Align>, private AlignedDynamicStorage<bUseDynamicAlloc>
{
    using InplaceAlloc = InplaceStorage<InplaceSize, Align>;
    using DynamicAlloc = AlignedDynamicStorage<bUseDynamicAlloc>;
    static constexpr auto use_dynamic_alloc = bUseDynamicAlloc;

    static_assert( InplaceSize > 0 || bUseDynamicAlloc, "Unable to store!" );

    BoxedStorageBase( const BoxedStorageBase & ) = delete;
    BoxedStorageBase &operator=( const BoxedStorageBase & ) = delete;

    void *allocate( std::size_t M )
    {
        if constexpr ( InplaceAlloc::inplace_size > 0 )
        {
            if ( InplaceAlloc::inplace_size >= M )
            {
                auto p = InplaceAlloc::ptr();
                DynamicAlloc::set_ptr( p );
                return p;
            }
        }
        if constexpr ( use_dynamic_alloc )
            return DynamicAlloc::allocate( M, Align );
        else
            return nullptr;
    }
    constexpr bool can_allocate( std::size_t M ) const
    {
        return ( InplaceAlloc::inplace_size >= M ) ? true : use_dynamic_alloc;
    }

    void deallocate()
    {
        if ( is_using_inplace() )
            DynamicAlloc::deallocate();
    }
    void *inplace_ptr() const
    {
        return InplaceAlloc::ptr();
    }
    void *get_ptr() const
    {
        if constexpr ( use_dynamic_alloc )
            return DynamicAlloc::ptr();
        else
            return InplaceAlloc::ptr();
    }
    constexpr bool is_using_inplace() const
    {
        if constexpr ( use_dynamic_alloc )
            return DynamicAlloc::ptr() == InplaceAlloc::ptr();
        else
            return true;
    }

    // if both bUseDynamicAlloc
    template<std::size_t InplaceSizeT, std::size_t AlignT>
    std::enable_if_t<bUseDynamicAlloc, bool> try_movein_ptr( BoxedStorageBase<InplaceSizeT, AlignT, true> &another )
    {
        assert( is_using_inplace() );
        if ( !another.is_using_inplace() )
        {
            DynamicAlloc::set_ptr( another.get_ptr() );
            another.set_ptr( nullptr );
            return true;
        }
        return false;
    }
};

/// @class AnyStorageBase Inplace storage/allocator for any type. It can be used like std::any type and std::function with SBO.
/// Small buffer optimization: when the stored object size <= InplaceSize, object will be allocated in inplace. Otherwise, object will be
/// allocated dynamically. It can be copyable and/or moveable, which are specicified by template parameters.
///
/// @tparam TypeInfoAccessor Traits (can be TypeInfoAccess<...> or CallableTypeInfoAccess<...>:
///     const TypeInfo *get_typeinfo(); // get static TypeInfo
///     void set_typeinfo<T>();
///     void set_typeinfo(const TypeInfo *);
template<class TypeInfoAccessor, std::size_t InplaceSize, bool bUseDynamicAlloc, bool bCopyConstructible, bool bMoveConstructible>
struct AnyStorageBase : public BoxedStorageBase<InplaceSize, alignof( std::aligned_storage_t<InplaceSize> ), bUseDynamicAlloc>
{
    using base_type = BoxedStorageBase<InplaceSize, alignof( std::aligned_storage_t<InplaceSize> ), bUseDynamicAlloc>;
    using this_type = AnyStorageBase<TypeInfoAccessor, InplaceSize, bUseDynamicAlloc, bCopyConstructible, bMoveConstructible>;

    static const auto has_allocator = bUseDynamicAlloc;
    static const auto has_inplace = InplaceSize > 0;
    static const auto is_copyable = bCopyConstructible;
    static const auto is_moveable = bMoveConstructible;

    using base_type::allocate;
    using base_type::can_allocate;
    using base_type::deallocate;
    using base_type::get_ptr;
    using base_type::inplace_ptr;
    using base_type::is_using_inplace;

    TypeInfoAccessor m_typeInfo; // assume: as long as there's a typeinfo,

    ~AnyStorageBase()
    {
        destroy();
    }
    template<class T>
    AnyStorageBase( T &&val )
    {
        if ( !base_type::allocate( sizeof( T ) ) )
            throw std::length_error( "Inplace size is too small for type T!" );
        emplace_impl<T>( std::forward<T>( val ) );
    }
    template<class T, class... Args>
    AnyStorageBase( std::in_place_type_t<T>, Args &&... args )
    {
        if ( !base_type::allocate() )
            throw std::length_error( "Inplace size is too small for type T!" );
        emplace_impl<T>( std::forward<Args>( args )... );
    }

    // copy constructor
    template<std::size_t InplaceSizeT, bool bUseDynamicAllocT, bool bMoveConstructibleT>
    AnyStorageBase( const AnyStorageBase<TypeInfoAccessor, InplaceSizeT, bUseDynamicAllocT, true, bMoveConstructibleT> &another )
    {
        if ( auto pTypeInfo = another.m_typeInfo.get_typeinfo() )
        {
            if ( !base_type::allocate( pTypeInfo->pSize() ) )
                throw std::length_error( "Inplace size is too small to copy type in another!" );
            pTypeInfo->pCopy( base_type::get_ptr(), another.get_ptr() );
        }
        m_typeInfo.set_typeinfo( another.get_typeinfo() );
    }

    // move constructor
    template<std::size_t InplaceSizeT, bool bUseDynamicAllocT, bool bCopyConstructibleT>
    AnyStorageBase( AnyStorageBase<TypeInfoAccessor, InplaceSizeT, bUseDynamicAllocT, bCopyConstructibleT, true> &&another )
    {
        if ( auto pTypeInfo = another.m_typeInfo.get_typeinfo() )
        {
            if constexpr ( bUseDynamicAlloc && bUseDynamicAllocT )
            {
                if ( base_type::try_movein_ptr( another ) ) // move
                {
                    m_typeInfo.set_typeinfo( another.get_typeinfo() );
                    another.m_typeInfo.set_typeinfo( nullptr ); // no typeinfo in another
                    return;
                }
            }
            if ( !base_type::allocate( pTypeInfo->pSize() ) )
                throw std::length_error( "Inplace size is too small to copy type in another!" );
            pTypeInfo->pMove( base_type::get_ptr(), another.get_ptr() );
        }
        m_typeInfo.set_typeinfo( another.get_typeinfo() );
    }

    // allocate memory and set type info
    // throws std::length_error when allocation fails.
    template<std::size_t InplaceSizeT, bool bUseDynamicAllocT, bool bCopyConstructibleT, bool bMoveConstructibleT>
    void preallocate_for_copy(
            const AnyStorageBase<TypeInfoAccessor, InplaceSizeT, bUseDynamicAllocT, bCopyConstructibleT, bMoveConstructibleT> &another )
    {
        if ( auto pTypeInfo = another.m_typeInfo.get_typeinfo() )
        {
            // check  if it needs free and reallocate.
            if ( auto pOldInfo = m_typeInfo.get_typeinfo() )
            {
                if ( pOldInfo->pSize() < pTypeInfo->pSize() )
                {
                    destroy();
                    if ( !base_type::allocate( pTypeInfo->pSize() ) )
                        throw std::length_error( "Inplace size is too small to copy type in another after deallocate!" );
                }
                else
                    pOldInfo->pDestroy();
            }
            else
            {
                if ( !base_type::allocate( pTypeInfo->pSize() ) )
                    throw std::length_error( "Inplace size is too small to copy type in another!" );
            }
            //                pTypeInfo->pCopy( base_type::get_ptr(), another.get_ptr() );
        }
        else
        {
            if ( auto pOldInfo = m_typeInfo.get_typeinfo() )
                pOldInfo->pDestroy();
        }

        m_typeInfo.set_typeinfo( another.get_typeinfo() );
    }

    template<std::size_t InplaceSizeT, bool bUseDynamicAllocT, bool bMoveConstructibleT>
    this_type &operator=( const AnyStorageBase<TypeInfoAccessor, InplaceSizeT, bUseDynamicAllocT, true, bMoveConstructibleT> &another )
    {
        preallocate_for_copy( another );
        if ( auto pInfo = m_typeInfo.get_typeinfo() )
            pInfo->pCopy( base_type::get_ptr(), another.get_ptr() );
        return *this;
    }
    template<std::size_t InplaceSizeT, bool bUseDynamicAllocT, bool bCopyConstructibleT>
    this_type &operator=( AnyStorageBase<TypeInfoAccessor, InplaceSizeT, bUseDynamicAllocT, bCopyConstructibleT, true> &&another )
    {
        if constexpr ( bUseDynamicAlloc && bUseDynamicAllocT )
        {
            if ( auto pTypeInfo = another.m_typeInfo.get_typeinfo(); pTypeInfo && !another.is_using_inplace() )
            {
                destroy();
                base_type::try_movein_ptr( another ); // must return true;
                m_typeInfo.set_typeinfo( another.get_typeinfo() );
                another.m_typeInfo.set_typeinfo( nullptr ); // no typeinfo in another
                return *this;
            }
        }
        preallocate_for_copy( another );
        if ( auto pInfo = m_typeInfo.get_typeinfo() )
            pInfo->pMove( base_type::get_ptr(), another.get_ptr() );
        return *this;
    }

    void destroy()
    {
        destroy_object();
        base_type::deallocate();
        m_typeInfo.set_typeinfo( nullptr );
    }
    void destroy_object()
    {
        if ( auto pTypeInfo = m_typeInfo.get_typeinfo() )
            pTypeInfo->pDestroy();
    }

    void *get_ptr() const
    {
        return m_typeInfo.get_typeinfo() ? base_type::get_ptr() : nullptr;
    }

protected:
    template<class T, class... Args>
    void emplace_impl( Args &&... args )
    {
        new ( base_type::get_ptr() ) T( std::forward<Args>( args )... );
        m_typeInfo.template set_typeinfo<T>();
    }
};

} // namespace ftl
