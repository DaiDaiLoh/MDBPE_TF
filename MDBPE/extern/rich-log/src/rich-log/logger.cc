#include "logger.hh"

#include <cstdarg>
#include <cstdio>
#include <cstring>
#include <ctime>
#include <mutex>

#include <clean-core/macros.hh>
#include <clean-core/utility.hh>
#include <clean-core/vector.hh>

#include <rich-log/experimental.hh>
#include <rich-log/log.hh>
#include <rich-log/message.hh>

#ifdef CC_OS_WINDOWS
#include <clean-core/native/win32_sanitized.hh>

// this macro is only defined with SDK versions beyond 10586
// (the call can of course still succeed depending on the version of the executing system)
#ifndef ENABLE_VIRTUAL_TERMINAL_PROCESSING
#define ENABLE_VIRTUAL_TERMINAL_PROCESSING 0x0004
#endif

#endif

#define RLOG_COLOR_TIMESTAMP "\u001b[38;5;37m"
#define RLOG_COLOR_RESET "\u001b[0m"

namespace
{
thread_local char tls_thread_name[32] = "";
rlog::verbosity::type g_break_on_log_min_verbosity = rlog::verbosity::Fatal;

rlog::logger_fun g_default_logger;
thread_local cc::vector<rlog::logger_fun> g_local_logger_stack;

CC_FORCE_INLINE void write_timebuffer(char* timebuffer, size_t size, std::time_t t, char const* format)
{
#ifdef CC_OS_WINDOWS
    std::tm lt_stack;
    ::localtime_s(&lt_stack, &t);
    std::tm* const lt = &lt_stack;
#else
    std::tm* const lt = std::localtime(&t);
#endif
    timebuffer[std::strftime(timebuffer, size, format, lt)] = '\0';
}

}

bool rlog::default_logger_fun(message_ref msg, bool& break_on_log)
{
    (void)break_on_log; // default behavior is fine

    auto stream = msg.verbosity >= rlog::verbosity::Warning ? stderr : stdout;

    // prepare timestamp
    char timebuffer[9];
    write_timebuffer(timebuffer, sizeof(timebuffer), msg.timestamp, "%H:%M:%S");

    // brief log line
    // [timestamp] [severity] [domain] [message]
    // 07:14:10 WARNING [NET] <the message being printed>\n

    char const* verbosity_color_code = "";
    char const* verbosity_name = "";
    switch (msg.verbosity)
    {
    case rlog::verbosity::Trace:
        verbosity_color_code = "\u001b[38;5;14m";
        verbosity_name = "TRACE ";
        break;
    case rlog::verbosity::Debug:
        verbosity_color_code = "\u001b[38;5;148m";
        verbosity_name = "DEBUG ";
        break;
    case rlog::verbosity::Info:
        verbosity_color_code = "\u001b[38;5;241m";
        // no verbosity name
        break;
    case rlog::verbosity::Warning:
        verbosity_color_code = "\u001b[38;5;202m";
        verbosity_name = "WARNING ";
        break;
    case rlog::verbosity::Error:
        verbosity_color_code = "\u001b[38;5;196m\u001b[1m";
        verbosity_name = "ERROR ";
        break;
    case rlog::verbosity::Fatal:
        verbosity_color_code = "\u001b[38;5;196m\u001b[1m";
        verbosity_name = "FATAL ";
        break;

    case rlog::verbosity::_count: // silence warning
        break;
    }

    // simple mutex to make sure LOGs are "atomic"
    // this is not really high performance, but those users should use set_global_default_logger anyways
    static std::mutex printf_mutex;
    auto _ = std::lock_guard<std::mutex>(printf_mutex);

    // flush other stream to improve ordering
    std::fflush(stream == stdout ? stderr : stdout);

    // timestamp and severity (always)
    int prefix_length = 9 + std::strlen(verbosity_name);
    std::fprintf(stream,                                                              //
                 RLOG_COLOR_TIMESTAMP "%s " RLOG_COLOR_RESET "%s%s" RLOG_COLOR_RESET, //
                 timebuffer, verbosity_color_code, verbosity_name);

    // domain, optional
    if (msg.domain != &Log::Default::domain)
    {
        std::fprintf(stream,                   //
                     "%s%s " RLOG_COLOR_RESET, //
                     msg.domain->ansi_color_code, msg.domain->name);
        prefix_length += std::strlen(msg.domain->name) + 1;
    }


    // print actual message line by line
    auto first_line = true;
    for (auto line : msg.message.split('\n'))
    {
        // TODO: limit output size if it's too large?

        if (first_line)
            first_line = false;
        else
            // pad with spaces
            std::fprintf(stream, "%*s", prefix_length, "");

        std::fprintf(stream, "%.*s\n", int(line.size()), line.data());
    }
    if (first_line) // empty msg?
        std::fprintf(stream, "\n");

    // flush curr streams to improve ordering
    std::fflush(stream == stdout ? stdout : stderr);

    return true;
}

