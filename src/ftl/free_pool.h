#pragma once

#include <type_traits>
#include <optional>
#include <memory>
#include <vector>
#include <cassert>

#define USE_DELETERADAPTOR

namespace ftl{


template<class A, class T>
using member_deallocate_pointer = void (A::*)(T*); 

template<class T>
struct IDeallocator
{
    virtual void deallocate(T* p) = 0;
    virtual ~IDeallocator() {}
};

template<class T>
struct IDeleter
{
    virtual void operator()(T* p) = 0;
    virtual ~IDeleter() {}
};

template<class T, class D = IDeleter<T>, member_deallocate_pointer<D, T> pDelFn = static_cast<member_deallocate_pointer<D, T>>(&D::operator())>
struct DeleterAdaptor
{
    D *d;
    DeleterAdaptor( D* d = nullptr):d(d){}

    void operator()(T* p){
        assert(d);
        ((*d).*pDelFn)(p);
    }
};

// most generic deleter which calls pDelFn( T*, D& ) to delete
// D must be copy constructible
// it can be used as deleter or deallocator
template<class T, class D = void *>
struct GenDeleter
{
    using DelFn = void (*)(T*, D&);

    DelFn pDelFn = nullptr;
    D pDeleter = D();

    template<class DT>
    GenDeleter( DelFn pF, DT pD)
        : pDelFn(pF)
        , pDeleter(static_cast<D>(pD)){}

    GenDeleter() = default;

    void deallocate(T* p)
    {
        assert( pDelFn );
        (*pDelFn)( p, pDeleter);
    }

    void operator()(T* p)
    {
        deallocate(p);
    }
};


struct RefDeallocator_Tag
{
};

struct CopyDeallocator_Tag
{
};

// ChildT implements call_dealloacate( T*,  D& );
template<class ChildT, class T , class D = IDeallocator<T>, class SaveDeallocTag = CopyDeallocator_Tag>
struct IDeletableBase
{
    using value_type = T;
    using this_type = IDeletableBase;
    using deallocator_type = D;

    static constexpr bool is_deallocator_copied = std::is_same<SaveDeallocTag, CopyDeallocator_Tag>::value;

    using saved_deallocator_type = std::conditional_t< is_deallocator_copied, std::optional<deallocator_type>,  deallocator_type*>;

    IDeletableBase(deallocator_type &dealloc) {
        set_deallocator(dealloc);
    }

    IDeletableBase() {}

    IDeletableBase(const this_type& ) {}
    this_type& operator=(const this_type&) {
        return this;
    }

    template<bool bCopy = is_deallocator_copied>
    std::enable_if_t<bCopy, void> set_deallocator( const deallocator_type &dealloc) {
            mDeallocator = dealloc;
    }
    template<bool bCopy = is_deallocator_copied>
    std::enable_if_t<!bCopy, void> set_deallocator( deallocator_type &dealloc) {
            mDeallocator = &dealloc;
    }
    
    void reset_deallocator() {
            mDeallocator = saved_deallocator_type();
    }

    const auto &get_deallocator() const {
        return mDeallocator;
    }

    auto &get_deallocator()  {
        return mDeallocator;
    }

    void deallocate_me() {
        if( auto& d = get_deallocator() ) {
            reset_deallocator();
            static_cast<T*>(this)->~T(); // call destructor
            static_cast<ChildT*>(this)->call_deallocate(static_cast<T*>(this), *d);
        }
    }

    virtual ~IDeletableBase() {
        if( auto& d = get_deallocator() ) {
            reset_deallocator();
            static_cast<ChildT*>(this)->call_deallocate(static_cast<T*>(this), *d);
        }
    }

private:
    saved_deallocator_type mDeallocator = saved_deallocator_type();
};

// call D(T*) to delete
// save deallocator by default
template<class T, class D = IDeallocator<T>, class SaveOrRefDeallocTag = CopyDeallocator_Tag >
struct IDeletable : IDeletableBase<IDeletable<T,D, SaveOrRefDeallocTag>, T, D, SaveOrRefDeallocTag >
{
    using value_type = T;
    using deallocator_type = D;
    using this_type = IDeletable;
    using parent_type = IDeletableBase<IDeletable<T,D, SaveOrRefDeallocTag>, T, D, SaveOrRefDeallocTag >;

