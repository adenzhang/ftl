#include <ftl/catch_or_ignore.h>
#include <ftl/out_printer.h>
#include <thread>
#include <try_catch/try_catch.h>
#include <unistd.h>

using namespace ftl;

void read_and_execute_command()
{
    outPrinter.println("will crach");
    int* p = nullptr;
    *p = 234;
}
void test_try()
{
    for (int i = 0; i < 3; ++i) {
        sleep(1);
        outPrinter.println(i, " will setjmp errno:", errno);
        FTL_TRY
        {
            read_and_execute_command();
        }
        FTL_CATCH
        {
            outPrinter.println(" recovered from interuption!");
        }
        FTL_TRY_END
    }
}
TEST_FUNC(try_catch_tests)
{
    _install_try_catch();
    std::thread th1(test_try);
    std::thread th2(test_try);
    th1.join();
    th2.join();
}
