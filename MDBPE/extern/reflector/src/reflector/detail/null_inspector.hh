#pragma once

namespace rf::detail
{
struct null_inspector
{
    template <class... Args>
    void operator()(Args&&...) const
    {
    }
};
}
