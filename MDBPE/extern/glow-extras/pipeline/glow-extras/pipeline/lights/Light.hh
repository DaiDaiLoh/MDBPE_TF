#pragma once

#include <typed-geometry/tg-lean.hh>

#include <glow/common/shared.hh>
#include <glow/std140.hh>

namespace glow
{
namespace pipeline
{
struct Light
{
    tg::vec4 posASize;    ///< Vec3 (Pos A) + float (Size)
    tg::vec4 posBRadius;  ///< Vec3 (Pos B) + float (Radius)
    tg::vec4 colorMaskId; ///< Vec3 (Color) + int (Mask ID)

    /// Tube light
    Light(tg::pos3 const& posA, tg::pos3 const& posB, tg::color3 const& color, float size, float radius, int maskId = 0)
      : posASize{posA, size}, posBRadius{posB, radius}, colorMaskId{color, float(maskId)}
    {
    }

    /// Sphere light
    Light(tg::pos3 const& pos, tg::color3 const& color, float size, float radius, int maskId = 0)
      : posASize{pos, size}, posBRadius{pos, radius}, colorMaskId{color, float(maskId)}
    {
    }

    /// Point light
    Light(tg::pos3 const& pos, tg::color3 const& color, float radius, int maskId = 0)
      : posASize{pos, 0.f}, posBRadius{pos, radius}, colorMaskId{color, float(maskId)}
    {
    }
};
}
}
