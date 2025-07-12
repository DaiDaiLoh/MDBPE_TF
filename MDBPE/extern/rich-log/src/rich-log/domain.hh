#pragma once

#include <clean-core/macros.hh>

#include <rich-log/detail/api.hh>

namespace rlog
{
namespace verbosity
{
/// Verbosity defines how relevant a log message is in increasing order
/// there can be minimum verbosity levels compile-time and run-time
/// per default:
///   - TRACE is disabled compile-time
///   - Debug is disabled run-time
///   - everything else is enabled
enum type : int
{
    /// TRACE is used to provide diagnostic information verbosely and in very small steps
    /// e.g. logging in each iteration of an algorithm
    Trace = 0,

    /// DEBUG is used to provide diagnostic information that usually do not matter
    /// e.g. logging the arguments to an algorithm
    Debug = 1,

    /// INFO is used to log interesting information that USERS of your algorithm might want to know about
    /// e.g. the selected graphics card, filenames for large save operations, UI actions
    Info = 2,

    /// WARNING is used to log suspicious calls / inputs
    /// e.g. loading empty files, showing empty meshes, degenerate faces
    /// while they are not strictly errors, you should minimize false positives (users will start to ignore them)
    Warning = 3,

    /// ERRROR is used for clear errors and wrong use of APIs
    /// e.g. trying to load a nonexistant file, providing unsupported argument combinations
    /// these errors do not crash the program, but are clearly wrong
    Error = 4,

    /// FATAL is used for errors that will, with a high probability, crash/invalidate the program
    /// e.g. failure to acquire crucial resources, out of memory
    /// fatal errors also tend to be unrecoverable with no clear way to fix them
    Fatal = 5,

    _count
};
}

struct domain_info
{
    rlog::verbosity::type min_verbosity = rlog::verbosity::Info; // always first
    char const* name = "";
    char const* ansi_color_code = "\u001b[38;5;244m";

    static constexpr domain_info make_named(char const* name)
    {
        domain_info di;
        di.name = name;
        return di;
    }
};
}

/// declares a logging domain
/// these names are identifiers of _namespaces_
/// so hierarchies can be formed by using ::
/// they are placed within the namespace they reside in. intended use is the same namespace as the corresponding library
///
/// Usage:
///
///   namespace MyEngine
///   {
///       RICH_LOG_DECLARE_DEFAULT_DOMAIN();
///       RICH_LOG_DECLARE_DOMAIN(MeshImporter::FBX);
///   }
///
/// Note:
///
///   in some .cc, you need to add:
///   namespace MyEngine
///   {
///       RICH_LOG_DEFINE_DEFAULT_DOMAIN("MyEngine");
///       RICH_LOG_DEFINE_DOMAIN(MeshImporter::FBX, "MyEngine::MeshImporter::FBX");
///   }
///
///   TODO: how can the struct be custom initialized?
///
///   Domain settings can be changed anytime, e.g.:
///
///     MyEngine::Log::Default::domain.min_verbosity = rlog::verbosity::Debug;
///     MyEngine::Log::Default::domain.break_on_log[rlog::verbosity::Warning] = true;
///
///     (this should be externally synchronized, otherwise it might create a race condition)
///
#define RICH_LOG_DECLARE_DEFAULT_DOMAIN() RICH_LOG_DECLARE_DOMAIN_DETAIL(Default, Trace, extern)
#define RICH_LOG_DECLARE_DOMAIN(Name) RICH_LOG_DECLARE_DOMAIN_DETAIL(Name, Trace, extern)
#define RICH_LOG_DECLARE_DOMAIN_DETAIL(Name, MinVerbosity, APIPrefix) \
    namespace Log                                                     \
    {                                                                 \
    namespace Name                                                    \
    {                                                                 \
    enum : int                                                        \
    {                                                                 \
        CompileTimeMinVerbosity = rlog::verbosity::MinVerbosity       \
    };                                                                \
                                                                      \
    APIPrefix ::rlog::domain_info domain;                             \
    }                                                                 \
    }                                                                 \
    RICH_LOG_IMPL_INJECT_DOMAIN_FOR_INTELLISENSE(Name)                \
    CC_FORCE_SEMICOLON

#define RICH_LOG_DEFINE_DEFAULT_DOMAIN(NameStr) RICH_LOG_DEFINE_DOMAIN(Default, NameStr)
#define RICH_LOG_DEFINE_DOMAIN(Name, NameStr)                                         \
    ::rlog::domain_info Log::Name::domain = ::rlog::domain_info::make_named(NameStr); \
    static ::rlog::detail::domain_registerer CC_MACRO_JOIN(_rlog_register_domain, __COUNTER__)(&Log::Name::domain) // force ;

#ifdef __INTELLISENSE__
#define RICH_LOG_IMPL_INJECT_DOMAIN_FOR_INTELLISENSE(name) \
    namespace name                                         \
    {                                                      \
    }
#else
#define RICH_LOG_IMPL_INJECT_DOMAIN_FOR_INTELLISENSE(name)
#endif

namespace rlog::detail
{
struct domain_registerer
{
    domain_registerer(domain_info* domain);
};
}

// declare a default domain
RICH_LOG_DECLARE_DOMAIN_DETAIL(Default, Trace, RLOG_API extern);
