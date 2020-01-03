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

// allocate only once
template<std::size_t InplaceSize, std::size_t Align, bool bUseDynamicAlloc>
struct AlignedOnceAllocatorWithInplace : private InplaceStorage<InplaceSize, Align>
{
    using InplaceAlloc = InplaceStorage<InplaceSize, Align>;
    static constexpr auto use_dynamic_alloc = bUseDynamicAlloc;

    void *allocate( std::size_t M )
    {
        if constexpr ( InplaceAlloc::inplace_size > 0 )
        {
            if ( InplaceAlloc::inplace_size >= M )
                return InplaceAlloc::ptr();
        }
        if constexpr ( use_dynamic_alloc )
            return std::aligned_alloc( Align, M );
        else
            return nullptr;
    }
    constexpr bool can_allocate( std::size_t M )
    {
        return ( InplaceAlloc::inplace_size >= M ) ? true : use_dynamic_alloc;
    }

    void deallocate( void *p )
    {
        if constexpr ( InplaceAlloc::inplace_size > 0 )
        {
            if ( InplaceAlloc::ptr() == p )
                return;
        }
        std::free( p );
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

    struct TypeErasureBase
    {
        template<class T>
        constexpr TypeErasureBase( erasure_traits<T> )
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

    template<class Signature, bool bMutCallable>
    struct TypeErasureBaseEx : TypeErasureBase
    {
        template<class T>
        constexpr TypeErasureBaseEx( erasure_traits<T> id ) : TypeErasureBase( id ), pInvoke( &erasure_traits<T>::noop )
        {
        }

        erasure_noop_type pInvoke;
    };

    template<class Result, class... Args>
    struct TypeErasureBaseEx<Result( Args... ), true> : TypeErasureBase
    {
        template<class T>
        constexpr TypeErasureBaseEx( erasure_traits<T> id )
            : TypeErasureBase( id ), pInvoke( &(erasure_traits<T>::template invoke_mut<Result, Args...>))
        {
        }

        erasure_invoke_type<Result, Args...> pInvoke;
    };
    template<class Result, class... Args>
    struct TypeErasureBaseEx<Result( Args... ), false> : TypeErasureBase
    {
        template<class T>
        constexpr TypeErasureBaseEx( erasure_traits<T> id ) : TypeErasureBase( id ), pInvoke( &(erasure_traits<T>::template invoke<Result, Args...>))
        {
        }

        erasure_invoke_type<Result, Args...> pInvoke;
    };

    template<class Signature, bool bMutCallable = false>
    using TypeInfoCallable = TypeErasureBaseEx<Signature, bMutCallable>;

    template<class T, class Signature = void, bool bMutCallable = false>
    struct StaticTypeInfo
    {
        static const TypeInfoCallable<Signature, bMutCallable> value;
    };
    template<class T, class Signature, bool bMutCallable>
    const TypeInfoCallable<Signature, bMutCallable> StaticTypeInfo<T, Signature, bMutCallable>::value = {erasure_traits<T>()};


    template<std::size_t InplaceSize,
             bool bUseDynamicAlloc,
             bool bMutCallable = false,
             bool bCopyConstructible = true,
             bool bMoveConstructible = true>
    struct StoragePolicy : public AlignedOnceAllocatorWithInplace<InplaceSize, alignof( std::aligned_storage_t<InplaceSize> ), bUseDynamicAlloc>
    {
        using base_type = AlignedOnceAllocatorWithInplace<InplaceSize, alignof( std::aligned_storage_t<InplaceSize> ), bUseDynamicAlloc>;

        static const auto has_allocator = bUseDynamicAlloc;
        static const auto has_inplace = InplaceSize > 0;
        static const auto is_mutable_callable = bMutCallable, is_copyable = bCopyConstructible, is_moveable = bMoveConstructible;

        using base_type::allocate;
        using base_type::can_allocate;
        using base_type::deallocate;

        bool is_inplace( const void *p ) const
        {
            return base_type::ptr() == p;
        }
    };

    template<std::size_t uiSize, class Signature = void, bool bMutCallable = false, bool bCopyConstructible = true, bool bMoveConstructible = true>
    struct Erasure
    {
    public:
        using TypeInfoExType = TypeInfoCallable<Signature, bMutCallable>;
        using this_type = Erasure;

        // Needs to be declared here for use in decltype below
        std::aligned_storage_t<uiSize> m_storage;
        const TypeInfoExType *m_pInfoEx;

    public:
        Erasure() : m_pInfoEx( nullptr )
        {
        }

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
            m_pInfoEx = &StaticTypeInfo<T, Signature, bMutCallable>::value;
        }

        template<class T, class... Args>
        void emplace_brace_init_impl( Args &&... args )
        {
            static_assert( !bCopyConstructible || type_erasure::erasure_traits<T>::is_copy_constructible, "T must be is_copy_constructible" );
            static_assert( !bMoveConstructible || type_erasure::erasure_traits<T>::is_move_constructible, "T must be is_move_constructible" );
            static_assert( sizeof( T ) <= uiSize, "T is too large" );

            ::new ( &m_storage ) T{std::forward<Args>( args )...};
            m_pInfoEx = &StaticTypeInfo<T, Signature, bMutCallable>::value;
        }

        template<class T>
        void construct( T &&value )
        {
            emplace_impl<RemoveCVRef_t<T>>( std::forward<T>( value ) );
        }
    }; // namespace type_erasure

} // namespace type_erasure

template<class Signature, std::size_t N, bool bCopyConstructible = true, bool bMoveConstructible = true>
using FixedFunction = typename type_erasure::Erasure<N, Signature, false, bCopyConstructible, bMoveConstructible>;
template<class Signature, std::size_t N, bool bCopyConstructible = true, bool bMoveConstructible = true>
using MutableFixedFunction = typename type_erasure::Erasure<N, Signature, true, bCopyConstructible, bMoveConstructible>;

} // namespace ftl
