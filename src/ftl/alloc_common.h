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
///    FreeList, AtomicFreeList
/////////////////////////////////////////////////////////////////////////////////


// @brief FreeList traits:
///    - typename Node
///    - FreeList(FreeList&& )
///    - T *pop()
///    - void push(T &)
///    - bool empty() const
//     - void clear()
template<class T>
struct FreeList
{
    struct FreeNode
    {
        FreeNode *pNext;

        FreeNode( T *next = nullptr ) : pNext( reinterpret_cast<FreeNode *>( next ) )
        {
        }
        FreeNode( FreeNode *next = nullptr ) : pNext( next )
        {
        }
    };
    using Node = FreeNode;

    // static_assert( sizeof( T ) >= sizeof( FreeNode * ), "sizeof T < sizeof(void *)" );

    FreeList() = default;
    FreeList( FreeList &&a )
    {
        pHead = a.pHead;
        a.pHead = nullptr;
    }

    void push( T &p )
    {
        new ( reinterpret_cast<FreeNode *>( &p ) ) FreeNode( pHead );
        pHead = reinterpret_cast<FreeNode *>( &p );
    }

    T *pop()
    {
        T *res = reinterpret_cast<T *>( pHead );
        if ( pHead )
            pHead = pHead->pNext;
        return res;
    }
    bool empty() const
    {
        return !pHead;
    }
    // release head / all.
    void clear()
    {
        pHead = nullptr;
    }
    // FreeList(const FreeList&) = delete;
    FreeNode *pHead = nullptr;
};

template<class U>
struct AtomicFreeList
{
    struct AtomicFreeNode
    {
        using this_type = AtomicFreeNode;
        using Ptr = std::atomic<AtomicFreeNode *>;

        static_assert( sizeof( U ) >= sizeof( std::atomic<AtomicFreeNode *> ), "Object U is too small!" );

        union {
            char buf[sizeof( U )];
            Ptr next;
        } data;

        void init()
        {
            new ( &data.next ) Ptr();
        }

        U *get_object()
        {
            return reinterpret_cast<U *>( data.buf );
        }
        Ptr &get_next_ref()
        {
            return data.next;
        }

        static void push( Ptr &head, U *pObj )
        {
            auto pNode = from_object( pObj );
            pNode->init();
            this_type *pHead = nullptr;
            do
            {
                pHead = head.load();
                pNode->get_next_ref() = pHead;
            } while ( !head.compare_exchange_weak( pHead, pNode ) );
        }

        static U *pop( Ptr &head )
        {
            auto pHead = head.load();
            this_type *pNext = nullptr;
            do
            {
                if ( !pHead )
                    break;
                pNext = pHead->data.next.load();
            } while ( !head.compare_exchange_weak( pHead, pNext ) );
            return pHead;
        }

        static AtomicFreeNode *from_object( U *pObj )
        {
            return reinterpret_cast<AtomicFreeNode *>( pObj );
        }
    };

    using Node = AtomicFreeNode;

    AtomicFreeList() = default;
    AtomicFreeList( AtomicFreeList &&a )
    {
        m_pHead = a.pHead;
        a.pHead = nullptr;
    }
    void push( U &p )
    {
        AtomicFreeNode::push( &m_pHead, &p );
    }
    U *pop()
    {
        return AtomicFreeNode::pop( &m_pHead );
    }
    bool empty() const
    {
        return nullptr == m_pHead.load();
    }
    void clear()
    {
        m_pHead = nullptr;
    }

    std::atomic<Node *> m_pHead;
};

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
///    GrowthPolicy
/////////////////////////////////////////////////////////////////////////////////

// linear growth: y = coef * x. For std::vector coef == 2.
struct LinearGrowthPolicy
{
    using size_type = std::size_t; // must be >= 0
    LinearGrowthPolicy( size_type initialVal, double coef = 2.0 )
        : m_val( std::max( size_type( 1 ), initialVal ) ),
          m_coef( coef ),
          m_bInc( coef > 1 ),
          m_limit( ( m_bInc ? std::numeric_limits<size_type>::max() : std::numeric_limits<size_type>::min() ) )
    {
    }

    LinearGrowthPolicy( size_type initialVal, double coef, size_type limit )
        : m_val( std::max( size_type( 1 ), initialVal ) ), m_coef( coef ), m_bInc( coef > 1 ), m_limit( limit )
    {
    }

    /// @param toatalVal accumulative total value
    size_type grow_to( size_type acculativeVal )
    {
        if ( m_bInc )
            return std::min( m_limit, std::max( m_val, size_type( acculativeVal * m_coef ) ) );
        return std::max( m_limit, std::min( m_val, size_type( acculativeVal * m_coef ) ) );
    }
    size_type &get_grow_value()
    {
        return m_val;
    }

private:
    size_type m_val; // current/recent grow value.
    double m_coef;
    bool m_bInc;
    size_type m_limit; // max/min value of m_val.
};


struct ConstGrowthPolicy
{
    using size_type = std::size_t; // must be >= 0

    ConstGrowthPolicy( size_type initialVal ) : m_val( initialVal )
    {
    }
    // always return initial value.
    size_type grow_to( size_type )
    {
        return m_val;
    }
    size_type m_val; // current/recent grow value.
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
