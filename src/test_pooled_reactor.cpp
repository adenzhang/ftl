#include "pooled_reactor.h"
#include "catch.hpp"
#include <vector>
#include <iostream>
#include <sstream>

using namespace reactor;

using EventType = int ;

template<typename  Reactors>
class ReactorsTestFixture
{
    size_t numThreads = 32, numReactors = 32, capTasks = 8096, capEvents=8096;
    size_t numEventsSent = 10000, numEventsGenerators = 10;
    size_t totalEvents = numEventsSent * numEventsGenerators;
public:
//    using Reactors = typename ReactorsPtr::element_type;
    using ReactorsPtr = typename Reactors::ReactorsPtr;
    ReactorsPtr reactors;
    std::vector< reactor::IEventSender<EventType>* > senders;
    std::atomic<size_t> numEventsRcv;

    size_t CountEventsRcv() {
        return numEventsRcv;
    }

    ReactorsTestFixture()
    {
        numEventsRcv = 0;
        reactors = Reactors::New();
        reactors->Init(numThreads, capTasks);
        for( size_t i=0; i< numReactors; ++i ) {
            std::stringstream ss;
            ss << "Sender" << i;
            senders.push_back( & (reactors->template RegisterHandler< EventType >(
                  [&, i](const EventType & e){
                      std::stringstream ss;
                      ss << i << " processing event: " << e ;
                      size_t t = std::rand()%20;
                      std::this_thread::sleep_for(std::chrono::microseconds( t*t ));
                      ++numEventsRcv;
//                      std::cout << ss.str()<< std::endl;
                  } , capEvents )->SetName( ss.str().c_str() )) );
        }

        std::cout << "--- threads:" << numThreads << ", reactors:" << numReactors <<", total events:" << totalEvents << std::endl;
    }

    ~ReactorsTestFixture()
    {
        reactors->Term();
    }

    void WaitSent() {
        while( !CountEventsRcv() )
            ;
    }

    void WaitRecv() {
        while ( CountEventsRcv() < totalEvents )
            ;
    }

    void DoWork()
    {
        for(size_t k=0; k<numEventsGenerators; ++k) {
            std::thread( [&] {
            for(size_t j=0; j< numEventsSent; ++j) {
                size_t i = std::rand() % numReactors;
                std::stringstream ss;
                if( !senders[i]->Send( static_cast<EventType>(j) ) ) {
                    ss << senders[i]->GetName() << " Failed to send " << j;
                    std::cout << ss.str() << std::endl;
                }else{
                }
            }
            } ).detach();
        }

        WaitSent();
        auto tsStart = std::chrono::steady_clock::now();
        WaitRecv();
        auto tsStop = std::chrono::steady_clock::now();
        std::cout << "- processing time(us): " << (tsStop - tsStart).count()/1000 << ", latency(ns):" << (tsStop - tsStart).count()/numEventsGenerators/numEventsSent << std::endl;

        for(auto sender: senders) {
            reactors->RemoveHandler( sender );
        }
        auto tsEnd = std::chrono::steady_clock::now();
        std::cout << "- closing time(us): " << (tsEnd - tsStop).count()/1000 << ", latency(ns):" << (tsEnd - tsStop).count()/senders.size() << std::endl;
    }
};

//TEST_CASE_METHOD(ReactorsTestFixture<SeparatedReactors>, "SeparatedReactors")
//{
//    DoWork();
//}

TEST_CASE_METHOD(ReactorsTestFixture<PooledReactors>, "PooledReactors")
{
    DoWork();
}

TEST_CASE_METHOD(ReactorsTestFixture<GroupedReactors>, "GroupedReactors")
{
    DoWork();
}
