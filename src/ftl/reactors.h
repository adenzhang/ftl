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
#include <atomic>
#include <cassert>
#include <functional>
#include <iostream>
#include <mutex>
#include <new>
#include <sstream>
#include <thread>
#include <unordered_set>
#include <vector>

#define QUEUE_FTL
//#define QUEUE_MOODYCAMEL
//#define QUEUE_OPAQUE
//#define QUEUE_TBB

//////////////////////////////////////////////////

// TBB doesn't support lockfree queue
#ifdef QUEUE_TBB
#undef USE_LOCKFREE_QUEUE
#endif

// FTL doesn't support blocking queue
#ifdef QUEUE_FTL
#define USE_LOCKFREE_QUEUE
#endif

#define USE_LOCKFREE_QUEUE
//////////////////////////////////////////////////

#include "concurrent_queue_adapter.h"

//#define _REACTOR_DBG
namespace ftl
{
namespace reactor
{

    struct IEventSenderBase
    {
        // if not async, block untill all pending events being processed
        // return true if successfully stopped
        virtual bool Stop( bool async = false ) = 0;

        virtual ~IEventSenderBase()
        {
        }
    };

    template<class EventT>
    using EventHandlerFunc = std::function<void( EventT && )>; // todo: inline size

    using ThreadTask = std::function<void()>; // todo: inline size

    template<typename EventType>
    struct IEventSender : public IEventSenderBase
    {
        using Handler = EventHandlerFunc<EventType>;

        // block until putting event to queue
        // return false if failure, eg. it's already stopped. Do NOT retry when it fails.
        virtual bool Send( EventType && ) = 0;

        // todo
        // filter when Send(). if ! filter(), Send() will return false and event will not be put in queue.
        //    virtual void SetFilter( const std::function<bool (const EventType&)> filter ) = 0;

        // return approximate number of pending events
        virtual size_t CountEvents() const = 0;

        virtual ~IEventSender()
        {
        }

        virtual IEventSender<EventType> &SetName( const char *name )
        {
            this->name = name;
            return *this;
        }
        virtual const char *GetName() const
        {
            return name.c_str();
        }

    protected:
        std::string name;
    };

    struct IReactorsBase
    {
        // @numThreads: ignored for SeparatedReactors.
        // @tasksCapacity: For PooledReactors, max number of pending tasks, approximately the sum of all handlers' pending events.
        //                For GroupedReactors, tasks capacity of a thread/group.
        //                Ignored for SeparatedReactors
        virtual bool Init( size_t numThreads, size_t tasksCapacity ) = 0;

        // block until all handlers are removed and threads are closed
        virtual void Term() = 0;

        // Inherited classes implements this generic function to register handlers for different EventType
        // @handler: handles incoming messages of EventType.
        // @eventsCapacity: max number of pending events in queue. Ignored for GroupedReactors.
        // @return an IEventSender which sends events to the handler
        //    virtual IEventSender<EventType>* RegisterHandler( const EventHandlerFunc<EventType>& handler, size_t eventsCapacity ) = 0;

        // IEventSender will be deleted when handler is removed.
        virtual void RemoveHandler( IEventSenderBase * ) = 0;

        // Used only by PooledReactor ot push a thread task.
        virtual void TryPush( const ThreadTask & )
        {
        }

        virtual ~IReactorsBase()
        {
        }
    };
    using ReactorsBasePtr = std::shared_ptr<IReactorsBase>;

    //////////////////////////////////////////////////////////
    ///  PooledReactors (Reactors shared a thread pool)
    ///
    /// all senders share the ownership of PooledReactors.

    template<typename EventType>
    class PooledEventSender : public IEventSender<EventType>
    {
    public:
        class PooledReactors;
        friend class PooledReactors;
        class PooledReactorsPtr;

    protected:
        ReactorsBasePtr dispatcher;
        typename IEventSender<EventType>::Handler handler;
#ifdef QUEUE_TBB
        blocking_queue<EventType> events; // MPSC queue
#else
        //    blocking_queue<EventType> events;  // MPSC queue
        lockfree_queue<EventType> events; // lockfree MPSC queue
#endif
        std::mutex working;
        std::atomic<bool> stopping;
        std::atomic<size_t> currToken, processedTasks, pendingTasks;

