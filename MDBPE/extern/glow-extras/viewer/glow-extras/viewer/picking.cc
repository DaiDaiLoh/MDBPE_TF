#include "picking.hh"

namespace glow::viewer
{
Picker pick() { return Picker(); }

Picker pick(pm::vertex_attribute<int32_t> va)
{
    auto p = Picker();
    p.setCustomIds(std::move(va));
    return p;
}

Picker pick(pm::edge_attribute<int32_t> ea)
{
    auto p = Picker();
    p.setCustomIds(std::move(ea));
    return p;
}

Picker pick(pm::face_attribute<int32_t> fa)
{
    auto p = Picker();
    p.setCustomIds(std::move(fa));
    return p;
}

void Picker::init(std::vector<pm::face_index> fi) { mToFaceIndex = std::move(fi); }

void Picker::init(std::vector<pm::vertex_index> vi) { mToVertexIndex = std::move(vi); }

void Picker::init(std::vector<pm::edge_index> ei) { mToEdgeIndex = std::move(ei); }

picking_result Picker::handlePicking(picking_result cur, tg::pos3 world_pos, tg::ivec2& ids, tg::vec3 normal, bool left_mouse, bool right_mouse)
{
    picking_result res = cur; // stuff

    if ((left_mouse && mPickingCallbackOnLeftClick) || (right_mouse && mPickingCallbackOnRightClick) || (mPickingCallbackOnHover))
    {
        if (mToFaceIndex.has_value() || mUserDefinedFaceIds.has_value())
        {
            // is MeshRenderable
            pm::face_index f_i = pm::face_index(ids.y);
            if (!hasUserDefinedIds())
            {
                f_i = mToFaceIndex.value()[ids.y];
            }
            if (mPickingCallbackOnHover)
            {
                res = mPickingCallbackOnHover(int(f_i), world_pos, normal);
            }
            if (right_mouse && mPickingCallbackOnRightClick)
            {
                res = mPickingCallbackOnRightClick(int(f_i), world_pos, normal);
            }
            if (left_mouse && mPickingCallbackOnLeftClick)
            {
                res = mPickingCallbackOnLeftClick(int(f_i), world_pos, normal);
            }
        }
        else if (mToVertexIndex.has_value() || mUserDefinedVertexIds.has_value())
        {
            // is PointRenderable
            pm::vertex_index v_i = pm::vertex_index(ids.y);
            if (!hasUserDefinedIds())
            {
                v_i = mToVertexIndex.value()[ids.y];
            }
            if (mPickingCallbackOnHover)
            {
                res = mPickingCallbackOnHover(int(v_i), world_pos, normal);
            }
            if (right_mouse && mPickingCallbackOnRightClick)
            {
                res = mPickingCallbackOnRightClick(int(v_i), world_pos, normal);
            }
            if (left_mouse && mPickingCallbackOnLeftClick)
            {
                res = mPickingCallbackOnLeftClick(int(v_i), world_pos, normal);
            }
        }
        else if (mToEdgeIndex.has_value() || mUserDefinedEdgeIds.has_value())
        {
            // is LineRenderable
            pm::edge_index e_i = pm::edge_index(ids.y);
            if (!hasUserDefinedIds())
            {
                e_i = mToEdgeIndex.value()[ids.y];
            }
            if (mPickingCallbackOnHover)
            {
                res = mPickingCallbackOnHover(int(e_i), world_pos, normal);
            }
            if (right_mouse && mPickingCallbackOnRightClick)
            {
                res = mPickingCallbackOnRightClick(int(e_i), world_pos, normal);
            }
            if (left_mouse && mPickingCallbackOnLeftClick)
            {
                res = mPickingCallbackOnLeftClick(int(e_i), world_pos, normal);
            }
        }
    }
    else
    {
        // id should not be picked
        ids.x = -1;
        ids.y = -1;
    }

    return res;
}

}
