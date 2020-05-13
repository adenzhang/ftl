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


    /// \tparam uiSize sizeof erasure. the acture size for functor is uiSize - sizeof(void*).
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
        constexpr static std::size_t SIZE = uiSize;
        constexpr static std::size_t SIZE_EFFECTIVE = SIZE - sizeof( void * ); /// sizeof buffer used to store funcion.

        //        using base_type = StoragePolicy<uiSize, false, bMutCallable, bCopyConstructible, bMoveConstructible>;

        // Needs to be declared here for use in decltype below
        constexpr static std::size_t FunctorStorageSize = SIZE_EFFECTIVE;
        const TypeInfoExType *m_pInfoEx = nullptr;
        char m_storage[FunctorStorageSize];

        void *get_storage()
        {
            return m_storage;
        }

    public:
        Erasure() : m_pInfoEx( nullptr )
        {
        }

        Erasure( std::nullptr_t ) : m_pInfoEx( nullptr )
        {
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

        /// \brief copy construct.
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

        /// \brief move construct.
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

        ~Erasure()
        {
            if ( m_pInfoEx )
            {
                m_pInfoEx->pDestroy( &m_storage );
                m_pInfoEx = nullptr;
            }
        }

        /// \brief copy.
        template<std::size_t M, bool bMutCallableT>
        Erasure &operator=( const Erasure<M, Signature, bMutCallableT, true, bMoveConstructible> &erasure )
        {
            static_assert( bMutCallableT == bMutCallable || !bMutCallableT, "Cannot convert from mutable call to immutable call!" );
            // dyanmic check : it should require erasure.size() <= this->capacity()
            static_assert( uiSize >= M, "Cannot move large to small memory!" );

            this->~Erasure();
            if ( erasure.m_pInfoEx )
                erasure.m_pInfoEx->pCopy( &m_storage, &erasure.m_storage );

            m_pInfoEx = erasure.m_pInfoEx;
            return *this;
        }

        /// \brief move copy.
        /// Will not work if moving into itself.
        template<std::size_t M, bool bMutCallableT>
        Erasure &operator=( Erasure<M, Signature, bMutCallableT, bCopyConstructible, true> &&erasure )
        {
            static_assert( bMutCallableT == bMutCallable || !bMutCallableT, "Cannot convert from mutable call to immutable call!" );
            // it should require erasure.size() <= this->capacity()
            static_assert( uiSize >= M, "Cannot move large to small memory!" );
            assert( &erasure != this );

            this->~Erasure();
            if ( erasure.m_pInfoEx )
                erasure.m_pInfoEx->pMove( &m_storage, &erasure.m_storage );

            m_pInfoEx = erasure.m_pInfoEx;
            return *this;
        }

        Erasure &operator=( std::nullptr_t )
        {
            this->~Erasure();
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

        /// \return internal functor size.
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
            return *target<T>();
        }

        template<class T, class... Args>
        T &emplace_brace_init( Args &&... args )
        {
            this->~Erasure();
            emplace_brace_init_impl<T>( std::forward<Args>( args )... );
            return *target<T>();
        }

        template<class T>
        T *target()
        {
            if ( !m_pInfoEx )
                return nullptr;
            void *p = &m_storage;
            return *static_cast<T *>( p );
        }

        template<class T>
        const T *target() const
        {
            if ( !m_pInfoEx )
                return nullptr;
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
            static_assert( !std::is_same<Signature, void>::value, "Non-callable erasure" );
            assert( m_pInfoEx && m_pInfoEx->pInvoke );
            return m_pInfoEx->pInvoke( &m_storage, std::forward<Args>( args )... );
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
            static_assert( sizeof( T ) <= SIZE_EFFECTIVE, "Functor T is too large" );

            ::new ( get_storage() ) T( std::forward<Args>( args )... );
            m_pInfoEx = &StaticCallableTypeInfo<T, Signature, bMutCallable>::value;
        }

        template<class T, class... Args>
        void emplace_brace_init_impl( Args &&... args )
        {
            static_assert( !bCopyConstructible || type_erasure::erasure_traits<T>::is_copy_constructible, "T must be is_copy_constructible" );
            static_assert( !bMoveConstructible || type_erasure::erasure_traits<T>::is_move_constructible, "T must be is_move_constructible" );
            static_assert( sizeof( T ) <= SIZE_EFFECTIVE, "Functor T is too large" );

            ::new ( get_storage() ) T{std::forward<Args>( args )...};
            m_pInfoEx = &StaticCallableTypeInfo<T, Signature, bMutCallable>::value;
        }

        template<class T>
        void construct( T &&value )
        {
            emplace_impl<RemoveCVRef_t<T>>( std::forward<T>( value ) );
        }
    }; // namespace type_erasure

} // namespace type_erasure

