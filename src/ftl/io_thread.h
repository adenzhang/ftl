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
#include <vector>
#include <ftl/chan.h>
#include <ftl/thread_array.h>
#include <ftl/log.h>
#include <ftl/inplace_function.h>

#include <sys/epoll.h>
#include <unistd.h>

namespace jz
{

// Async IO thread.
// Recv buffers and sending buffers are running in Single-Producer-Single-Consumer threading model.
//
// when sending: allocated in worker thread, released in io thread.
// when receving: allocated in io thread, releaseed in worker thread.

struct IOError
{
    unsigned bytesProcessed = 0;
    int errorno = 0;
}; // <bytesProcessed, errno> where bytesProcessed >= 0.

struct SockInfo
{
    struct Deleter
    {
        void operator()( SockInfo *p )
        {
            p->release();
        }
    };
    using Ptr = std::unique_ptr<SockInfo, Deleter>;

    // @return <bytesSent, errno>
    IOError recv( char *buf, unsigned bufsize )
    {
        // todo
        return {};
    }
    // @return <bytesSent, errno>
    IOError send( const char *buf, unsigned bufsize )
    {
        // todo
        return {};
    }

    /// \brief release this object. Users should not directly delete this object.
    virtual bool release() = 0;

    /// \brief close this sock
    virtual bool close() = 0;

    int m_sock;
    std::atomic<bool> m_closed{false};
};

inline std::ostream &operator<<( std::ostream &os, const SockInfo &sock )
{
    os << "sockfd: " << sock.m_sock;
    return os;
}

template<size_t CAP>
struct IOBuffer
{
    using size_type = unsigned;
    constexpr static size_t CAPACITY = CAP;

    size_type m_pos = 0; // start pos for write/send; end pos for read/recv.

    constexpr size_t capacity() const
    {
        return CAP;
    }
    size_type size() const
    {
        return m_size;
    }
    void clear()
    {
        m_pos = 0;
        m_size = 0;
    }
    void resize( size_type n )
    {
        m_size = n;
    }
    char &operator[]( size_type idx )
    {
        return m_data[idx];
    }

protected:
    size_type m_size = 0;
    char m_data[CAP];
};

struct DynBuffer
{
    using size_type = unsigned;

    size_type m_pos = 0; // start pos for read/write

    DynBuffer() = default;
    DynBuffer( size_type nReserves )
    {
        if ( nReserves > 0 )
            m_data.reserve( nReserves );
    }
    size_type size() const
    {
        return static_cast<size_type>( m_data.size() );
    }
    size_t capacity() const
    {
        return m_data.capacity();
    }
    void resize( size_type n )
    {
        m_data.resize( n );
    }
    void clear()
    {
        m_pos = 0;
        m_data.clear();
    }
    char &operator[]( size_type idx )
    {
        return m_data[idx];
    }

protected:
    std::vector<char> m_data;
};

using IOThreadTask = ftl::InplaceFunction<void( std::size_t ), 64>;

struct IOEvents
{
    enum EVENT_TYPE
    {
        READ_EVENT,
        WRITE_EVENT
    };

    virtual ~IOEvents() = 0;

    // called by epoll thread when initializtion.
    // SockInfo not share-owned by IOEvents and SockInfo::release should not be called
    virtual void set_sock( SockInfo &sock ) = 0;

    /// \brief callback when IO read event is ready.
    virtual void on_event_ready( EVENT_TYPE ) = 0;

    virtual void set_thread_delegate( ftl::ThreadArrayDelegate<IOThreadTask> threadDelegate ) = 0;

    /// \brief called when this object is not needed when connection is closed.
    virtual void release() = 0;
};

/// \brief Per-sock object.
/// Epoll thread sends read ready event, which call read function may results in sending parsed object to worker threads.
struct RecvBase : IOEvents
{
    using LoggerType = decltype( GET_LOGGER( "" ) );

    void set_sock( SockInfo &sock ) override
    {
        m_pSock = &sock;
    }

    /// \brief set worker thread
    void set_thread_delegate( ftl::ThreadArrayDelegate<IOThreadTask> threadDelegate ) override
    {
        m_workerThreadDelegate = threadDelegate;
    }

