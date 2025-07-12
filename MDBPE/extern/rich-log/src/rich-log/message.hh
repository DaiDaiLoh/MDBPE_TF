#pragma once

#include <ctime>

#include <rich-log/domain.hh>
#include <rich-log/fwd.hh>

#include <clean-core/string_view.hh>

namespace rlog
{
/// a non-owning reference to a logged message
/// CAUTION: DO NOT STORE THIS
///          the message becomes invalid after the LOG call
struct message_ref
{
    std::time_t timestamp;
    rlog::location const* location;
    rlog::domain_info const* domain;
    rlog::verbosity::type verbosity;
    cc::string_view thread_name;
    cc::string_view message;
};
}