    public:
        // should not be called directly, use PooledEventDispatcher::RegisterHandler() to create it.
        PooledEventSender( ReactorsBasePtr d, const typename IEventSender<EventType>::Handler &h, size_t siz )
            : dispatcher( d ), handler( h ), events( siz ), stopping( false ), currToken( 0 ), processedTasks( 0 ), pendingTasks( 0 )
        {
        }

        template<typename... Args>
        bool Send( Args... e )
        {
            return Send( EventType( e... ) );
        }

        bool Send( EventType &&e ) override
        {
            if ( stopping )
                return false;
            std::stringstream ss;
#ifdef QUEUE_TBB
            try
            {
                events.push( e );
            }
            catch ( tbb::user_abort & )
            {
                return false;
            }

#else
            while ( !events.try_push( e, 1000000 ) )
            {
#ifdef _REACTOR_DBG
                ss << name << " failed to push event " << e;
                std::cout << ss.str() << std::endl;
                ss.str( "" );
#endif
            }
#endif
#ifdef _REACTOR_DBG
            ss << name << " pushed event " << e;
            std::cout << ss.str() << std::endl;
            ss.str( "" );
#endif

            auto token = ++currToken;
            ++pendingTasks;
            dispatcher->TryPush( [this, token]() { Process( token ); } );

            return true;
        }

        bool Stop( bool async = false ) override
        {
            stopping = true;
#ifdef QUEUE_TBB
            events.abort();
#endif
            while ( true )
            {
                if ( stopping && events.empty() && processedTasks == pendingTasks && working.try_lock() )
                {
                    working.unlock();
                    return true; // stopped
                }
                if ( async )
                    return false;
            }
        }

        size_t CountEvents() const override
        {
            return events.size();
        }

    protected:
        void Process( size_t token )
        {
            std::stringstream ss;
            if ( events.empty() )
            {
#ifdef _REACTOR_DBG
                ss << name << " empty token " << token;
                std::cout << ss.str() << std::endl;
#endif
            }
            else if ( working.try_lock() )
            {
                // drain the events. It may block the thread for a while, but avoid unnecessary rescheduling.
                EventType e;
                while ( events.try_pop( e ) )
                {
                    handler( std::move( e ) );
                }
                working.unlock();
            }
            else if ( token == currToken )
            {
                // if this is current last event and there's a working task, reschedule to ensure it being processed.
#ifdef _REACTOR_DBG
                ss << name << " reschedule token " << token;
                std::cout << ss.str() << std::endl;
#endif
                ++pendingTasks;
                dispatcher->TryPush( [this, token]() { this->Process( token ); } );
            }
            else
            {
                // no reschedule. it will be processed by subsequent task or current working task.
#ifdef _REACTOR_DBG
                ss << name << " will process token " << token << ":" << currToken;
                std::cout << ss.str() << std::endl;
#endif
            }
            ++processedTasks;
        }
    };

    class PooledReactors : public std::enable_shared_from_this<PooledReactors>, public IReactorsBase
    {
        template<typename EventType>
        friend class PooledEventSender;

    public:
        using ThisType = PooledReactors;
        using SuperType = IReactorsBase;
        using SuperSharedType = std::enable_shared_from_this<ThisType>;
        using ReactorsPtr = std::shared_ptr<ThisType>;

    protected:
        using HandlerSet = std::unordered_set<IEventSenderBase *>;

        HandlerSet _handlers;
        std::mutex _handlersMutex;

        using TaskQ = blocking_queue<ThreadTask>;

        std::unique_ptr<TaskQ> _tasks; // blocking MPMC queue
        std::vector<std::thread> _threads;
        std::atomic<bool> _stopping;

