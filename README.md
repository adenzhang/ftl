# C++ Fast Template Library (ftl)

### C++ Template Concepts (stdconcept.h)
std extentions and helper classes/functions
- std::hash specilization for std::pair and std::tuple
-  FTL_CHECK_EXPR: define template class to check expressions/types
-  tuple helpers: tuple_foreach, tuple_enumerate, tuple_reduce, tuple_has_type
-  variant helpers: variant_has_type
-  Others: GetTypeName, array_size, overload, Identity

### Container Serialization (container_serialization.h)
- StrLiteral operator""_sl (todo: move to string utils)
- scoped_stream_redirect (todo: move to string utils)
- operator>>, operator<< for containers: list, map, vector
- 
### Generic Iterator (gen_iterator.h)
class gen_iterator used for containers which only have to implate required functions

### Macro CREATE_ENUM (enum_create.h)
enum creator to provide enum serialization/deserialization.

### Intrusive List (intrusive_list.h)
intrusive_singly_list: users should provide memeber variable

### Shared Object Pool (shared_object_pool.h)
An object pool, the life time of which is managed by objects it created. The pool will be automatically destroyed when all objects are destroyed.

### Free Pool (free_pool.h)
Different object pools.
- PoolAllocator: allocate objects when needed.
- ChunkPool: the growth strategy is parameterized.
- FixedPool: fixed sized object pool.

### Object Pool (object_pool.h)
- ObjectPool

### Thread Pool (thread_pool.h)
- ThreadPool: all threads share a concurrent fix-size message queue
- ThreadArray: each thread has its own fix-size message queue

### circular_queue (circular_queue.h)
- circular_queue
- inline_circular_queue
### Binary Tree for test (binary_tree.h)

### Try Catch (try_catch.h)
Catch and recover from some signal events.

### Lockfree Queues
- spsc_queue.h : ring buffer
- mpsc_bounded_queue.h: fix-sized array queue
- mpsc_unbunded_queue.h: node based singly list queue

### Out Printer (out_printer.h)
helper class OutPrinter helps to print multiple variables, containers.

## ------- Advanced Data Structures / Algorithms  -----------
### Indexed Tree (indexed_tree.h)
indexed_tree (todo: 2D indexed tree)
### KMP (kmp_search.h)

