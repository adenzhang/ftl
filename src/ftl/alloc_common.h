#pragma once
#include <cassert>
#include <atomic>
#include <algorithm>

namespace ftl
{
using Byte = unsigned char;

inline constexpr bool is_pow2( std::size_t n )
{
    return n != 0 && ( n & ( n - 1 ) ) == 0;
}

// Result is >= n
template<class Int>
inline constexpr Int align_up( Int n, Int uiAlignment )
{
    assert( n > 0 );
    assert( is_pow2( uiAlignment ) );
    return ( n + ( uiAlignment - 1 ) ) & ~( uiAlignment - 1 );
}

////////////////////////////////////////////////////////////////////
/// \brief Instrusive Singly List - Atomic Operations
////////////////////////////////////////////////////////////////////

template<typename T>
void PushSinglyListNode( std::atomic<T *> *head, T *node, std::atomic<T *> T::*pMemberNext )
{
    T *pHead = nullptr;
    do
    {
        pHead = head->load();
        node->*pMemberNext = pHead;
    } while ( !head->compare_exchange_weak( pHead, node ) );
}

template<typename T>
T *PopSinglyListNode( std::atomic<T *> *head, std::atomic<T *> T::*pMemberNext )
{
    auto pHead = head->load();
    T *pNext = nullptr;
    do
    {
        if ( !pHead )
            break;
        pNext = ( pHead->*pMemberNext ).load();
    } while ( !head->compare_exchange_weak( pHead, pNext ) );
    return pHead;
}

// non-atomic push singly list node
template<typename T>
void PushSinglyListNode( T **head, T *node, T *T::*pMemberNext )
{
    node->*pMemberNext = *head;
    *head = node;
}

template<typename T>
T *PopSinglyListNode( T **head, T *T::*pMemberNext )
{
    if ( *head )
    {
        T *top = *head;
        *head = top->*pMemberNext;
        return top;
    }
    return nullptr;
}
//------- static pMemberNext, more efficient --------------
template<typename T, std::atomic<T *> T::*pMemberNext>
void PushSinglyListNode( std::atomic<T *> *head, T *node )
{
    T *pHead = nullptr;
    do
    {
        pHead = head->load();
        node->*pMemberNext = pHead;
    } while ( !head->compare_exchange_weak( pHead, node ) );
}

template<typename T, std::atomic<T *> T::*pMemberNext>
T *PopSinglyListNode( std::atomic<T *> *head )
{
    auto pHead = head->load();
    T *pNext = nullptr;
    do
    {
        if ( !pHead )
            break;
        pNext = ( pHead->*pMemberNext ).load();
    } while ( !head->compare_exchange_weak( pHead, pNext ) );
    return pHead;
}

// non-atomic push singly list node
template<typename T, T *T::*pMemberNext>
void PushSinglyListNode( T **head, T *node )
{
    node->*pMemberNext = *head;
    *head = node;
}

template<typename T, T *T::*pMemberNext>
T *PopSinglyListNode( T **head )
{
    if ( *head )
    {
        T *top = *head;
        *head = top->*pMemberNext;
        return top;
    }
    return nullptr;
}


/////////////////////////////////////////////////////////////////////////////////
///    StaticFnDeleter
/////////////////////////////////////////////////////////////////////////////////

/// @brief StaticFnDeleter call fn(D*, T*) to delete object.
template<class T>
struct StaticFnDeleter
{
    using D = void *;
    using DelFn = void ( * )( D, T * );

    DelFn pDelFn = nullptr;
    D pDeleter = D();

    template<class DT>
    StaticFnDeleter( DelFn pF, DT pD ) : pDelFn( pF ), pDeleter( static_cast<D>( pD ) )
    {
    }

    StaticFnDeleter() = default;

    void deallocate( T *p )
    {
        assert( pDelFn );
        ( *pDelFn )( pDeleter, p );
    }

    void operator()( T *p )
    {
        p->~T();
        deallocate( p );
    }
};

/////////////////////////////////////////////////////////////////////////////////
///    Allocator trait
///      - T *allocate( size_t )
///      - void deallocate( T*, size_t )
///      - clear()   // return all allocated elements to free list
/////////////////////////////////////////////////////////////////////////////////

/// add clear() to std::allocator
template<class T, class Alloc = std::allocator<T>>
class AllocWrapper : public Alloc
{
public:
    template<class... Args>
    AllocWrapper( Args &&... args ) : Alloc( std::forward<Args>( args )... )
    {
    }
    void clear()
    {
    }
};

} // namespace ftl