    public:
        //// event dispatcher interface
        bool Init( size_t numThreads, size_t tasksCapacity ) override
        {
            if ( !numThreads || !tasksCapacity )
            {
                return false;
            }
            _stopping = false;
            _threads.reserve( numThreads );
            _tasks.reset( new TaskQ( tasksCapacity ) );

            for ( size_t i = 0; i < numThreads; ++i )
            {
                _threads.push_back( std::thread( [&]() {
                    while ( !_stopping )
                    {
                        ThreadTask t;
#ifdef QUEUE_TBB
                        try
                        {
                            _tasks->pop( t );
                            t();
                        }
                        catch ( tbb::user_abort & )
                        {
                            while ( _tasks->try_pop( t ) )
                                t();
#ifdef _REACTOR_DBG
                            std::cout << " user aborted\n";
#endif
                        }
#else
                        if ( _tasks->try_pop( t, 2000 ) ) // usec
                            t();
#endif
                    }
                    assert( _tasks->empty() && "No tasks after stopping work!" );
                } ) );
            }
            return true;
        }

        void Term() override
        {
            _stopping = true;
            if ( _threads.empty() )
                return;
            do
            { // signal stop
                std::lock_guard<std::mutex> guard( _handlersMutex );
                for ( auto e : _handlers )
                    e->Stop( true );
            } while ( false );

            //        std::cout << " wait until no tasks " << _tasks->size() << std::endl;
            while ( !_tasks->empty() )
                ;
            //        std::cout << " removing hanlders " << _handlers.size() << std::endl;
            while ( !_handlers.empty() )
                RemoveHandler( *_handlers.begin() );
#ifdef QUEUE_TBB
            _tasks->abort();
#endif
            //        std::cout << " joining threads\n";
            for ( auto &th : _threads )
                th.join();
            _tasks.reset();
            _threads.clear();
        }

        template<typename EventType>
        PooledEventSender<EventType> *RegisterHandler( const EventHandlerFunc<EventType> &handler, size_t eventsCapacity = 256 )
        {
            if ( _stopping )
                return nullptr;
            std::lock_guard<std::mutex> guard( _handlersMutex );
            auto p = std::unique_ptr<PooledEventSender<EventType>>(
                    new PooledEventSender<EventType>( std::dynamic_pointer_cast<SuperType>( GetThisPtr() ), handler, eventsCapacity ) );
            return static_cast<PooledEventSender<EventType> *>( *_handlers.emplace( p.release() ).first );
        }

        // block untill all pending messages being processed
        void RemoveHandler( IEventSenderBase *sender ) override
        {
            do
            { // do not hold mutext until stopped
                std::lock_guard<std::mutex> guard( _handlersMutex );
                auto it = _handlers.find( sender );
                if ( it == _handlers.end() )
                    return;
                _handlers.erase( it );
                if ( sender->Stop( true ) )
                {
                    delete sender;
                    return;
                }
            } while ( false );
            sender->Stop( false );
            delete sender;
        }

        ReactorsPtr GetThisPtr()
        {
            return SuperSharedType::shared_from_this();
        }

        static ReactorsPtr New()
        {
            return std::make_shared<ThisType>();
        }

        // should not be called directly, use New() instead
        PooledReactors() : _stopping( false )
        {
        }

        ~PooledReactors()
        {
            Term();
        }

        //// thread pool interface
        //

        void TryPush( const ThreadTask &task ) override
        {

            //        while( !_stopping )  // alwasy push even stopping
            {
#ifdef QUEUE_TBB
                _tasks->push( task );
#else
                for ( size_t i = 0; !_tasks->try_push( task, 1000000 ); )
                    ++i;
#ifdef _REACTOR_DBG
                std::stringstream ss;
                ss << " Failed to push task, size:" << _tasks->size();
                std::cout << ss.str() << std::endl;
#endif
#endif
            }
        }

    protected:
        PooledReactors( const PooledReactors & ) = delete;
        PooledReactors &operator=( const PooledReactors & ) = delete;
    };

    using PooledReactorsPtr = std::shared_ptr<PooledReactors>;

