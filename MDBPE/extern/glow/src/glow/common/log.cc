#include "log.hh"

#include <typed-geometry/feature/assert.hh>

#include <chrono>
#include <ctime>
#include <iomanip>
#include <sstream>

std::ostream* glow::detail::logStream = nullptr;
std::ostream* glow::detail::logStreamError = nullptr;
std::string glow::detail::logPrefix = "[$t][$l] ";
uint8_t glow::detail::logMask = 0xFF;

void glow::setLogStream(std::ostream* ossNormal, std::ostream* ossError)
{
    detail::logStream = ossNormal;
    detail::logStreamError = ossError;
}

glow::detail::LogObject::LogObject(std::ostream* oss, LogLevel lvl) : oss(oss)
{
    auto prefix = logPrefix; // copy

    // replace $l
    auto pos = prefix.find("$l");
    if (pos != std::string::npos)
    {
        const char* stype = nullptr;
        switch (lvl)
        {
        case LogLevel::Info:
            stype = "Info";
            break;
        case LogLevel::Debug:
            stype = "Debug";
            break;
        case LogLevel::Error:
            stype = "Error";
            break;
        case LogLevel::Warning:
            stype = "Warning";
            break;
        }

        prefix.replace(pos, 2, stype);
    }

    // replace $t
    pos = prefix.find("$t");
    if (pos != std::string::npos)
    {
        auto now = std::chrono::system_clock::to_time_t(std::chrono::system_clock::now());
        char timestr[10];
        std::strftime(timestr, sizeof(timestr), "%H:%M:%S", std::localtime(&now));

        prefix.replace(pos, 2, timestr);
    }

    *this << prefix;
}

glow::detail::LogObject::~LogObject()
{
    if (oss)
        *oss << std::endl;
}

void glow::setLogPrefix(const std::string& prefix)
{
    detail::logPrefix = prefix;
}

std::ostream* glow::getLogStreamError()
{
    return detail::logStreamError ? detail::logStreamError : &std::cerr;
}

std::ostream* glow::getLogStream()
{
    return detail::logStream ? detail::logStream : &std::cout;
}

glow::detail::LogObject glow::log(LogLevel lvl)
{
    if (!((uint8_t)lvl & detail::logMask))
        return {nullptr, lvl}; // masked

    switch (lvl)
    {
    case LogLevel::Info:
        return {getLogStream(), lvl};
    case LogLevel::Warning:
    case LogLevel::Error:
        return {getLogStreamError(), lvl};
    case LogLevel::Debug:
#ifdef NDEBUG
        return {nullptr, lvl}; // no output
#else
        return {getLogStream(), lvl};
#endif
    default:
        TG_ASSERT(0 && "unknown log level");
        return {nullptr, lvl}; // error, wrong log level
    }
}

void glow::setLogMask(uint8_t mask)
{
    detail::logMask = (uint8_t)mask;
}

void glow::setLogMask(glow::LogLevel mask)
{
    detail::logMask = (uint8_t)mask;
}

uint8_t glow::getLogMask()
{
    return detail::logMask;
}
