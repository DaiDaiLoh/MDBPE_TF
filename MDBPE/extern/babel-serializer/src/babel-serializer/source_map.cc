#include "source_map.hh"

#include <clean-core/char_predicates.hh>
#include <clean-core/utility.hh>

babel::source_map::source_map(cc::string_view source) { parse(source); }

babel::source_map::source_map(cc::span<const std::byte> source)
{
    parse(cc::string_view(reinterpret_cast<char const*>(source.data()), source.size()));
}

int babel::source_map::line_of(const char* c) const
{
    CC_ASSERT(c >= &_source.front() && c <= &_source.back() && "char not in source");
    auto idx = -1;
    for (auto l : _lines)
    {
        if (c < l.data())
            return idx;

        ++idx;
    }
    return int(_lines.size() - 1);
}

void babel::source_map::parse(cc::string_view source)
{
    CC_ASSERT(_lines.empty() && "can only be called once");
    _source = source;

    if (source.empty())
        return;

    char const* line_start = source.begin();
    for (auto const& c : source)
    {
        if (c < 0x20 && c != '\n' && c != '\t' && c != '\r')
        {
            _is_binary = true;
            _lines.clear();
            return;
        }

        if (c == '\n')
        {
            auto sv = cc::string_view(line_start, &c);
            _lines.push_back(sv.trim('\r'));
            line_start = &c + 1;
        }
    }

    if (line_start != source.end())
    {
        auto sv = cc::string_view(line_start, source.end());
        _lines.push_back(sv.trim('\r'));
    }
}