    //////////////////////////////////////////////////////////
    ///  SeparatedReactors (Each reactor is assigned a thread to handle event, shareing nothing with each other)
    ///

    template<typename EventType>
    class SeparatedEventSender : public IEventSender<EventType>
    {
    protected:
        typename IEventSender<EventType>::Handler handler;
#ifdef QUEUE_TBB
        blocking_queue<EventType> events; // SPSC queue
#else
#ifdef USE_LOCKFREE_QUEUE
        lockfree_queue<EventType> events; // lockfree MPSC queue
#else
        blocking_queue<EventType> events; // SPSC queue
#endif
#endif
        std::atomic<bool> stopping;
        std::unique_ptr<std::thread> th;

    public:
        SeparatedEventSender( const typename IEventSender<EventType>::Handler &h, size_t siz ) : handler( h ), events( siz ), stopping( false )
        {
            th.reset( new std::thread( [this] {
                while ( !stopping )
                {
                    EventType e;
#ifdef QUEUE_TBB
                    try
                    {
                        events.pop( e );
                        handler( e );
                    }
                    catch ( tbb::user_abort & )
                    {
                        while ( events.try_pop( e ) )
                            handler( e );
                    }
#else
                    if ( events.try_pop( e, 1000 ) )
                    {
                        handler( e );
                    }
#endif
                }
                assert( events.empty() && "User aborted!" );
            } ) );
        }

        template<typename... Args>
        bool Send( Args... e )
        {
            return Send( EventType( e... ) );
        }

        bool Send( const EventType &e ) override
        {
            if ( stopping )
                return false;
#ifdef QUEUE_TBB
            try
            {
                events.push( e );
            }
            catch ( tbb::user_abort & )
            {
                return false;
            }

#else
            while ( !events.try_push( e, 500 ) )
            {
                ;
            }
#endif
            return true;
        }

        bool Stop( bool async = false ) override
        {
            stopping = true;
#ifdef QUEUE_TBB
            ++*reinterpret_cast<char *>( &async );
            events.abort();
            th->join();
            return true; // always sync if tbb
#else
            while ( true )
            {
                if ( events.empty() )
                {
                    th->join();
                    return true; // stopped
                }
                if ( async )
                    return false;
            }
#endif
        }

        size_t CountEvents() const override
        {
            return events.size();
        }
    };

    class SeparatedReactors : public std::enable_shared_from_this<SeparatedReactors>, public IReactorsBase
    {
        template<typename EventType>
        friend class EventSender;

    public:
        using ThisType = SeparatedReactors;
        using SuperType = IReactorsBase;
        using SuperSharedType = std::enable_shared_from_this<SeparatedReactors>;
        using ReactorsPtr = std::shared_ptr<ThisType>;

        using HandlerSet = std::unordered_set<IEventSenderBase *>;

    protected:
        std::atomic<size_t> _stopping;
        HandlerSet _handlers;
        std::mutex _handlersMutex;

    public:
        // threads will be created when handler/sender is created
        bool Init( size_t, size_t ) override
        {
            return true;
        }

        void Term() override
        {
            _stopping = true;
            while ( !_handlers.empty() )
                RemoveHandler( *_handlers.begin() );
        }

        template<typename EventType>
        SeparatedEventSender<EventType> *RegisterHandler( const EventHandlerFunc<EventType> &handler, size_t eventsCapacity = 32 )
        {
            if ( _stopping )
                return nullptr;
            std::lock_guard<std::mutex> guard( _handlersMutex );
            auto p = std::unique_ptr<SeparatedEventSender<EventType>>( new SeparatedEventSender<EventType>( handler, eventsCapacity ) );
            return static_cast<SeparatedEventSender<EventType> *>( *_handlers.emplace( p.release() ).first );
        }

        // block untill all pending messages being processed
        void RemoveHandler( IEventSenderBase *sender ) override
        {
            do
            { // do not hold mutext until stopped
                std::lock_guard<std::mutex> guard( _handlersMutex );
                auto it = _handlers.find( sender );
                if ( it == _handlers.end() )
                    return;
                _handlers.erase( it );
                if ( sender->Stop( true ) )
                {
                    delete sender;
                    return;
                }
            } while ( false );
            sender->Stop( false );
            delete sender;
        }

