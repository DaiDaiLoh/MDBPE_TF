#pragma once

#include <memory>

#include "force_semicolon.hh"
#include "nodiscard.hh"

/** Shared definitions
 *
 * Can also be used for forward declarations
 *
 * Usage:
 *
 * GLOW_SHARED(class, X);
 * GLOW_SHARED(struct, Y);
 *
 * // now SharedX is usable
 * // and SharedY... for struct Y
 */

#define GLOW_SHARED(type, class_or_struct_name) \
    type class_or_struct_name;                  \
                                                \
    using Shared##class_or_struct_name = std::shared_ptr<class_or_struct_name> // force ;

/** Shared create method
 *
 * Simple helper to add a
 * static SharedX create(...)
 * method to a class or struct
 */

#define GLOW_SHARED_CREATOR(class_or_struct_name)                                   \
    template <typename... Args>                                                     \
    GLOW_NODISCARD static Shared##class_or_struct_name create(Args&&... args)       \
    {                                                                               \
        return std::make_shared<class_or_struct_name>(std::forward<Args>(args)...); \
    }                                                                               \
    GLOW_FORCE_SEMICOLON
