#include "Cycler.hh"

#include <glow/common/log.hh>

using namespace glow;
using namespace timing;

static std::string thousandSep(size_t val)
{
    auto s = std::to_string(val);
    auto l = s.size();
    while (l > 3)
    {
        s = s.substr(0, l - 3) + "'" + s.substr(l - 3);
        l -= 3;
    }
    return s;
}

void Cycler::print(const std::string &prefix, int64_t ops, bool restart)
{
    auto dt = cycles() - mCycles;

    if (ops > 0)
        glow::info() << prefix << thousandSep(dt) << " cycles (" << dt / (double)ops << " per op)";
    else
        glow::info() << prefix << thousandSep(dt) << " cycles";

    if (restart)
        mCycles = cycles();
}