        ReactorsPtr GetThisPtr()
        {
            return SuperSharedType::shared_from_this();
        }

        static ReactorsPtr New()
        {
            return std::make_shared<ThisType>();
        }

        // should not be called directly, use New() instead
        SeparatedReactors() : _stopping( false )
        {
        }

        ~SeparatedReactors()
        {
            Term();
        }
    };

    using SeparatedReactorsPtr = std::shared_ptr<SeparatedReactors>;

    //////////////////////////////////////////////////////////
    ///  GroupedReactors (Reactors within a group share a single thread)
    ///
    /// all senders share the ownership of dispatcher.

    template<typename EventType>
    class GroupedEventSender : public IEventSender<EventType>
    {
    public:
        class GroupedReactors;
        friend class GroupedReactors;
        class GroupedReactorsPtr;

    protected:
        typename IEventSender<EventType>::Handler handler;
#ifdef QUEUE_TBB
        using QueueType = blocking_queue<Task>; // MPSC queue
#else
#ifdef USE_LOCKFREE_QUEUE
        using QueueType = lockfree_queue<ThreadTask>; // lockfree MPSC queue
#else
        using QueueType = blocking_queue<Task>; // MPSC queue
#endif
#endif
        QueueType &events;
        std::mutex working;
        std::atomic<bool> stopping;
        std::atomic<size_t> currToken;

    public:
        // should not be called directly, use PooledEventDispatcher::RegisterHandler() to create it.
        GroupedEventSender( const typename IEventSender<EventType>::Handler &h, QueueType &events )
            : handler( h ), events( events ), stopping( false ), currToken( 0 )
        {
        }

        template<typename... Args>
        bool Send( Args... e )
        {
            return Send( EventType( e... ) );
        }

        bool Send( EventType &&e ) override
        {
            if ( stopping )
                return false;
            std::stringstream ss;
            auto task = [e = std::move( e ), this] {
                handler( std::move( const_cast<EventType &>( e ) ) );
                --currToken;
            };
#ifdef QUEUE_TBB
            try
            {
                events.push( std::move( task ) );
            }
            catch ( tbb::user_abort & )
            {
                return false;
            }

#else
            while ( !events.try_push( std::move( task ), 1000000 ) )
            {
#ifdef _REACTOR_DBG
                ss << name << " failed to push event " << e;
                std::cout << ss.str() << std::endl;
                ss.str( "" );
#endif
            }
#endif
#ifdef _REACTOR_DBG
            ss << name << " pushed event " << e;
            std::cout << ss.str() << std::endl;
            ss.str( "" );
#endif

            ++currToken;
            return true;
        }

        bool Stop( bool async = false ) override
        {
            stopping = true;
            if ( !async )
                while ( CountEvents() )
                    ;
            return currToken == 0; // not accurate!
        }

        size_t CountEvents() const override
        {
            return currToken;
        }
    };

    class GroupedReactors : public std::enable_shared_from_this<GroupedReactors>, public IReactorsBase
    {
    public:
        using ThisType = GroupedReactors;
        using SuperType = IReactorsBase;
        using SuperSharedType = std::enable_shared_from_this<ThisType>;
        using ReactorsPtr = std::shared_ptr<ThisType>;

    protected:
        using HandlerSet = std::unordered_set<IEventSenderBase *>;

        std::vector<HandlerSet> _handlers;
        std::mutex _handlersMutex;

#ifdef USE_LOCKFREE_QUEUE
        using TaskQ = lockfree_queue<ThreadTask>; // lockfree MPSC queue
#else
        using TaskQ = blocking_queue<Task>; // blocking MPSC queue
#endif

        std::vector<std::unique_ptr<TaskQ>> _tasksQ;
        std::vector<std::thread> _threads;
        std::atomic<bool> _stopping;