    /// \return may return EWOULDBLOCK.
    void on_event_ready( EVENT_TYPE event ) override
    {
        assert( event == IOEvents::READ_EVENT );
        assert( m_pSock );
        assert( m_recvBuf.size() == 0 );

        IOError ret;
        if ( m_recvBuf.m_pos == 0 ) // the already read bytes.
        {
            unsigned msglen = 0;
            auto ret = recv_header( *m_pSock, msglen );
            if ( ret.errorno || !ret.bytesProcessed )
            {
                FMT_W( m_logger, "Error when reading header {}, errno:{}", *m_pSock, ret.errorno );
                m_recvBuf.clear();
                m_pSock->close();
                return;
            }
            m_recvBuf.resize( msglen );
        }
        auto msglength = m_recvBuf.size(); // the whole message length including the header.
        if ( ret.bytesProcessed != msglength )
        {
            ret = continue_recv( *m_pSock, unsigned( msglength - m_recvBuf.m_pos ) );
            if ( ret.bytesProcessed == 0 || ret.errorno )
            {
                if ( ret.errorno == EAGAIN )
                    return;
                FMT_W( m_logger, "Error when reading {}, errno:{}", *m_pSock, ret.errorno );
                m_recvBuf.clear();
                m_pSock->close();
                return;
            }
        }
        on_recv_complete( *m_pSock );
        m_recvBuf.clear(); // reset to 0.
    }

    void release() override
    {
        if ( m_recvBuf.size() || m_recvBuf.m_pos )
        {
            FMT_E( m_logger, "release sock:{} with non-empty recv buffer size:{}, pos:{}", *m_pSock, m_recvBuf.size(), m_recvBuf.m_pos );
        }
        delete this;
    }

    ~RecvBase() override
    {
    }

protected:
    /// Child must implement this. Output msglength.
    /// Typical implemention: {
    ///     start_recv(headersize);
    ///     get_msg_length_from_header;
    /// }
    virtual IOError recv_header( SockInfo &sock, unsigned &msglength ) = 0;

    /// \brief Child must implement this. Data has been read into recvBuf.
    /// In this function, users may choose to send generated structure to worker thread.
    virtual void on_recv_complete( SockInfo &sock ) = 0;

protected:
    /// \brief read header and allocate the whole buffer. recvBuf.m_pos is the bytes read.
    IOError start_recv( SockInfo &sock, unsigned HeaderSize )
    {
        m_recvBuf.clear();
        m_recvBuf.resize( HeaderSize );
        auto ret = sock.recv( &m_recvBuf[0], HeaderSize );
        if ( ret.bytesProcessed != HeaderSize )
            m_recvBuf.resize( unsigned( std::max( ret.bytesProcessed, 0u ) ) );
        m_recvBuf.m_pos = ret.bytesProcessed;
        return ret;
    }
    IOError continue_recv( SockInfo &sock, unsigned nBytes )
    {
        assert( nBytes + m_recvBuf.m_pos <= m_recvBuf.size() );
        auto ret = sock.recv( &m_recvBuf[m_recvBuf.m_pos], nBytes );
        m_recvBuf.m_pos += ret.bytesProcessed;
        return ret;
    }

protected:
    DynBuffer m_recvBuf;
    SockInfo *m_pSock = nullptr;
    ftl::ThreadArrayDelegate<IOThreadTask> m_workerThreadDelegate;
    LoggerType m_logger;
};


/// \brief per sock object.
/// Multiple worker threads send messages to this object, single SendThread consume.
/// Epoll thread notify when send event is ready.
///
/// \note Another strategy is to use thread pool to check all chans.
///
//  - callback on_event_ready by epoll thread when ready for send.
//  - call sendChan to push message to buffer, when worker thread wants to send.
template<size_t MsgBufferSize = 512>
struct SendBase : IOEvents
{
    using LoggerType = decltype( GET_LOGGER( "" ) );
    using Msg = IOBuffer<MsgBufferSize>;
    using Chan = ftl::MPSCPooledChan<Msg>; // multiple workers send, single SendingIOThread consume.
    using MsgPtr = typename Chan::Ptr;

    void set_sock( SockInfo &sock ) override
    {
        m_pSock = &sock;
    }

    /// \brief call in epoll thread to notify
    void on_event_ready( IOEvents::EVENT_TYPE ) override
    {
        assert( m_pSock );
        post_send_task();
    }

    /// called by epoll thread or other thread to set sending thread.
    void set_thread_delegate( ftl::ThreadArrayDelegate<IOThreadTask> threadDelegate ) override
    {
        m_SendThreadDelegate = threadDelegate;
    }

    MsgPtr create_msg()
    {
        return m_chan.create_unique();
    }

