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
#include <utility>
#include <cstdint>
#include <cstdlib>
#include <cassert>
#include <stdexcept>

namespace ftl
{

template<typename F>
struct scope_exit
{
    scope_exit( F &&f ) : f( std::forward<F>( f ) )
    {
    }
    ~scope_exit()
    {
        if ( !mReleased )
            f();
    }

    void release()
    {
        mReleased = true;
    }

protected:
    F f;
    bool mReleased = false;
};

template<typename T>
using RemoveCVRef_t = std::remove_cv_t<std::remove_reference_t<T>>;

template<class Signature>
using FuncPtr = std::add_pointer_t<Signature>;

template<class F>
struct FuncRef;

template<class Ret, class... Args>
struct FuncRef<Ret( Args... )>
{
    template<class F, class = std::enable_if_t<!std::is_same_v<FuncRef, F>>>
    FuncRef( F &&f ) : m_func( call<std::remove_reference_t<F>> ), m_addr( reinterpret_cast<std::uintptr_t>( &f ) )
    {
    }

    Ret operator()( Args... args ) const
    {
        return m_func( m_addr, std::forward<Args>( args )... );
    }

private:
    template<class F>
    static Ret call( std::uintptr_t addr, Args... args )
    {
        return ( *reinterpret_cast<F *>( addr ) )( std::forward<Args>( args )... );
    }

private:
    FuncPtr<Ret( std::uintptr_t addr, Args... args )> m_func;
    std::uintptr_t m_addr;
};

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

///============  FuncPtr Erasure =================================
/// static function based implementation

namespace type_erasure
{
    //    using erasure_delete_type = FuncPtr<void( const void * )>;
    using erasure_destroy_type = FuncPtr<void( const void * )>;
    using erasure_copy_type = FuncPtr<void( void *, const void * )>;
    using erasure_move_type = FuncPtr<void( void *, void * )>;
    using erasure_size_type = FuncPtr<std::size_t()>;

    using erasure_noop_type = FuncPtr<void( const void * )>;


    template<class Result, class... Args>
    using erasure_invoke_type = FuncPtr<Result( const void *, Args &&... )>;

    template<class T>
    struct erasure_traits
    {
        static constexpr auto is_copy_constructible = std::is_constructible_v<T, const T &>;
        static constexpr auto is_move_constructible = std::is_constructible_v<T, T &&>;
        static constexpr auto is_const = std::is_const_v<T>;

        static void destroy( const void *p )
        {
            reinterpret_cast<const T *>( p )->~T();
        }

        static void copy( void *pDest, const void *pSrc )
        {
            if constexpr ( is_copy_constructible )
                ::new ( pDest ) T( *reinterpret_cast<const T *>( pSrc ) );
        }
        static void move( void *pDest, void *pSrc )
        {
            if constexpr ( is_move_constructible )
                ::new ( pDest ) T( std::move( *reinterpret_cast<T *>( pSrc ) ) );
        }

        template<class Result, class... Args>
        static Result invoke( const void *pFunctor, Args &&... args )
        {
            return ( *reinterpret_cast<const T *>( pFunctor ) )( std::forward<Args>( args )... );
        }

        template<class Result, class... Args>
        static Result invoke_mut( const void *pFunctor, Args &&... args )
        {
            return ( *reinterpret_cast<T *>( const_cast<void *>( pFunctor ) ) )( std::forward<Args>( args )... );
        }

        static bool can_cast( const void *pU )
        {
            return dynamic_cast<const T *>( pU );
        }

        static constexpr std::size_t size()
        {
            return sizeof( T );
        }

        void noop( const void * )
        {
        }

        // todo compare: ==, !=
    };

    struct TypeErasureInfoBase
    {
        template<class T>
        constexpr TypeErasureInfoBase( erasure_traits<T> )
            : pDestroy( &erasure_traits<T>::destroy ),
              pCopy( &erasure_traits<T>::copy ),
              pMove( &erasure_traits<T>::move ),
              pSize( &erasure_traits<T>::size )
        {
        }