void rlog::experimental::set_whitelist_filter(cc::unique_function<bool(cc::string_view domain, cc::string_view message)>)
{
    // is ignored in the current model
}

bool rlog::detail::do_log(const domain_info& domain, verbosity::type verbosity, location* loc, rlog::rate::log_rate_limiter* rate_limiter, cc::string_view message)
{
    auto const curr_time = std::time(nullptr);

    if (rate_limiter && !rate_limiter->try_log())
        return false;

    message_ref msg;
    msg.timestamp = curr_time;
    msg.location = loc;
    msg.domain = &domain;
    msg.verbosity = verbosity;
    msg.thread_name = tls_thread_name;
    msg.message = message;

    CC_ASSERT(0 <= verbosity && verbosity < rlog::verbosity::_count);
    auto break_on_log = false;
    break_on_log |= verbosity >= g_break_on_log_min_verbosity;
    break_on_log |= loc->break_on_log;
    break_on_log |= loc->break_on_log_once;

    loc->break_on_log_once = false; // always disable after
    loc->last_log = msg.timestamp;

    // try local loggers
    auto consumed = false;
    for (auto i = int(g_local_logger_stack.size()) - 1; i >= 0; --i)
    {
        if (g_local_logger_stack[i](msg, break_on_log))
        {
            consumed = true;
            break;
        }
    }
    // .. try user-defined default logger
    if (!consumed && g_default_logger.is_valid())
    {
        if (g_default_logger(msg, break_on_log))
        {
            consumed = true;
        }
    }
    // .. if still not consumed, use built-in default logger
    if (!consumed)
        default_logger_fun(msg, break_on_log);

    return break_on_log;
}

void rlog::set_current_thread_name(const char* fmt, ...)
{
    if (fmt != nullptr)
    {
        std::va_list args;
        va_start(args, fmt);
        std::vsnprintf(tls_thread_name, sizeof(tls_thread_name), fmt, args);
        va_end(args);
    }
    else
    {
        tls_thread_name[0] = '\0';
    }
}

void rlog::set_console_log_style(rlog::console_log_style)
{
    // deprecated
}

bool rlog::enable_win32_colors()
{
#ifdef CC_OS_WINDOWS
    ::HANDLE const console_handle = ::GetStdHandle(STD_OUTPUT_HANDLE);
    if (console_handle == INVALID_HANDLE_VALUE)
        return false;

    ::DWORD prev_mode;
    if (!::GetConsoleMode(console_handle, &prev_mode))
        return false;

    prev_mode |= ENABLE_VIRTUAL_TERMINAL_PROCESSING;
    if (!::SetConsoleMode(console_handle, prev_mode))
        return false;

    return true;
#else
    return true;
#endif
}

void rlog::set_break_on_log_minimum_verbosity(verbosity::type v) { g_break_on_log_min_verbosity = v; }

void rlog::set_global_default_logger(logger_fun logger) { g_default_logger = cc::move(logger); }

void rlog::push_local_logger(logger_fun logger)
{
    CC_ASSERT(logger.is_valid() && "loggger must be a valid function");
    g_local_logger_stack.push_back(cc::move(logger));
}

void rlog::pop_local_logger()
{
    CC_ASSERT(!g_local_logger_stack.empty() && "no local logger on the stack. scope mismatch? or wrong thread?");
    g_local_logger_stack.pop_back();
}

static cc::vector<rlog::domain_info*>& g_domains()
{
    static cc::vector<rlog::domain_info*> v;
    return v;
}

rlog::detail::domain_registerer::domain_registerer(domain_info* domain) { g_domains().push_back(domain); }

cc::span<rlog::domain_info*> rlog::get_domains() { return g_domains(); }
