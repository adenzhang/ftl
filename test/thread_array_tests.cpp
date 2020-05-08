#include <ftl/thread_array.h>
#include <ftl/mpsc_bounded_queue.h>
#include <iostream>
#include <ftl/ftl.h>


ADD_TEST_CASE( test_thread_array )
{
    using Task = std::function<void( size_t )>;
    using MPSCQue = ftl::MPSCBoundedQueue<Task>;
    using MyThreadArray = ftl::ThreadArray<Task, MPSCQue>;

    const size_t N = 4;
    MyThreadArray ta( N, 3 );
    for ( size_t i = 0; i < 10; ++i )
    {
        for ( unsigned j = 0; j < N; ++j )
        {
            while ( !ta.put_task( j, [i]( size_t tidx ) { std::cout << "thread idx:" << tidx << ", msg:" << i << std::endl; } ) )
            {
                std::cout << "Failed thread idx:" << j << ", msg:" << i << std::endl;
                std::this_thread::sleep_for( std::chrono::milliseconds( 50 ) );
            }
        }
    }
    ta.stop( true );
    std::cout << "stopped!" << std::endl;
}
