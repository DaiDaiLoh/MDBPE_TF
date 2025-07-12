#pragma once

#include <string>

namespace glow::viewer
{
struct file
{
    file(std::string filepath) : filepath(std::move(filepath)) {}

    std::string filepath;
};
}