/// \brief memory layout of std::function: { functor_storage: 16 bytes, pointer to object manager: 8 bytes, _M_invoker: 8 bytes }
///     - In std::function, Effective size for functor is 16 bytes;
///     - std::function doesn't support move constuctor.
///
/// memory layout of InplaceFunction: { pointer to object manager : 8 bytes , functor_storage: inplace fixed size }
///    - it doesn't check nullptr when invoke InplaceFunction.
template<class Signature, std::size_t N, bool bCopyConstructible = true, bool bMoveConstructible = true>
using InplaceFunction = typename type_erasure::Erasure<N, Signature, false, bCopyConstructible, bMoveConstructible>;
template<class Signature, std::size_t N, bool bCopyConstructible = true, bool bMoveConstructible = true>
using MutableInplaceFunction = typename type_erasure::Erasure<N, Signature, true, bCopyConstructible, bMoveConstructible>;


/////////////////////////////////////////////////////////////////////////////////////////
///
///           Member functions
///
/////////////////////////////////////////////////////////////////////////////////////////

template<class MemberFuncT>
struct MemberFuncInvoke;

template<class Obj, class Ret, class... Args>
struct MemberFuncInvoke<Ret ( Obj::* )( Args... )>
{
    constexpr static auto is_const_member = false;
    using ClassType = std::remove_const_t<Obj>;
    using ObjectType = ClassType;
    using MemberFuncType = Ret ( Obj::* )( Args... );

    Ret invoke( MemberFuncType pMemberFunc, ObjectType &obj, Args &&... args ) const
    {
        return ( obj.*pMemberFunc )( std::forward<Args>( args )... );
    }
};

template<class Obj, class Ret, class... Args>
struct MemberFuncInvoke<Ret ( Obj::* )( Args... ) const>
{
    constexpr static auto is_const_member = true;
    using ClassType = std::remove_const_t<Obj>;
    using ObjectType = const ClassType;
    using MemberFuncType = Ret ( Obj::* )( Args... ) const;

    Ret invoke( MemberFuncType pMemberFunc, const ClassType &obj, Args &&... args ) const
    {
        return ( obj.*pMemberFunc )( std::forward<Args>( args )... );
    }
};
/////////////////////  StaticMemberFunc /////////////////////////////////////

/// \brief StaticMemberFunc. m_pMemberFunc is constexpr.
template<class PMemberFuncT, PMemberFuncT pMembFunc>
struct StaticMemberFunc : MemberFuncInvoke<PMemberFuncT>
{
    using base_type = MemberFuncInvoke<PMemberFuncT>;
    using base_type::ClassType;
    using base_type::invoke;
    using base_type::is_const_member;
    using base_type::MemberFuncType;
    constexpr static typename base_type::MemberFuncType m_pMemberFunc = pMembFunc; // todo: delect virtual or not, to optimize non-virtual member.

    template<class... Args>
    auto operator()( typename base_type::ClassType &obj, Args &&... args ) const
    {
        return base_type::invoke( m_pMemberFunc, obj, std::forward<Args>( args )... );
        //        return ( obj.*m_pMemberFunc )( std::forward<Args>( args )... );
    }
};
namespace internal
{
    template<auto val, class Obj, class Ret, class... Args>
    static constexpr auto member_func_impl( Ret ( Obj::* )( Args... ) const )
    {
        return StaticMemberFunc<Ret ( Obj::* )( Args... ) const, val>{};
    }
    template<auto val, class Obj, class Ret, class... Args>
    static constexpr auto member_func_impl( Ret ( Obj::* )( Args... ) )
    {
        return StaticMemberFunc<Ret ( Obj::* )( Args... ), val>{};
    }
}; // namespace internal

/// \brief MemberFunc with dynamic m_pMemberFunc.
template<class PMemberFuncT>
struct MemberFunc;
template<class Obj, class Ret, class... Args>
struct MemberFunc<Ret ( Obj::* )( Args... ) const>
{
    constexpr static bool is_const_member = true;
    using ObjectType = const Obj;
    using ClassType = std::remove_const_t<Obj>;
    using MemberFuncType = Ret ( Obj::* )( Args... ) const;

    MemberFuncType m_pMemberFunc = nullptr; // todo: delect virtual or not, to optimize non-virtual member.