        erasure_destroy_type pDestroy;
        erasure_copy_type pCopy;
        erasure_move_type pMove;
        erasure_size_type pSize;
    };

    //    template<class U>
    //    struct TypeErasureInfo : TypeErasureInfoBase
    //    {
    //        template<class T = U>
    //        constexpr TypeErasureInfo( erasure_traits<T> ) : TypeErasureInfoBase( erasure_traits<T>{} )
    //        {
    //        }
    //    };

    template<class Signature, bool bMutCallable>
    struct TypeErasureInvokeInfo : TypeErasureInfoBase
    {
        template<class T>
        constexpr TypeErasureInvokeInfo( erasure_traits<T> id ) : TypeErasureInfoBase( id ), pInvoke( &erasure_traits<T>::noop )
        {
        }

        erasure_noop_type pInvoke;
    };

    template<class Result, class... Args>
    struct TypeErasureInvokeInfo<Result( Args... ), true> : TypeErasureInfoBase
    {
        template<class T>
        constexpr TypeErasureInvokeInfo( erasure_traits<T> id )
            : TypeErasureInfoBase( id ), pInvoke( &(erasure_traits<T>::template invoke_mut<Result, Args...>))
        {
        }

        erasure_invoke_type<Result, Args...> pInvoke;
    };
    template<class Result, class... Args>
    struct TypeErasureInvokeInfo<Result( Args... ), false> : TypeErasureInfoBase
    {
        template<class T>
        constexpr TypeErasureInvokeInfo( erasure_traits<T> id )
            : TypeErasureInfoBase( id ), pInvoke( &(erasure_traits<T>::template invoke<Result, Args...>))
        {
        }

        erasure_invoke_type<Result, Args...> pInvoke;
    };



    template<class T>
    struct StaticTypeInfo
    {
        static const TypeErasureInfoBase value;
    };
    template<class T>
    const TypeErasureInfoBase StaticTypeInfo<T>::value = {erasure_traits<T>()};

    template<class Signature, bool bMutCallable = false>
    using CallableTypeInfo = TypeErasureInvokeInfo<Signature, bMutCallable>;

    template<class T, class Signature = void, bool bMutCallable = false>
    struct StaticCallableTypeInfo
    {
        static const CallableTypeInfo<Signature, bMutCallable> value;
    };
    template<class T, class Signature, bool bMutCallable>
    const CallableTypeInfo<Signature, bMutCallable> StaticCallableTypeInfo<T, Signature, bMutCallable>::value = {erasure_traits<T>()};


    template<class TypeInfo = TypeErasureInfoBase>
    struct TypeInfoAccess
    {