    /// \brief post msg to the queue.
    bool send_msg( MsgPtr pMsg )
    {
        if ( !m_chan.send( std::move( pMsg ) ) )
            return false;
        post_send_task();
        return true;
    }

    /// \brief release object when sock is closed.
    void release() override
    {
        if ( m_chan.peek() )
        {
            FMT_E( m_logger, "release sock:{} with non-empty send buffer.", *m_pSock );
        }
        delete this;
    }

protected:
    void post_send_task()
    {
        assert( m_SendThreadDelegate );
        m_SendThreadDelegate.put_task( [this]( std::size_t ) {
            while ( auto pMsg = m_chan.recv_unique() )
            {
                send_msg_proc( pMsg );
            }
        } );
    }
    /// \return 0 for completed sending, or errno
    int send_msg_proc( MsgPtr &pMsg )
    {
        Msg &msg = *pMsg;
        char *buf = &msg.data[0];
        while ( msg.m_pos != msg.size() )
        {
            auto res = m_pSock->send( buf + msg.m_pos, msg.size() - msg.m_pos );
            msg.m_pos += res.bytesProcessed;
            if ( res.bytesProcessed == 0 || res.errorno )
            {
                if ( res.errorno != EAGAIN ) // error
                {
                    FMT_E( m_logger, "Error sending msg ", *m_pSock );
                    msg.clear();
                    m_pSock->close();
                }
                else // retry triggered by SEND_EVENT.
                {
                    pMsg.release(); // keep message in the queue.
                }
                return res.errorno;
            }
        }
        msg.clear();
        return 0;
    }

protected:
    Chan m_chan; // send Msg to IO thread.
    ftl::ThreadArrayDelegate<IOThreadTask> m_SendThreadDelegate;
    SockInfo *m_pSock = nullptr;
    LoggerType m_logger;
};

// todo: use inplace_function as task by default.
template<class RecvThreadArrayT = ftl::ThreadArray<IOThreadTask, ftl::SPSCRingQueue<IOThreadTask>>,
         class SendThreadArrayT = ftl::ThreadArray<IOThreadTask, ftl::SPSCRingQueue<IOThreadTask>>>
class EpollThread
{
public:
    using LoggerType = decltype( GET_LOGGER( "" ) );

    using RecvThreadArray = RecvThreadArrayT;
    using RecvTask = typename RecvThreadArray::task_type;
    using RecvTaskQue = typename RecvThreadArray::task_queue_type;

    using SendThreadArray = SendThreadArrayT;
    using SendTask = typename SendThreadArray::task_type;
    using SendTaskQue = typename SendThreadArray::task_queue_type;

protected:
    struct SockData : SockInfo
    {
        std::int64_t m_recvThreadIdx = -1, m_sendThreadIdx = -1; // -1 for not threaded send/recv. IOEvents are processed in epoll thread.
        IOEvents *m_pRecvEvents = nullptr;
        IOEvents *m_pSendEvents = nullptr;
        EpollThread *m_parent = nullptr;
        std::atomic<int> m_refcount = {1}; // object may be referenced in epoll thread, recv threads, send threads.

        SockData( int sockfd, EpollThread *parent )
        {
            m_sock = sockfd;
            m_parent = parent;
        }

        SockData &inc_ref()
        {
            m_refcount.fetch_add( 1 );
            return *this;
        }

        bool release() override
        {
            if ( dec_ref() != 0 )
                return false;
            if ( m_pRecvEvents )
                m_pRecvEvents->release();
            if ( m_pSendEvents )
                m_pSendEvents->release();
            return true;
        }
        bool close() override
        {
            return m_parent->close_sock( *this );
        }

    protected:
        int dec_ref()
        {
            auto nref = m_refcount.fetch_sub( 1 );
            if ( nref == 1 )
                release();
            return nref - 1;
        }
    };

    enum class EpollStatus
    {
        NONE,
        WORKING,
        STOPPED
    };

protected:
    std::string m_name;
    LoggerType m_logger;

    int m_epollFd;
    std::thread m_epollThread;
    std::atomic<int> polling_timeout_millisec = 200;
    std::atomic<EpollStatus> m_epollStatus{EpollStatus::NONE};

    RecvThreadArray m_myRecvThreads;
    SendThreadArray m_mySendThreads;

    RecvThreadArray *m_pRecvThreads = nullptr;
    SendThreadArray *m_pSendThreads = nullptr;