    IDeletable(deallocator_type &dealloc): parent_type(dealloc) {}

    IDeletable() {}

    void call_deallocate(T* p, D& d)
    {
        d(p);
    }
};

// D implements member function void D::DelFn(T*)
// reference to deallocator by default
template<class T, class D = IDeallocator<T>, class SaveOrRefDeallocTag = RefDeallocator_Tag, member_deallocate_pointer<D, T> DelFn = static_cast<member_deallocate_pointer<D, T>>(&D::deallocate) >
struct IDeallocatable : IDeletableBase<IDeallocatable<T,D, SaveOrRefDeallocTag, DelFn>, T, D, SaveOrRefDeallocTag >
{
    using value_type = T;
    using deallocator_type = D;
    using this_type = IDeallocatable;
    using parent_type = IDeletableBase<IDeallocatable<T,D, SaveOrRefDeallocTag, DelFn>, T, D, SaveOrRefDeallocTag >;

    IDeallocatable(deallocator_type &dealloc): parent_type(dealloc) {}

    IDeallocatable() {}

    void call_deallocate(T* p, D& d)
    {
        (d.*DelFn)(p);
    }
};

// calls D::operator()(T*).
// copy deallocator by default
template <class T, class D = IDeleter<T>, class CopyOrRef = CopyDeallocator_Tag,  member_deallocate_pointer<D, T> DelFn = static_cast<member_deallocate_pointer<D, T>>(&D::operator())>
using IDeletableFn = IDeallocatable<T, D, CopyOrRef, DelFn>;

template<class T>
struct null_delete{
    void operator()(T* ){}
};

template<class T, class Deleter = std::default_delete<T> >
struct FreeList
{
    struct FreeNode
    {
        FreeNode *pNext;

        FreeNode(T *next = nullptr):pNext( reinterpret_cast<FreeNode*>(next)){}
        FreeNode(FreeNode *next = nullptr):pNext(next){}
    };

    FreeList(T *p = nullptr, const Deleter& d ={}): pHead( reinterpret_cast<FreeNode*>(p)),mDel(d) {}

    FreeList(const Deleter& d):mDel(d){}

    void push(T &p){
        new (reinterpret_cast<FreeNode*>(&p)) FreeNode(pHead);
        pHead = reinterpret_cast<FreeNode*>(&p);
    }

    T* pop() {
        T* res = reinterpret_cast<T*>(pHead);
        if( pHead ) 
            pHead = pHead->pNext;
        return res;
    }
    bool empty() const {
        return !pHead;
    }

    template<class D = Deleter>
    typename std::enable_if<std::is_same<D, null_delete<T>>::value, void >::type clear() {
        pHead = nullptr;
    }

    template<class D = Deleter>
    typename std::enable_if<!std::is_same<D, null_delete<T>>::value, void >::type clear() {
        while( auto p=pop() ) {
            mDel( p );
        }
        pHead = nullptr;
    }

    FreeNode *pHead = nullptr;
    Deleter mDel;
};



// allocate when constructing BasePool
// ChildT implements: T *do_alloc()
template<class T, class FreeList, class ChildT >
class BasePool : IDeallocator<T>, IDeleter<T>
{
public:
    using child_type = ChildT;
    using this_type = BasePool;


//    using T = typename ChildT::value_type;
//    using FreeList = typename ChildT::freelist_type;


    BasePool( const FreeList& fl): freeList(fl) {}

    FreeList& get_freelist(){
        return freeList;
    }

    T* allocate()
    {
        if( auto p = freeList.pop() )
            return p;
        else {
//            auto pChild = dynamic_cast<child_type*>(this);
//            assert(pChild && "Unable to get child");
//            return pChild->do_alloc();
            return do_alloc();
        }
    }

    virtual T* do_alloc() = 0;