    MemberFunc() = default;
    MemberFunc( Ret ( Obj::*pMemberFunc )( Args... ) const ) : m_pMemberFunc( pMemberFunc )
    {
    }

    auto operator()( ObjectType &obj, Args &&... args ) const
    {
        return ( obj.*m_pMemberFunc )( std::forward<Args>( args )... );
    }
};
template<class Obj, class Ret, class... Args>
struct MemberFunc<Ret ( Obj::* )( Args... )>
{
    constexpr static bool is_const_member = false;
    using ObjectType = Obj;
    using ClassType = std::remove_const_t<Obj>;
    using MemberFuncType = Ret ( Obj::* )( Args... );

    MemberFuncType m_pMemberFunc = nullptr; // todo: delect virtual or not, to optimize non-virtual member.

    MemberFunc() = default;
    MemberFunc( Ret ( Obj::*pMemberFunc )( Args... ) ) : m_pMemberFunc( pMemberFunc )
    {
    }

    auto operator()( ObjectType &obj, Args &&... args ) const
    {
        return ( obj.*m_pMemberFunc )( std::forward<Args>( args )... );
    }
};
/////////////////////  DelegateFunc /////////////////////////////////////

template<class PMemberFuncT>
struct DelegateFunc;
template<class Obj, class Ret, class... Args>
struct DelegateFunc<Ret ( Obj::* )( Args... ) const>
{
    constexpr static bool is_const_member = true;
    using ObjectType = const Obj;
    using ClassType = std::remove_const_t<Obj>;
    using MemberFuncType = Ret ( Obj::* )( Args... ) const;

    MemberFuncType m_pMemberFunc = nullptr; // todo: delect virtual or not, to optimize non-virtual member.
    ObjectType *m_pObj = nullptr;

    DelegateFunc() = default;
    DelegateFunc( Ret ( Obj::*pMemberFunc )( Args... ) const, ObjectType *obj ) : m_pMemberFunc( pMemberFunc ), m_pObj( obj )
    {
    }

    auto operator()( Args &&... args ) const
    {
        return ( m_pObj->*m_pMemberFunc )( std::forward<Args>( args )... );
    }
};
template<class Obj, class Ret, class... Args>
struct DelegateFunc<Ret ( Obj::* )( Args... )>
{
    constexpr static bool is_const_member = false;
    using ObjectType = Obj;
    using ClassType = std::remove_const_t<Obj>;
    using MemberFuncType = Ret ( Obj::* )( Args... );

    MemberFuncType m_pMemberFunc = nullptr; // todo: delect virtual or not, to optimize non-virtual member.
    ObjectType *m_pObj = nullptr;

    DelegateFunc() = default;
    DelegateFunc( Ret ( Obj::*pMemberFunc )( Args... ), ObjectType *obj ) : m_pMemberFunc( pMemberFunc ), m_pObj( obj )
    {
    }

    auto operator()( Args &&... args ) const
    {
        return ( m_pObj->*m_pMemberFunc )( std::forward<Args>( args )... );
    }
};

/////////////////////  DeletableDelegateFunc /////////////////////////////////////

template<class Deleter, class PMemberFuncT>
struct DeletableDelegateFunc;
template<class Deleter, class Obj, class Ret, class... Args>
struct DeletableDelegateFunc<Deleter, Ret ( Obj::* )( Args... ) const>
{
    constexpr static bool is_const_member = true;
    using ObjectType = const Obj;
    using ClassType = std::remove_const_t<Obj>;
    using MemberFuncType = Ret ( Obj::* )( Args... ) const;

    MemberFuncType m_pMemberFunc = nullptr; // todo: delect virtual or not, to optimize non-virtual member.
    ObjectType *m_pObj = nullptr;
    Deleter *m_deleter;

    DeletableDelegateFunc() = default;
    DeletableDelegateFunc( Ret ( Obj::*pMemberFunc )( Args... ) const,
                           ObjectType *obj,
                           std::conditional_t<std::is_reference_v<Deleter>, Deleter, const Deleter &> &deleter )
        : m_pMemberFunc( pMemberFunc ), m_pObj( obj ), m_deleter{deleter}
    {
    }

    auto operator()( Args &&... args ) const
    {
        return ( m_pObj->*m_pMemberFunc )( std::forward<Args>( args )... );
    }
};
template<class Deleter, class Obj, class Ret, class... Args>
struct DeletableDelegateFunc<Deleter, Ret ( Obj::* )( Args... )>
{
    constexpr static bool is_const_member = false;
    using ObjectType = Obj;
    using ClassType = std::remove_const_t<Obj>;
    using MemberFuncType = Ret ( Obj::* )( Args... );

