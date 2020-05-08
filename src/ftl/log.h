#pragma once

#include <atomic>
#include <unordered_map>
#include <chrono>
#include <mutex>
#include <iostream>
#include <memory>


namespace ftl
{
// braces format "{}"
struct FormatB
{
    using this_type = FormatB;

    struct LackObjectsError : public std::runtime_error
    {
        LackObjectsError( const char *e = nullptr ) : std::runtime_error( e ){};
    };
    struct LackBracesError : public std::runtime_error
    {
        LackBracesError( const char *e = nullptr ) : std::runtime_error( e ){};
    };

    template<class OStream>
    const this_type &operator()( OStream &os ) const
    {
        return *this;
    }

    template<class OStream>
    const this_type &operator()( OStream &os, const char *fmt ) const
    {
        for ( ; *fmt; ++fmt )
        {
            if ( *fmt == '{' && *( fmt + 1 ) == '}' )
            {
                //                throw LackObjectsError( fmt );
            }
            if ( ( *fmt == '{' && *( fmt + 1 ) == '{' ) || ( *fmt == '}' && *( fmt + 1 ) == '}' ) )
                ++fmt;
            os << *fmt;
        }
        return *this;
    }

    template<class OStream, class A, class... Args>
    const this_type &operator()( OStream &os, const char *fmt, A &&a, Args &&... args ) const
    {
        for ( ; *fmt; ++fmt )
        {
            if ( *fmt == '{' && *( fmt + 1 ) == '}' )
            {
                os << std::forward<A>( a );
                return operator()( os, fmt + 2, std::forward<Args>( args )... );
            }
            if ( ( *fmt == '{' && *( fmt + 1 ) == '{' ) || ( *fmt == '}' && *( fmt + 1 ) == '}' ) )
                ++fmt;
            os << *fmt;
        }
        //        throw LackBracesError( fmt );
        return *this;
    }
};

static constexpr FormatB format;

struct Cat
{
    template<class OStream>
    const Cat &operator()( OStream &os ) const
    {
        return *this;
    }

    template<class OStream, class A, class... Args>
    const Cat &operator()( OStream &os, A &&a, Args &&... args ) const
    {
        os << std::forward<A>( a );
        return operator()( os, std::forward<Args>( args )... );
    }
};
static constexpr Cat cat;


template<size_t N>
struct FmtDigit;

template<>
struct FmtDigit<3>
{
    void operator()( char *buf, size_t nanoPart )
    {
        sprintf( buf, ".%03zu", nanoPart / 1000000 );
    }
};
template<>
struct FmtDigit<6>
{
    void operator()( char *buf, size_t nanoPart )
    {
        sprintf( buf, ".%06zu", nanoPart / 1000 );
    }
};
template<>
struct FmtDigit<9>
{
    void operator()( char *buf, size_t nanoPart )
    {
        sprintf( buf, ".%09zu", nanoPart );
    }
};
template<>
struct FmtDigit<0>
{
    void operator()( char *buf, size_t nanoPart )
    {
        //
    }
};

/// @brief print timepoint in fmt which is used in calling strftime
template<size_t nSubsecondDigits, class TimePointT = std::chrono::system_clock::time_point>
char *PrintTimePoint( char *buf, size_t bufsize, const TimePointT &tp, const char *fmt )
{
    static constexpr size_t Nano2SecondMultiple = 1000000000;
    size_t timeNanoSeconds = tp.time_since_epoch().count();
    size_t secondsPart = timeNanoSeconds / Nano2SecondMultiple;
    size_t nanoSecondsPart = timeNanoSeconds % Nano2SecondMultiple;
    std::chrono::time_point<std::chrono::system_clock, std::chrono::seconds> timeInSec{std::chrono::seconds( secondsPart )};
    auto timet = std::chrono::system_clock::to_time_t( timeInSec );
    tm atime;
    gmtime_r( &timet, &atime );
    auto nBytes = strftime( buf, bufsize, fmt, &atime ); // YYYYMMDD-HH:MM:SS
    if ( nBytes < 0 )
        return nullptr;
    FmtDigit<nSubsecondDigits>()( buf + nBytes, nanoSecondsPart ); // .sss, todo: if constexpr
    return buf;
}

template<size_t nSubsecondDigits, class TimePointT = std::chrono::system_clock::time_point>
char *PrintNow( char *buf, size_t bufsize, const char *fmt )
{
    return PrintTimePoint<nSubsecondDigits>( buf, bufsize, std::chrono::system_clock::now(), fmt );
}

struct SimpleLogger
{
    enum Level
    {
        NONE,
        TRACE,
        DEBUG,
        INFO,
        WARN,
        ERROR
    };