    std::size_t m_nextRecvThreadIdx = 0, m_nextSendThreadIdx = 0; // round robin.
    std::atomic<std::size_t> m_nSocks{0};

public:
    EpollThread( const std::string &name ) : m_name( name )
    {
        m_logger = GET_LOGGER( m_name );
    }

    EpollThread(
            const std::string &name, std::size_t nRecvThreads, std::size_t nRecvQueueSize, std::size_t nSendThreads, std::size_t nSendQueueSize )
        : m_name( name )
    {
        m_logger = GET_LOGGER( m_name );
        start_epoll();
        start_io_threads( nRecvThreads, nRecvQueueSize, nSendThreads, nSendQueueSize );
    }

    /// \brief start epoll thread.
    bool start_epoll()
    {
        m_epollFd = epoll_create1( 0 );
        if ( m_epollFd <= 0 )
        {
            CAT_E( m_logger, " Failed to create epoll fd!" );
            return false;
        }
        // start epoll thread
        m_epollThread = std::thread( [this] { epoll_proc(); } );
    }

    /// \brief init internal io threads. if IO threads are used, user must either init internal io threads, or set external threads.
    bool start_io_threads( std::size_t nRecvThreads, std::size_t nRecvQueueSize, std::size_t nSendThreads, std::size_t nSendQueueSize )
    {
        if ( !m_myRecvThreads.start( nRecvThreads, nRecvQueueSize ) )
            return false;
        if ( !m_mySendThreads.start( nSendThreads, nSendQueueSize ) )
            return false;

        m_pRecvThreads = &m_myRecvThreads;
        m_pSendThreads = &m_mySendThreads;

        return true;
    }

    /// \brief send external recv threads
    void set_recv_threads( RecvThreadArray *pRecvThreads )
    {
        m_pRecvThreads = pRecvThreads;
    }

    /// \brief send external send threads
    void set_send_threads( SendThreadArray *pSendThreads )
    {
        m_pSendThreads = pSendThreads;
    }

    // return true if stopped already
    bool initiate_stop()
    {
        LOG_I( m_logger, __func__ );

        m_epollStatus = EpollStatus::STOPPED;
        //        std::vector<int> socks;
        //        {
        //            auto locked = m_sockFds.lockit();
        //            for ( auto &p : *locked )
        //                socks.push_back( p.first );
        //        }
        //        for ( auto fd : socks )
        //        {
        //            close_sock( fd );
        //        }
        return !m_epollThread.joinable();
    }

    bool is_running() const
    {
        return m_epollThread.joinable();
    }

    void wait_stop()
    {
        if ( m_epollThread.joinable() )
            m_epollThread.join();
    }

    /// \brief if calling function keeps returned SockInfo*, it must call inco
    typename SockInfo::Ptr add_sock( int sockFd, IOEvents *pRecv, IOEvents *pSend )
    {
        if ( pRecv )
            ++m_nextRecvThreadIdx;
        if ( pSend )
            ++m_nextSendThreadIdx;
        auto res = add_sock( sockFd, pRecv, m_nextRecvThreadIdx, pSend, m_nextSendThreadIdx );
        if ( !res )
        {
            if ( pRecv )
                --m_nextRecvThreadIdx;
            if ( pSend )
                --m_nextSendThreadIdx;
        }
        return std::move( res );
    }

    /// \param iRecvThreadIdx or iSendThreadIdx, if -1, process event within epoll thread.
    typename SockInfo::Ptr add_sock( int sockFd, IOEvents *pRecv, int iRecvThreadIdx, IOEvents *pSend, int iSendThreadIdx )
    {
        if ( !pRecv && !pSend )
        {
            return nullptr;
        }
        SockData *sock = new SockData( sockFd, this );
        if ( pRecv && iRecvThreadIdx >= 0 )
            sock->m_recvThreadIdx = iRecvThreadIdx % m_pRecvThreads->size();
        if ( pSend && iSendThreadIdx >= 0 )
        {
            sock->m_sendThreadIdx = iSendThreadIdx % m_pSendThreads->size();
            pSend->set_thread_delegate( m_pSendThreads->get_thread_deleate( sock->m_sendThreadIdx ) ); // send sending thread.
        }
        sock->m_pRecvEvents = pRecv;
        sock->m_pSendEvents = pSend;

        //-- add to epoll fd.

        epoll_event evt;
        evt.events = EPOLLET;
        if ( pRecv )
        {
            evt.events |= EPOLLIN;
            pRecv->set_sock( sock );
        }
        if ( pSend )
        {
            evt.events |= EPOLLOUT;
            pSend->set_sock( sock );
        }
        evt.data.ptr = sock;
        if ( epoll_ctl( m_epollFd, EPOLL_CTL_ADD, sockFd, &evt ) )
        {
            CAT_E( m_logger, " Failed to add recv sock:", sockFd );
            delete sock;
            return nullptr;
        }

        CAT_I( m_logger, "Added sock ", sockFd );
        return SockInfo::Ptr( &sock->inc_ref(), SockInfo::Deleter{} );
    }

protected:
    /// \return true if closed.
    bool close_sock( SockData &sock )
    {
        if ( sock.m_closed )
            return true;
        if ( epoll_ctl( m_epollFd, EPOLL_CTL_DEL, sock.m_sock.m_sock, nullptr ) ) // this make sure sock be removed once.
        {
            CAT_W( m_logger, " Failed to EPOLL_CTL_DEL sock:", sock );
            sock.m_closed = true;
            return false;
        }
        sock.m_closed = true;

        close( sock.m_sock );
        CAT_I( m_logger, " Closed sock:", sock );
        release_sock( sock ); // release the ownership by epoll thread.
        return true;
    }