    MemberFuncType m_pMemberFunc = nullptr; // todo: delect virtual or not, to optimize non-virtual member.
    ObjectType *m_pObj = nullptr;
    Deleter m_deleter;

    DeletableDelegateFunc() = default;
    DeletableDelegateFunc( Ret ( Obj::*pMemberFunc )( Args... ),
                           ObjectType *obj,
                           std::conditional_t<std::is_reference_v<Deleter>, Deleter, const Deleter &> &deleter )
        : m_pMemberFunc( pMemberFunc ), m_pObj( obj ), m_deleter{deleter}
    {
    }

    auto operator()( Args &&... args ) const
    {
        return ( m_pObj->*m_pMemberFunc )( std::forward<Args>( args )... );
    }
};

/// \return StaticMemberFunc
template<auto pMemberFunc>
constexpr auto member_func()
{
    return internal::member_func_impl<pMemberFunc>( pMemberFunc );
};

/// \return dynamic MemberFun.
template<class Obj, class Ret, class... Args>
auto member_func( Ret ( Obj::*pMemberFunc )( Args... ) )
{
    //    return MemberFunc<Ret ( Obj::* )( Args... )>( pMemberFunc );
    return [pMemberFunc]( Obj &obj, Args &&... args ) { return ( obj.*pMemberFunc )( std::forward<Args>( args )... ); };
}
template<class Obj, class Ret, class... Args>
auto member_func( Ret ( Obj::*pMemberFunc )( Args... ) const )
{
    //    return MemberFunc<Ret ( Obj::* )( Args... ) const>( pMemberFunc );
    return [pMemberFunc]( const Obj &obj, Args &&... args ) { return ( obj.*pMemberFunc )( std::forward<Args>( args )... ); };
}

/// \return DelegateFunc.
template<class Obj, class Ret, class... Args>
auto member_func( Ret ( Obj::*pMemberFunc )( Args... ), Obj *pobj )
{
    //    return DelegateFunc<Ret ( Obj::* )( Args... )>( pMemberFunc, pobj );
    return [pMemberFunc, pobj]( Args &&... args ) { return ( pobj->*pMemberFunc )( std::forward<Args>( args )... ); };
}
template<class Obj, class Ret, class... Args>
auto member_func( Ret ( Obj::*pMemberFunc )( Args... ) const, const Obj *pobj )
{
    //    return DelegateFunc<Ret ( Obj::* )( Args... ) const>( pMemberFunc, pobj );
    return [pMemberFunc, pobj]( Args &&... args ) { return ( pobj->*pMemberFunc )( std::forward<Args>( args )... ); };
}

/// \brief make delegate that owns (constructs) an object copy.
/// \return DelegateFunc.
template<class Obj, class Ret, class... Args>
auto member_func_copy( Ret ( Obj::*pMemberFunc )( Args... ), const Obj &obj )
{
    //    return DelegateFunc<Ret ( Obj::* )( Args... )>( pMemberFunc, pobj );
    return [pMemberFunc, obj = obj]( Args &&... args ) { return ( obj.*pMemberFunc )( std::forward<Args>( args )... ); };
}
/// \brief make delegate that owns (constructs) an object copy.
template<class Obj, class Ret, class... Args>
auto member_func_copy( Ret ( Obj::*pMemberFunc )( Args... ) const, Obj &&obj )
{
    //    return DelegateFunc<Ret ( Obj::* )( Args... ) const>( pMemberFunc, pobj );
    return [pMemberFunc, obj = std::move( obj )]( Args &&... args ) { return ( obj.*pMemberFunc )( std::forward<Args>( args )... ); };
}


/// \return dynamic DeletableDelegateFunc.
template<class Deleter, class Obj, class Ret, class... Args>
auto member_func( Ret ( Obj::*pMemberFunc )( Args... ),
                  Obj *pobj,
                  std::conditional_t<std::is_reference_v<Deleter>, Deleter, const Deleter &> &deleter )
{
    return DeletableDelegateFunc<Deleter, Ret ( Obj::* )( Args... )>( pMemberFunc, pobj, deleter );
}
template<class Deleter, class Obj, class Ret, class... Args>
auto member_func( Ret ( Obj::*pMemberFunc )( Args... ) const,
                  const Obj *pobj,
                  std::conditional_t<std::is_reference_v<Deleter>, Deleter, const Deleter &> &deleter )
{
    return DeletableDelegateFunc<Deleter, Ret ( Obj::* )( Args... ) const>( pMemberFunc, pobj, deleter );
}

} // namespace ftl
