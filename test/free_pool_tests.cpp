#include "ftl/catch.hpp"
#include "ftl/free_pool.h"

#include <cassert>
#include <chrono>
#include <iostream>

// #define ALLOC_SHARED
//#define ALLOC_UNIQUE
// else use intrusive deleter

//#define USE_FIXEDPOOL
#define USE_CHUNKPOOL

// fro shared_ptr or unique_ptr
#define USE_DELETERADAPTOR

//
const size_t LINE_SIZE = 64;

using namespace ftl;

//struct Node : IDeallocatable<Node>                                    // call IDeallocator::deallocate(T*)
struct Node : IDeletableFn<Node, IDeleter<Node>, RefDeallocator_Tag> // IDeallocator::operator()(T*)
//struct Node : IDeletableFn<Node, GenDeleter<Node>, CopyDeallocator_Tag >
{

#ifdef ALLOC_SHARED
    using Ptr = std::shared_ptr<Node>;
#elif defined(ALLOC_UNIQUE)

#ifdef USE_DELETERADAPTOR
    using Ptr = std::unique_ptr<Node, DeleterAdaptor<Node>>;
#else
    using Ptr = std::unique_ptr<Node, DeleteFunc<Node>>;
#endif

#else
    using Ptr = Node*;
#endif

    Ptr l = Ptr(), r = Ptr();

    int check() const
    {
        if (l)
            return l->check() + 1 + r->check();
        else
            return 1;
    }
};

using NodePtr = typename Node::Ptr;

#ifdef USE_FIXEDPOOL
typedef FixedPool<Node> NodePool;
#elif defined(USE_CHUNKPOOL)
typedef ChunkPool<Node, ConstGrowthStrategy> NodePool;
#else
typedef PoolAllocator<Node> NodePool;
#endif

//typedef Pool<Node> NodePool;

NodePtr make(int d, NodePool& store)
{
#ifdef ALLOC_SHARED
    auto root = store.create_shared();
#elif defined(ALLOC_UNIQUE)
    auto root = store.create_unique();
#else
    auto root = store.create();
#endif
    assert(root);

    if (d > 0) {
        root->l = make(d - 1, store);
        root->r = make(d - 1, store);
    } else {
        //        root->l=root->r=0;
    }

    return std::move(root);
}

#define CalcNodes(nodes) ((1 << (nodes) + 1) - 1)

static int main_test(int argc, const char* argv[])
{
    auto tsStart = std::chrono::steady_clock::now();
    //    Apr apr;
    int min_depth = 4;
    int max_depth = std::max(min_depth,
        (argc == 2 ? atoi(argv[1]) : 10));
    int stretch_depth = max_depth + 1;

    //    assert( stretch_depth < 0);
    assert(stretch_depth < 32);
    const size_t MaxNodes = CalcNodes(stretch_depth); // nodes: 2**(depth+1)-1
    //    std::cout << "allocated :" << MemSize << " nodes:" << MaxNodes << " blocksize:" << sizeof(Node) << " stretch_depth:" << stretch_depth << std::endl;
    // Alloc then dealloc stretchdepth tree
    {
#if defined(USE_FIXEDPOOL)
        NodePool store(MaxNodes);
#elif defined(USE_CHUNKPOOL)
        NodePool store(1024);
#else
        NodePool store;
#endif
        auto c = make(stretch_depth, store);
        std::cout << "stretch tree of depth " << stretch_depth << "\t "
                  << "check: " << c->check() << std::endl;
    }

#if defined(USE_FIXEDPOOL)
    NodePool long_lived_store(MaxNodes);
#elif defined(USE_CHUNKPOOL)
    NodePool long_lived_store(1024);
#else
    NodePool long_lived_store;
#endif

    std::cout << "starting..." << std::endl;
    auto long_lived_tree = make(max_depth, long_lived_store);

    // buffer to store output of each thread
    char* outputstr = (char*)malloc(LINE_SIZE * (max_depth + 1) * sizeof(char));

    //    static FreeList<Node> *s_freeList;
    //    #pragma omp threadprivate(s_freeList)
    //    #pragma omp parallel
    //    {
    //        s_freeList = new FreeList<Node>;
    //    }

#pragma omp parallel for
    for (int d = min_depth; d <= max_depth; d += 2) {
        //        auto & freeList= *s_freeList;
        int iterations = 1 << (max_depth - d + min_depth);
        int c = 0;

        // Create a memory pool for this thread to use.
#if defined(USE_FIXEDPOOL)
        NodePool store(CalcNodes(d));
#elif defined(USE_CHUNKPOOL)
        NodePool store(1024);
#else
        NodePool store;
#endif
        //        NodePool store(0, {}, freeList);

        for (int i = 1; i <= iterations; ++i) {
            {
                auto a = make(d, store);
                c += a->check();
            }
            store.clear();
        }

        // each thread write to separate location
        sprintf(outputstr + LINE_SIZE * d, "%d\t trees of depth %d\t check: %d\n", iterations, d, c);
    }

    // print all results
    for (int d = min_depth; d <= max_depth; d += 2)
        printf("%s", outputstr + (d * LINE_SIZE));
    free(outputstr);

    std::cout << "long lived tree of depth " << max_depth << "\t "
              << "check: " << (long_lived_tree->check()) << "\n";

    auto tsStop = std::chrono::steady_clock::now();
    std::cout << "- cpp time(ms): " << (tsStop - tsStart).count() / 1000 / 1000 << std::endl;
    return 0;
}
TEST_CASE("free_pool", "asdf")
{
    const char* argv[] = { "main", "5" };
    main_test(2, argv);
}