    template<class... Args>
    T* create(Args&&... args) {
        if( auto p = allocate() ) {
            new(p) T(std::forward<Args>(args)...);
            if constexpr( std::is_same_v<typename T::deallocator_type, GenDeleter<T>> ) {
                p->set_deallocator(GenDeleter<T>(this_type::Deallocate, this));
            }else{
                p->set_deallocator(*this);
            }

            return p;
        }
        return nullptr;
    }

    template<class... Args>
    std::shared_ptr<T> create_shared(Args&&... args)
    {
        if( auto p = allocate() ) {
            new (p) T(std::forward<Args>(args)...);
#ifdef USE_DELETERADAPTOR
            return {p, DeleterAdaptor<T>(this)};
#else
            return {p, [this](T* pObj){ deallocate(pObj);} };
#endif
        }
        return nullptr;
    }

    template<class... Args>
    typename T::Ptr create_unique(Args&&... args)
    {
        if( auto p = allocate() ) {
            new (p) T(std::forward<Args>(args)...);
#ifdef USE_DELETERADAPTOR
            return {p, DeleterAdaptor<T>(this)};
#else
            return {p,[this](T* pObj){ deallocate(pObj);} };
#endif
        }
        return nullptr;
    }

    void deallocate(T* p) override
    {
        assert(p);
        freeList.push(*p);
    }

    void operator()(T* p) override
    {
        deallocate(p);
    }

    static void Deallocate( T *p, void *&d)
    {
        static_cast<this_type*>(d)->freeList.push(*p);
    }

    void clear() // reset
    {
        freeList.clear();
    }

protected:
    FreeList freeList;
};

// allocate when custructing FreePool
template<typename T, class Alloc = std::allocator<T> , class FreeList = FreeList<T, null_delete<T>> >
class FixedPool : BasePool<T, FreeList, FixedPool<T, Alloc,FreeList> >
{
public:
    using this_type = FixedPool;
    using parent_type = BasePool<T, FreeList, this_type >;

    using parent_type::create;
    using parent_type::create_unique;
    using parent_type::create_shared;

    // pre-allocate memory. memory will be deleted when FreePool destructs.
    // T must be IDeallocatable
    FixedPool(size_t nBlocks, const Alloc &a=Alloc(), const FreeList& freeList=FreeList())
        : parent_type(freeList), mAlloc(a), mBlocks(nBlocks)
    {
        if( !nBlocks ) return;
        mCurrent = mMem = mAlloc.allocate(mBlocks);
        assert(mMem);
        if(!mMem) {
            throw std::bad_alloc();
        }
    }

    ~FixedPool() 
    {
        if(mMem) {
            mAlloc.deallocate( mMem, mBlocks );
            mMem = nullptr;
        }
    }

    T* do_alloc()
    {
        if( mCurrent + 1 > mMem + mBlocks ) return nullptr;
        auto res = (mCurrent);
        mCurrent += 1;
        return res;
    }

    void clear() // reset
    {
        parent_type::clear();
        mCurrent = mMem;
    }

protected:
    Alloc mAlloc;
    const size_t mBlocks;
    T *mMem = nullptr;
    T *mCurrent;
};

struct ConstGrowthStrategy
{
    ConstGrowthStrategy(size_t initial) :value(initial){}
    size_t next_value(){
        return value;
    }
    const size_t value;
};

struct DoubleGrowthStrategy
{
    DoubleGrowthStrategy(size_t initial) :value(initial){}
    size_t next_value(){
        value *= 2;
        return value;
    }
    size_t value;
};

// allocate when custructing FreePool
template<typename T, class ChunkSizeGrowthStrategy = ConstGrowthStrategy, class Alloc = std::allocator<T> , class FreeList = FreeList<T, null_delete<T>> >
class ChunkPool : BasePool<T, FreeList, ChunkPool<T, ChunkSizeGrowthStrategy, Alloc,FreeList> >
{
public:
    using this_type = ChunkPool;
    using parent_type = BasePool<T, FreeList, this_type >;

    using parent_type::create;
    using parent_type::create_unique;
    using parent_type::create_shared;

    static constexpr bool is_const_growth = std::is_same_v<ChunkSizeGrowthStrategy, ConstGrowthStrategy>;