    bool release_sock( SockData &sock )
    {
        auto sockfd = sock.m_sock;
        if ( sock.release() )
        {
            FMT_I( m_logger, " EPOLL_CTL_DEL released sock:{}", sockfd );
            return true;
        }
        return false;
    }

    void epoll_proc()
    {
        constexpr auto MAX_EVENTS = 64u;

        m_epollStatus = EpollStatus::WORKING;
        epoll_event events[MAX_EVENTS];
        while ( EpollStatus::STOPPED != m_epollStatus.load() )
        {
            int nEvents = epoll_wait( m_epollFd, events, MAX_EVENTS, polling_timeout_millisec.load() );
            if ( EpollStatus::STOPPED == m_epollStatus.load() )
            {
                CAT_I( m_logger, "Ignore epoll events while stopping. nEvents:", nEvents );
                break;
            }
            for ( int i = 0; i < nEvents; ++i )
            {
                assert( events[i].data.ptr );
                if ( !events[i].data.ptr )
                {
                    CAT_W( m_logger, "null EpollUserData on sock" );
                    continue;
                }
                auto pUserData = reinterpret_cast<SockData *>( events[i].data.ptr );
                if ( events[i].events & EPOLLERR || events[i].events & EPOLLHUP ) // error
                {
                    CAT_E( m_logger, "[E] epoll event error sock:", *pUserData );
                    close_sock( *pUserData );
                }
                else if ( !pUserData )
                {
                    CAT_E( m_logger, "[E] epoll event null SockData:", *pUserData );
                }
                else if ( events[i].events & EPOLLIN )
                {
                    on_recv_ready( *pUserData );
                }
                else if ( events[i].events & EPOLLOUT )
                {
                    on_send_ready( *pUserData );
                }
                else
                {
                    FMT_E( m_logger, "[E] unknown epoll event :{}, SockData:", events[i].events, *pUserData );
                }
            }
        }

        close( m_epollFd );
        CAT_I( m_logger, "Exited epoll_proc" );
    }
    // process epoll read event.
    void on_recv_ready( SockData &sock )
    {
        assert( sock.m_pRecvEvents );
        if ( !sock.m_pRecvEvents )
        {
            return;
        }
        if ( sock.m_recvThreadIdx < 0 )
        {
            sock.m_pRecvEvents->on_event_ready( IOEvents::READ_EVENT );
            return;
        }

        sock.inc_ref();
        m_pRecvThreads->put_task( sock.m_recvThreadIdx, [&]( std::size_t ) {
            sock.m_pRecvEvents->on_event_ready( IOEvents::READ_EVENT );
            release_sock( sock );
        } );
    }

    void on_send_ready( SockData &sock )
    {
        assert( sock.m_pSendEvents );
        if ( !sock.m_pSendEvents )
        {
            return;
        }
        if ( sock.m_sendThreadIdx < 0 )
        {
            sock.m_pSendEvents->on_event_ready( IOEvents::WRITE_EVENT );
            return;
        }

        sock.inc_ref();
        m_pSendThreads->put_task( sock.m_recvThreadIdx, [&]( std::size_t ) {
            sock.m_pSendEvents->on_event_ready( IOEvents::WRITE_EVENT );
            release_sock( sock );
        } );
    }
};
} // namespace jz
