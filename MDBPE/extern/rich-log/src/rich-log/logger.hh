#pragma once

#include <cstdint>

#include <clean-core/macros.hh>
#include <clean-core/span.hh>
#include <clean-core/unique_function.hh>

#include <rich-log/detail/api.hh>
#include <rich-log/domain.hh>
#include <rich-log/location.hh>
#include <rich-log/message.hh>

namespace rlog
{
enum class console_log_style
{
    // [timestamp]     [thread id] [severity] [domain] [message]
    // 06.05.20 07:14:10 t000      INFO       NET      <the message being printed>\n
    verbose,

    // [timestamp] [severity] [domain (optional)] [message]
    // 07:14:10 INFO NET <the message being printed>\n
    brief,

    // [timestamp] [severity] [domain (optional)] [message]
    // 07:14 I NET <the message being printed>\n
    briefer,

    // [message]
    // <the message being printed>\n
    message_only,

    // like verbose, but without color. useful if running inside a terminal
    // that performs poorly with color codes, like qt creators builtin terminal
    verbose_no_color,

    // like verbose, but also including filename:line for the log location
    verbose_with_location
};

/// sets the name for the calling thread, as it appears for the default logger in the log prefix, using printf syntax
/// pass nullptr to un-set
RLOG_API void set_current_thread_name(char const* fmt, ...) CC_PRINTF_FUNC(1);

/// changes the way print_to_console formats its output
[[deprecated("replace the default logger instead. 2022-06-25")]] //
RLOG_API void
set_console_log_style(console_log_style style);

/// enables ANSI Escape sequences in Windows conhost.exe and cmd.exe
RLOG_API bool enable_win32_colors();

/// sets a global minimum verbosity that will trigger breakpoints
/// e.g. rlog::set_break_on_log_minimum_verbosity(rlog::verbosity::Warning);
///      will break on every warning, error, or fatal LOG
/// default is Fatal
RLOG_API void set_break_on_log_minimum_verbosity(verbosity::type v);

/// logger functions process message references
/// CAUTION: DO NOT STORE THE MESSAGE REFERENCE (you need to make copies of the string_views)
/// break_on_log can be set to true or false to overwrite the default behavior
/// returns true if the message is consumed and should not propagate to the next logger
using logger_fun = cc::unique_function<bool(message_ref msg, bool& break_on_log)>;

/// sets the global default logger (i.e. the fallback when no local logger overwrite is provided)
/// CAUTION: this function must be externally synchronized
///          no LOG call must happen (e.g. in a different thread) at the same time
///          usually, this is called once at the start of main and otherwise not touched
RLOG_API void set_global_default_logger(logger_fun logger);

/// pushes a logger onto the threadlocal log overwrite stack
/// i.e. all subsequent LOG calls on the current thread are routed to this logger (unless further overwritten)
/// NOTE: rlog::scoped_logger_override can be used for automatic scoping
RLOG_API void push_local_logger(logger_fun logger);

/// pops a logger from the threadlocal log overwrite stack
RLOG_API void pop_local_logger();

/// the default logger
/// this can be used for custom loggers that still want the default behavior
RLOG_API bool default_logger_fun(message_ref msg, bool& break_on_log);

/// returns all registered domains
/// NOTE: the result is invalidated when a new domain is added (e.g. DLL load or before main)
RLOG_API cc::span<domain_info*> get_domains();

/// helper struct for a threadlocal scoped log overwrite
/// Usage:
///
///   auto _ = rlog::scoped_logger_override([](rlog::message_ref msg, bool& break_on_log) {
///       custom_log(msg.message);
///       return true; // consumed
///   });
///
struct RLOG_API scoped_logger_override
{
    [[nodiscard]] explicit scoped_logger_override(logger_fun logger) { push_local_logger(cc::move(logger)); }

    ~scoped_logger_override() { pop_local_logger(); }

    scoped_logger_override(scoped_logger_override&&) = delete;
    scoped_logger_override& operator=(scoped_logger_override&&) = delete;
    scoped_logger_override(scoped_logger_override const&) = delete;
    scoped_logger_override& operator=(scoped_logger_override const&) = delete;
};

/// helper struct for a threadlocal scoped log silence
/// Usage:
///
///    auto _ = rlog::scoped_logger_silence{}; // no logs in this scope
///    auto _ = rlog::scoped_logger_silence{flag}; // no logs in this scope if flag is true
///
struct RLOG_API scoped_logger_silence
{
    [[nodiscard]] explicit scoped_logger_silence(bool do_silence = true, verbosity::type allow_above_verbosity = verbosity::Fatal)
      : do_silence(do_silence)
    {
        if (do_silence)
            push_local_logger([v = allow_above_verbosity](rlog::message_ref m, bool&) { return m.verbosity <= v; });
    }

    ~scoped_logger_silence()
    {
        if (do_silence)
            pop_local_logger();
    }

    scoped_logger_silence(scoped_logger_silence&&) = delete;
    scoped_logger_silence& operator=(scoped_logger_silence&&) = delete;
    scoped_logger_silence(scoped_logger_silence const&) = delete;
    scoped_logger_silence& operator=(scoped_logger_silence const&) = delete;

private:
    bool do_silence;
};
}