    SimpleLogger( const std::string &name, std::ostream &os, Level level, std::atomic<Level> *globalLevel )
        : name( name ), os( os ), level( level ), globalLevel( globalLevel )
    {
    }

    static std::string get_timestr()
    {
        char buffer[32];
        return PrintNow<3>( buffer, 32, "%Y%m%d-%T" );
    }

    const std::string name;
    std::ostream &os;

    std::atomic<Level> level{NONE};
    std::atomic<Level> *globalLevel = nullptr;
    // todo: add mutex
};


template<class OStreamT = std::basic_ostream<char>>
SimpleLogger *GetSimpleLogger( const std::string &name, std::ostream &os = std::cout )
{
    using SimpleLoggerPtr = std::unique_ptr<SimpleLogger>;
    using Loggers = std::unordered_map<std::string, SimpleLoggerPtr>;

    static Loggers loggers;
    static std::atomic<SimpleLogger::Level> g_lobalLevel{SimpleLogger::Level::DEBUG};
    static std::mutex mut;
    {
        std::unique_lock<std::mutex> lock( mut );
        auto it = loggers.find( name );
        if ( it != loggers.end() )
            return it->second.get();
        auto pLogger = SimpleLoggerPtr( new SimpleLogger{name, os, SimpleLogger::DEBUG, &g_lobalLevel} );
        auto ret = pLogger.get();
        loggers.insert( Loggers::value_type( name, std::move( pLogger ) ) );
        return ret;
    }
}
} // namespace ftl

#define GET_LOGGER( name ) ftl::GetSimpleLogger( name )

#define LOG_LEVEL( LEVEL, logger, expr )                                                                                                            \
    logger->os << ftl::SimpleLogger::get_timestr() << " [" #LEVEL "] " << logger->name << " : " << expr << std::endl << std::flush
#define LOG_T( logger, expr ) LOG_LEVEL( TRACE, logger, expr )
#define LOG_D( logger, expr ) LOG_LEVEL( DEBUG, logger, expr )
#define LOG_I( logger, expr ) LOG_LEVEL( INFO, logger, expr )
#define LOG_W( logger, expr ) LOG_LEVEL( WARN, logger, expr )
#define LOG_E( logger, expr ) LOG_LEVEL( ERROR, logger, expr )

#define FMT_LEVEL( LEVEL, logger, ... )                                                                                                             \
    logger->os << ftl::SimpleLogger::get_timestr() << " [" #LEVEL "] " << logger->name << " : ";                                                    \
    ftl::format( logger->os, __VA_ARGS__ );                                                                                                         \
    logger->os << std::endl
#define FMT_T( logger, ... ) FMT_LEVEL( TRACE, logger, __VA_ARGS__ )
#define FMT_D( logger, ... ) FMT_LEVEL( DEBUG, logger, __VA_ARGS__ )
#define FMT_I( logger, ... ) FMT_LEVEL( INFO, logger, __VA_ARGS__ )
#define FMT_W( logger, ... ) FMT_LEVEL( WARN, logger, __VA_ARGS__ )
#define FMT_E( logger, ... ) FMT_LEVEL( ERROR, logger, __VA_ARGS__ )

#define CAT_LEVEL( LEVEL, logger, ... )                                                                                                             \
    logger->os << ftl::SimpleLogger::get_timestr() << " [" #LEVEL "] " << logger->name << " : ";                                                    \
    ftl::cat( logger->os, __VA_ARGS__ );                                                                                                            \
    logger->os << std::endl
#define CAT_T( logger, ... ) CAT_LEVEL( TRACE, logger, __VA_ARGS__ )
#define CAT_D( logger, ... ) CAT_LEVEL( DEBUG, logger, __VA_ARGS__ )
#define CAT_I( logger, ... ) CAT_LEVEL( INFO, logger, __VA_ARGS__ )
#define CAT_W( logger, ... ) CAT_LEVEL( WARN, logger, __VA_ARGS__ )
#define CAT_E( logger, ... ) CAT_LEVEL( ERROR, logger, __VA_ARGS__ )
