#include <ftl/out_printer.h>
#include <signal.h>
#include <thread>
#include <try_catch.h>
#include <unistd.h>

namespace ftl {
TryPointsQ& get_global_try_points()
{
    static thread_local TryPointsQ s_q;
    return s_q;
}
static const char* get_sig_desc(int s)
{
    switch (s) {
    case SIGSEGV:
        return "Segment Default";
    case SIGILL:
        return "SIGILL";
    case SIGFPE:
        return "SIGFPE";
    default:
        return "UNDEFINED";
    }
}

void handle_signal(int signum)
{
    /* We may have been waiting for input when the signal arrived,
     but we are no longer waiting once we transfer control. */
    //    printf(" handle_signal:%d\n", signum);
    outPrinter.println(" handle_signal:", signum, get_sig_desc(signum));
    auto& jmpbuf = get_global_try_points().back();
    siglongjmp(jmpbuf.jmpbuf, 1);
    outPrinter.println(" handle_signal exiting\n");
}
void install_signal_catcher()
{
    signal(SIGSEGV, handle_signal);
}
}
