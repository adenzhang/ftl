#include "try_catch.h"
#include <cxxabi.h>
#include <execinfo.h>
#include <pthread.h>
#include <signal.h>

// IMPL_get_global_try_points( MAX_TRY_DEPTH )

thread_try_points_base_t&
_get_thread_try_points()
{
    static thread_local thread_try_points<MAX_TRY_DEPTH> s_try_points;
    return s_try_points;
}

static IPrintf**
_get_thread_printf()
{
    static thread_local IPrintf* s_printf = nullptr; //&stdoutPrintf;
    return &s_printf;
}

void _print_stack_trace(IPrintf* fmt)
{
    static constexpr int max_frames = 63;
    if (!fmt)
        return;
    auto& printf = *fmt;

    void* addrlist[max_frames + 1];

    int addrlen = backtrace(addrlist, sizeof(addrlist) / sizeof(void*));

    if (addrlen == 0) {
        return;
    }

    // resolve addresses into strings containing "filename(function+address)",
    // this array must be free()-ed
    char** symbollist = backtrace_symbols(addrlist, addrlen);

    static constexpr size_t funcnamesize = 256;
    char funcname[funcnamesize];

    for (int i = 3; i < addrlen; i++) {
        char *begin_name = nullptr, *begin_offset = nullptr, *end_offset = nullptr;

        // find parentheses and +address offset surrounding the mangled name:
        // ./module(function+0x15c) [0x8048a6d]
        for (char* p = symbollist[i]; *p; ++p) {
            if (*p == '(')
                begin_name = p;
            else if (*p == '+')
                begin_offset = p;
            else if (*p == ')' && begin_offset) {
                end_offset = p;
                break;
            }
        }

        if (begin_name && begin_offset && end_offset && begin_name < begin_offset) {
            *begin_name++ = '\0';
            *begin_offset++ = '\0';
            *end_offset = '\0';

            int status;
            size_t funcnamelen = funcnamesize;
            abi::__cxa_demangle(begin_name, funcname, &funcnamelen, &status);
            if (status == 0) {
                printf("  %s : %s+%s\n", symbollist[i], funcname, begin_offset);
            } else {
                printf("  %s : %s()+%s\n", symbollist[i], begin_name, begin_offset);
            }
        } else {
            // couldn't parse the line? print the whole line.
            printf("  %s\n", symbollist[i]);
        }
    }

    free(symbollist);
}

static int const s_supported_signals[] = { SIGSEGV, SIGFPE, SIGILL };
static struct sigaction s_saved_sigactions[sizeof(s_supported_signals) / sizeof(int)];

struct try_catch_installer {
    static bool& installed()
    {
        static bool s_installed = false;
        return s_installed;
    }
    static void set_installed(bool bEnable)
    {
        installed() = bEnable;
    }

    static const char* get_signal_desc(int s)
    {
        switch (s) {
        case SIGSEGV:
            return "Segmentation Fault";
        case SIGFPE:
            return "Floating Point Exception";
        case SIGILL:
            return "Illegal Instruction";
        default:
            return nullptr;
        }
    }
    static void signal_handler(int s, siginfo_t*, void*)
    {
        auto tid = pthread_self();
        auto pPrintf = *_get_thread_printf();
        if (pPrintf)
            pPrintf->printf(" signal_handler tid:%ld\n", tid);
        if (auto pDesc = get_signal_desc(s)) {
            if (auto pTop = _get_thread_try_points().top()) {
                if (pTop->data) {
                    pPrintf = pTop->data;
                }

                if (pPrintf) {
                    pPrintf->printf("signal caught %s!\n", pDesc);
                    _print_stack_trace(pPrintf);
                }
                _get_thread_try_points().jump2top();
            } else if (pPrintf) {
                pPrintf->printf(" no jmpbuf set for tid:%ld\n", tid);
                _print_stack_trace(pPrintf);
            }
        } else {
            if (pPrintf)
                pPrintf->printf("unsupported signal %d\n", s);
        }

        if (pPrintf)
            pPrintf->printf("\nExiting signal_handler\n");
    }
    static void install()
    {
        auto pPrintf = *_get_thread_printf();
        if (pPrintf)
            if (pPrintf)
                pPrintf->printf("\ninstalling signal_handler\n");
        struct sigaction act;
        sigemptyset(&act.sa_mask);
        act.sa_flags = SA_SIGINFO | SA_STACK;
#if defined(__clang__)
#pragma GCC diagnostic ignored "-Wdisabled-macro-expansion"
#endif
        act.sa_sigaction = &signal_handler;

        for (int signum : s_supported_signals) {
            if (sigaction(signum, &act, &s_saved_sigactions[signum])) {
                if (pPrintf)
                    pPrintf->printf("ERROR! Failed installing sigaction %s\n", get_signal_desc(signum));
            }
        }
        set_installed(true);
    }
    static void uninstall()
    {
        if (!installed())
            return;
        auto pPrintf = *_get_thread_printf();
        for (int signum : s_supported_signals) {
            if (sigaction(signum, &s_saved_sigactions[signum], nullptr)) {
                if (pPrintf)
                    pPrintf->printf("ERROR! Failed restoring sigaction %s\n", get_signal_desc(signum));
            }
        }
        set_installed(false);
    }

    try_catch_installer()
    {
        install();
    }
};
void _install_try_catch(bool bForce)
{

    static try_catch_installer s_install;
    if (bForce)
        try_catch_installer::install();
    return;
}
void _uninstall_try_catch()
{
    try_catch_installer::uninstall();
}
bool _is_try_catch_installed()
{
    return try_catch_installer::installed();
}
void _set_thread_stack_printer(IPrintf* fmt)
{
    *_get_thread_printf() = fmt;
}
