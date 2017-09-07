#ifndef _CONCURRENT_QUEUE_ADAPTER_H
#define _CONCURRENT_QUEUE_ADAPTER_H

#ifdef QUEUE_MOODYCAMEL

#include "concurrent_queue.h"
#include "blocking_concurrent_queue.h"

template<typename T>
class lockfree_queue: moodycamel::ConcurrentQueue<T>
{
public:
    using SuperType = moodycamel::ConcurrentQueue<T>;
    using ThisType = lockfree_queue<T>;

public:
    lockfree_queue( size_t cap ): SuperType( cap )
    {}
    bool empty() const {
        return SuperType::size_approx() == 0;
    }
    size_t size() const {
        return SuperType::size_approx();
    }

    bool try_push( const T& v, size_t ) {
        return SuperType::try_enqueue( v );
    }
    bool try_pop( T& v, size_t = 0) {
        return SuperType::try_dequeue( v );
    }
};

template<typename T>
class blocking_queue: moodycamel::BlockingConcurrentQueue<T>
{
public:
    using SuperType = moodycamel::BlockingConcurrentQueue<T>;
    using ThisType = blocking_queue<T>;

public:
    blocking_queue( size_t cap ): SuperType( cap )
    {}


    bool empty() const {
        return SuperType::size_approx() == 0;
    }
    size_t size() const {
        return SuperType::size_approx();
    }

    bool try_push( const T& v, std::size_t ) {
        return SuperType::try_enqueue( v );
    }
    bool try_pop( T& v, std::size_t usec = 0) {
        return SuperType::wait_dequeue_timed( v, usec );
    }
};

#endif // QUEUE_MOODYCAMEL

#ifdef QUEUE_OPAQUE

#include "opaque/sync/mpsc_array_queue.h"
#include "opaque/sync/blocking_mpsc_array_queue.h"

template<typename T>
class lockfree_queue
{
public:
    using SuperType = opq::sync::MpscArrayQueue<T, 1024>;
    using ThisType = lockfree_queue<T>;

public:
    lockfree_queue( size_t ): q(new SuperType()), siz(0)
    {}
    bool empty() const {
        return siz == 0;
    }
    size_t size() const {
        return siz;
    }

    bool try_push( const T& v, size_t = 0 ) {
        if( q->try_enqueue( v ) ) {
            ++siz;
            return true;
        }
        return false;
    }
    bool try_pop( T& v, size_t = 0 ) {
        if( q->try_dequeue( v ) ) {
            --siz;
            return true;
        }
        return false;
    }
protected:
    std::unique_ptr<SuperType> q;
    std::atomic<size_t> siz;
};

template<typename T>
class blocking_queue
{
public:
    using SuperType = opq::sync::BlockingMpscArrayQueue<T, 8096>;  // NOTE MPSC, not MPMC queue
    using ThisType = blocking_queue<T>;

    using MicroSec = std::chrono::duration<size_t, std::micro>;

public:
    blocking_queue( size_t ): q( new SuperType()), siz(0)
    {
    }

    bool empty() const {
        return siz == 0;
    }
    size_t size() const {
        return siz;
    }

    bool try_push( const T& v, std::size_t usec = 0 ) {
        if( q->try_enqueue_for( MicroSec(usec), v ) ) {
            ++siz;
            return true;
        }
        return false;
    }
    bool try_pop( T& v, std::size_t usec = 0 ) {
        if( q->try_dequeue_for( MicroSec(usec), v ) ) {
            --siz;
            return true;
        }
        return false;
    }
protected:
    std::unique_ptr<SuperType> q;
    std::atomic<size_t> siz;
};

#endif // QUEUE_OPAQUE

#ifdef QUEUE_TBB

#include "tbb/concurrent_queue.h"

// MPMC queue
template<typename T>
class blocking_queue: public tbb::concurrent_bounded_queue<T>
{
public:
    using SuperType = tbb::concurrent_bounded_queue<T>;
    using ThisType = blocking_queue<T>;

public:
    blocking_queue( size_t s )
    {
        this->set_capacity( s );
    }
};

#endif // QUEUE_TBB


#endif // _CONCURRENT_QUEUE_ADAPTER_H
