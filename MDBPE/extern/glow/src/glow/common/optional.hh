#pragma once

#include <typed-geometry/feature/assert.hh>

namespace glow
{
// lightweight std::optional alternative for pre-C++17
// only use it with value types or things with sane default ctor
template <class T>
struct optional
{
private:
    bool _has_value = false;
    T _value;

public:
    optional() : _has_value(false) {}
    optional(T const& v) : _has_value(true), _value(v) {}

    bool has_value() const { return _has_value; }
    explicit operator bool() const { return has_value(); }

    T const& value() const
    {
        TG_ASSERT(has_value());
        return _value;
    }
    T const& value_or(T const& def) const { return has_value() ? _value : def; }
    T const& operator*() const { return value(); }

    optional& operator=(T const& v)
    {
        _has_value = true;
        _value = v;
    }

    void reset()
    {
        _has_value = false;
        _value = {};
    }
};
}