        const TypeInfo *m_pInfo = nullptr;
        auto get_typeinfo() const
        {
            return m_pInfo;
        }
        void set_typeinfo( const TypeInfo *pInfo )
        {
            m_pInfo = pInfo;
        }
        template<class... U, auto... V>
        void set_typeinfo()
        {
            m_pInfo = &StaticTypeInfo<U..., V...>::value;
        }
    };
    template<class TypeInfo>
    struct CallableTypeInfoAccess : TypeInfoAccess<TypeInfo>
    {
        template<class... U, auto... V>
        void set_typeinfo()
        {
            TypeInfoAccess<TypeInfo>::m_pInfo = &StaticCallableTypeInfo<U..., V...>::value;
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

    template<std::size_t uiSize,
             class Signature,
             //             bool bUseDynamicAlloc,
             bool bMutCallable = false,
             bool bCopyConstructible = true,
             bool bMoveConstructible = true>
    struct Erasure //: public StoragePolicy<uiSize, bUseDynamicAlloc, bMutCallable, bCopyConstructible, bMoveConstructible>
    {
    public:
        using TypeInfoExType = CallableTypeInfo<Signature, bMutCallable>;
        using this_type = Erasure;
        //        using base_type = StoragePolicy<uiSize, false, bMutCallable, bCopyConstructible, bMoveConstructible>;

        // Needs to be declared here for use in decltype below
        std::aligned_storage_t<uiSize> m_storage;
        const TypeInfoExType *m_pInfoEx;

    public:
        Erasure() : m_pInfoEx( nullptr )
        {
        }

        // throws std::length_error
        template<std::size_t M, bool bMutCallableT>
        Erasure( const Erasure<M, Signature, bMutCallableT, true, bMoveConstructible> &erasure )
        {
            static_assert( bMutCallableT == bMutCallable || !bMutCallableT, "Cannot convert from mutable call to immutable call!" );
            // it should require erasure.size() <= this->capacity()
            static_assert( uiSize >= M, "Cannot move large to small memory!" );
            if ( erasure.m_pInfoEx )
            {
                erasure.m_pInfoEx->pCopy( &m_storage, &erasure.m_storage );
            }

            m_pInfoEx = reinterpret_cast<const TypeInfoExType *>( erasure.m_pInfoEx );
        }

        template<std::size_t M, bool bMutCallableT>
        Erasure( Erasure<M, Signature, bMutCallableT, bCopyConstructible, true> &&erasure )
        {
            static_assert( bMutCallableT == bMutCallable || !bMutCallableT, "Cannot convert from mutable call to immutable call!" );
            // it should require erasure.size() <= this->capacity()
            static_assert( uiSize >= M, "Cannot move large to small memory!" );
            assert( static_cast<void *>( &erasure ) != static_cast<void *>( this ) );

            if ( erasure.m_pInfoEx )
            {
                erasure.m_pInfoEx->pMove( &m_storage, &erasure.m_storage );
            }

            m_pInfoEx = reinterpret_cast<const TypeInfoExType *>( erasure.m_pInfoEx );
        }

        template<class T, class Test = std::enable_if_t<!std::is_same_v<this_type, RemoveCVRef_t<T>>>>
        Erasure( T &&value )
        {
            construct( std::forward<T>( value ) );
        }

        template<class T, class... Args>
        Erasure( std::in_place_type_t<T>, Args &&... args )
        {
            emplace_impl<T>( std::forward<Args>( args )... );
        }

        ~Erasure()
        {
            if ( m_pInfoEx )
            {
                m_pInfoEx->pDestroy( &m_storage );
            }
        }

        template<std::size_t M, bool bMutCallableT>
        Erasure &operator=( const Erasure<M, Signature, bMutCallableT, true, bMoveConstructible> &erasure )
        {
            static_assert( bMutCallableT == bMutCallable || !bMutCallableT, "Cannot convert from mutable call to immutable call!" );
            // it should require erasure.size() <= this->capacity()
            static_assert( uiSize >= M, "Cannot move large to small memory!" );
            this->~Erasure();

            if ( erasure.m_pInfoEx )
            {
                erasure.m_pInfoEx->pCopy( &m_storage, &erasure.m_storage );
            }

            m_pInfoEx = erasure.m_pInfoEx;

            return *this;
        }

        // Will not work if moving into itself
        template<std::size_t M, bool bMutCallableT>
        Erasure &operator=( Erasure<M, Signature, bMutCallableT, bCopyConstructible, true> &&erasure )
        {
            static_assert( bMutCallableT == bMutCallable || !bMutCallableT, "Cannot convert from mutable call to immutable call!" );
            // it should require erasure.size() <= this->capacity()
            static_assert( uiSize >= M, "Cannot move large to small memory!" );
            assert( &erasure != this );

            this->~Erasure();

            if ( erasure.m_pInfoEx )
            {
                erasure.m_pInfoEx->pMove( &m_storage, &erasure.m_storage );
            }

            m_pInfoEx = erasure.m_pInfoEx;

            return *this;
        }

        template<class T, class Test = std::enable_if_t<!std::is_same_v<this_type, RemoveCVRef_t<T>>>>
        Erasure &operator=( T &&value )
        {
            this->~Erasure();
            construct( std::forward<T>( value ) );
            return *this;
        }

        bool empty() const
        {
            return nullptr == m_pInfoEx;
        }

        explicit operator bool() const
        {
            return !empty();
        }

        std::size_t size() const
        {
            return empty() ? 0 : m_pInfoEx->pSize();
        }
        constexpr std::size_t capacity() const
        {
            return uiSize;
        }

        template<class T, class... Args>
        T &emplace( Args &&... args )
        {
            this->~Erasure();
            emplace_impl<T>( std::forward<Args>( args )... );
            return get<T>();
        }

        template<class T, class... Args>
        T &emplace_brace_init( Args &&... args )
        {
            this->~Erasure();
            emplace_brace_init_impl<T>( std::forward<Args>( args )... );
            return get<T>();
        }

        template<class T>
        T &get()
        {
            assert( m_pInfoEx );
            void *p = &m_storage;
            return *static_cast<T *>( p );
        }

        template<class T>
        const T &get() const
        {
            assert( m_pInfoEx );
            const void *p = &m_storage;
            return *static_cast<const T *>( p );
        }

        // If you want to support a mutable callable (such as a mutable lambda),
        // must wrap it in something like const_unsafe_fun before assigning to the Erasure.
        // See: http://www.open-std.org/jtc1/sc22/wg21/docs/papers/2015/n4348.html
        template<class... Args>
        auto call( Args &&... args ) const
        {
            static_assert( !std::is_same<Signature, void>::value, "Non-callable erasure" );
            assert( m_pInfoEx && m_pInfoEx->pInvoke );
            return m_pInfoEx->pInvoke( &m_storage, std::forward<Args>( args )... );
        }
        template<class... Args>
        auto operator()( Args &&... args ) const
        {
            return call( std::forward<Args>( args )... );
        }

        void clear()
        {
            ~Erasure();
            m_pInfoEx = nullptr;
        }

        const TypeInfoExType *info() const
        {
            return m_pInfoEx;
        }

    private:
        template<class T, class... Args>
        void emplace_impl( Args &&... args )
        {
            static_assert( !bCopyConstructible || type_erasure::erasure_traits<T>::is_copy_constructible, "T must be is_copy_constructible" );
            static_assert( !bMoveConstructible || type_erasure::erasure_traits<T>::is_move_constructible, "T must be is_move_constructible" );
            static_assert( sizeof( T ) <= uiSize, "T is too large" );

            ::new ( &m_storage ) T( std::forward<Args>( args )... );
            m_pInfoEx = &StaticCallableTypeInfo<T, Signature, bMutCallable>::value;
        }

        template<class T, class... Args>
        void emplace_brace_init_impl( Args &&... args )
        {
            static_assert( !bCopyConstructible || type_erasure::erasure_traits<T>::is_copy_constructible, "T must be is_copy_constructible" );
            static_assert( !bMoveConstructible || type_erasure::erasure_traits<T>::is_move_constructible, "T must be is_move_constructible" );
            static_assert( sizeof( T ) <= uiSize, "T is too large" );

            ::new ( &m_storage ) T{std::forward<Args>( args )...};
            m_pInfoEx = &StaticCallableTypeInfo<T, Signature, bMutCallable>::value;
        }

        template<class T>
        void construct( T &&value )
        {
            emplace_impl<RemoveCVRef_t<T>>( std::forward<T>( value ) );
        }
    }; // namespace type_erasure

} // namespace type_erasure

template<class Signature, std::size_t N, bool bCopyConstructible = true, bool bMoveConstructible = true>
using InplaceFunction = typename type_erasure::Erasure<N, Signature, false, bCopyConstructible, bMoveConstructible>;
template<class Signature, std::size_t N, bool bCopyConstructible = true, bool bMoveConstructible = true>
using MutableInplaceFunction = typename type_erasure::Erasure<N, Signature, true, bCopyConstructible, bMoveConstructible>;

} // namespace ftl