    // pre-allocate memory. memory will be deleted when FreePool destructs.
    // T must be IDeallocatable
    ChunkPool(size_t chunckSize = 16, size_t initialChunks = 0, const Alloc &a=Alloc(), const FreeList& freeList=FreeList())
        : parent_type(freeList), mAlloc(a), mChuckSize(chunckSize), mCurrChunk(0)
    {
        for(size_t i=0;i< initialChunks; ++i )
            append_chunk();
    }

    ~ChunkPool() 
    {
        for( auto& ch: mChunks ) {
            if constexpr( is_const_growth )
                mAlloc.deallocate( ch.mMem, mChuckSize.next_value() );
            else
                mAlloc.deallocate( ch.mMem, ch.mSize );
            ch.clear();
        }
    }

    T* do_alloc()
    {
        while(true) {
            if( mCurrChunk >= mChunks.size() ) {
                if( auto pChunk = append_chunk( ) ) {
                if constexpr( is_const_growth ) 
                    return pChunk->do_alloc(mChuckSize.next_value());
                else
                    return pChunk->do_alloc();
                }else
                    return nullptr;
            } else {
                auto& ch = mChunks[mCurrChunk];
                if constexpr( is_const_growth ) {
                    if( auto p = ch.do_alloc(mChuckSize.next_value()) )
                        return p;
                }else{
                    if( auto p = ch.do_alloc() )
                        return p;
                }

                ++mCurrChunk;
            }
        }
    }

    void clear() // reset/free allocated objects
    {
        parent_type::clear();
        for( auto& ch: mChunks ) {
            ch.clear();
        }
        mCurrChunk = 0;
    }

protected:

    struct DynSizedChunkInfo{
        size_t mSize = 0;
        T *mMem = nullptr;
        T *mCurrent = nullptr;

        DynSizedChunkInfo(size_t n = 0): mSize(n){}

        void clear(){
            mCurrent = mMem;
        }
        T* do_alloc()
        {
            if( mCurrent + 1 > mMem + mSize ) return nullptr;
            auto res = (mCurrent);
            mCurrent += 1;
            return res;
        }
    };

    struct ConstSizedChunkInfo{
        T *mMem = nullptr;
        T *mCurrent = nullptr;

        ConstSizedChunkInfo(size_t ){}

        void clear(){
            mCurrent = mMem;
        }
        T* do_alloc(size_t mSize)
        {
            if( mCurrent + 1 > mMem + mSize ) return nullptr;
            auto res = (mCurrent);
            mCurrent += 1;
            return res;
        }
    };
    using ChunkInfo = std::conditional_t<is_const_growth, ConstSizedChunkInfo, DynSizedChunkInfo>;
    

    ChunkInfo *append_chunk() {
            size_t n = mChuckSize.next_value();
            ChunkInfo ch { n };
            ch.mCurrent = ch.mMem = mAlloc.allocate( n );
            if( !ch.mMem )
                return nullptr;
            return &mChunks.emplace_back(ch);
    }
    using ChunkList = std::vector<ChunkInfo>;
    ChunkSizeGrowthStrategy mChuckSize;
    Alloc mAlloc;
    ChunkList mChunks;
    size_t mCurrChunk;
};

// allocate when needed
template<typename T, class Alloc = std::allocator<T> , class FreeList = FreeList<T, std::default_delete<T>> >
class PoolAllocator  : BasePool<T, FreeList, PoolAllocator<T, Alloc, FreeList> >
{
public:
    using this_type = PoolAllocator;
    using parent_type = BasePool<T, FreeList, this_type >;

    using parent_type::create;
    using parent_type::create_unique;
    using parent_type::create_shared;


    // pre-allocate memory. memory will be deleted when FreePool destructs.
    // T must be IDeallocatable
    PoolAllocator( const Alloc &a=Alloc(), const FreeList& freeList=FreeList())
        : parent_type(freeList), mAlloc(a)
    {
    }

    T* do_alloc()
    {
        return mAlloc.allocate(1);
    }

    void clear() // reset
    {
        parent_type::clear();
    }

protected:
    Alloc mAlloc;
};

} // namespace ftl