    public:
        // tasksCapacity per therad
        bool Init( size_t numThreads, size_t tasksCapacity ) override
        {
            if ( !numThreads || !tasksCapacity )
            {
                return false;
            }
            _stopping = false;
            _threads.reserve( numThreads );
            _tasksQ.reserve( numThreads );
            _handlers.resize( numThreads );

            for ( size_t i = 0; i < numThreads; ++i )
            {
                _tasksQ.emplace_back( new TaskQ( tasksCapacity ) );
                _threads.emplace_back( [this, i]() {
                    auto &tasks = *_tasksQ[i];
                    while ( !_stopping )
                    {
                        ThreadTask t;
#ifdef QUEUE_TBB
                        try
                        {
                            tasks.pop( t );
                            t();
                        }
                        catch ( tbb::user_abort & )
                        {
#ifdef _REACTOR_DBG
                            std::cout << "aborted thread " << i << std::endl;
#endif
                            while ( tasks.try_pop( t ) )
                            {
                                t();
                            }
                        }
#else
                        if ( tasks.try_pop( t, 2000 ) ) // usec
                            t();
#endif
                    }
#ifdef _REACTOR_DBG
                    std::cout << "exiting thread " << i << std::endl;
#endif
                    assert( tasks.empty() && "No tasks after stopping work!" );
                } );
            }
            return true;
        }
        void Term() override
        {
            _stopping = true;
            if ( _threads.empty() )
                return;
            do
            { // signal stop
                std::lock_guard<std::mutex> guard( _handlersMutex );
                for ( auto &h : _handlers )
                    for ( auto e : h )
                        e->Stop( true );
            } while ( false );

            {
                std::lock_guard<std::mutex> guard( _handlersMutex );

                for ( size_t numTasks = 0; numTasks; )
                { // wait until no tasks in queue
                    numTasks = 0;
                    for ( auto &pTaskQ : _tasksQ )
                        numTasks += pTaskQ->size();
                }
                for ( auto &h : _handlers )
                    while ( !h.empty() )
                    {
                        auto p = *h.begin();
                        h.erase( h.begin() );
                        delete p;
                    }

//            std::cout << "aborting tasks" <<  std::endl;
#ifdef QUEUE_TBB
                for ( auto &pTaskQ : _tasksQ )
                {
                    pTaskQ->abort();
                }
#endif
                for ( auto &th : _threads )
                    th.join();
//            std::cout << "clearing threads" <<  std::endl;
#ifdef QUEUE_TBB
                _tasksQ.clear();
#endif
                _threads.clear();
            }
        }

        template<typename EventType>
        GroupedEventSender<EventType> *RegisterHandler( const EventHandlerFunc<EventType> &handler, size_t )
        {
            if ( _stopping )
                return nullptr;
            std::lock_guard<std::mutex> guard( _handlersMutex );
            // find a taskQ with least handlers
            size_t k = 0;
            for ( size_t i = 0; i < _tasksQ.size(); ++i )
            {
                if ( _handlers[i].size() < _handlers[k].size() )
                    k = i;
            }
            auto p = std::unique_ptr<GroupedEventSender<EventType>>( new GroupedEventSender<EventType>( handler, *_tasksQ[k] ) );
            return static_cast<GroupedEventSender<EventType> *>( *_handlers[k].emplace( p.release() ).first );
        }

        // block untill all pending messages being processed
        void RemoveHandler( IEventSenderBase *sender ) override
        {
            std::lock_guard<std::mutex> guard( _handlersMutex );
            for ( auto &h : _handlers )
            {
                auto it = h.find( sender );

                if ( it == h.end() )
                    continue;
                h.erase( it );
                while ( !sender->Stop( false ) )
                    ;
                delete sender;
                return;
            }
        }

        ReactorsPtr GetThisPtr()
        {
            return SuperSharedType::shared_from_this();
        }

        static ReactorsPtr New()
        {
            return std::make_shared<ThisType>();
        }
    };
    using GroupedReactorsPtr = std::shared_ptr<GroupedReactors>;
} // namespace reactor
} // namespace ftl
