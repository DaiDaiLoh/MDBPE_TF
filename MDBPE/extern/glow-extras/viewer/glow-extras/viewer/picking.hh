#pragma once

#include <cstdint>    // int32_t
#include <functional> // std::function
#include <optional>
#include <type_traits>

#include <polymesh/Mesh.hh>
#include <typed-geometry/tg.hh>

namespace glow::viewer
{
struct picking_result
{
    tg::color3 pickingColor = tg::color3::red;
    tg::color3 borderColor = tg::color3::white;
    int borderWidth = 2;
    bool successfulPick = true;

    static picking_result noSuccess()
    {
        picking_result r;
        r.successfulPick = false;
        return r;
    }
};

template <class F>
std::function<picking_result(int32_t, tg::pos3, tg::vec3)> makePickFunction(F&& f);

class Picker
{
public:
    void setCustomIds(pm::vertex_attribute<int32_t> va) { mUserDefinedVertexIds = std::move(va); }
    void setCustomIds(pm::edge_attribute<int32_t> ea) { mUserDefinedEdgeIds = std::move(ea); }
    void setCustomIds(pm::face_attribute<int32_t> fa) { mUserDefinedFaceIds = std::move(fa); }

    bool hasUserDefinedIds() const { return (mUserDefinedVertexIds || mUserDefinedEdgeIds || mUserDefinedFaceIds); }

    std::optional<pm::vertex_attribute<int32_t>> const& getUserDefinedVertexIds() const { return mUserDefinedVertexIds; }
    std::optional<pm::edge_attribute<int32_t>> const& getUserDefinedEdgeIds() const { return mUserDefinedEdgeIds; }
    std::optional<pm::face_attribute<int32_t>> const& getUserDefinedFaceIds() const { return mUserDefinedFaceIds; }

    void init(std::vector<pm::face_index> fi);
    void init(std::vector<pm::vertex_index> vi);
    void init(std::vector<pm::edge_index> ei);

    picking_result handlePicking(picking_result cur, tg::pos3 world_pos, tg::ivec2& ids, tg::vec3 normal, bool left_mouse, bool right_mouse);

    template <class F>
    Picker onLeftClick(F&& func)
    {
        mPickingCallbackOnLeftClick = makePickFunction(std::forward<F>(func));
        return *this;
    }

    template <class F>
    Picker onRightClick(F&& func)
    {
        mPickingCallbackOnRightClick = makePickFunction(std::forward<F>(func));
        return *this;
    }

    template <class F>
    Picker onHover(F&& func)
    {
        mPickingCallbackOnHover = makePickFunction(std::forward<F>(func));
        return *this;
    }

private:
    std::function<picking_result(int32_t, tg::pos3, tg::vec3)> mPickingCallbackOnRightClick;
    std::function<picking_result(int32_t, tg::pos3, tg::vec3)> mPickingCallbackOnLeftClick;
    std::function<picking_result(int32_t, tg::pos3, tg::vec3)> mPickingCallbackOnHover;

    // maps back from automatically generated face_ids to face indices of the underlying mesh
    std::optional<std::vector<pm::face_index>> mToFaceIndex;
    std::optional<std::vector<pm::vertex_index>> mToVertexIndex;
    std::optional<std::vector<pm::edge_index>> mToEdgeIndex;

    // temporary storage of user-defined pm::..._attributes
    std::optional<pm::vertex_attribute<int32_t>> mUserDefinedVertexIds;
    std::optional<pm::edge_attribute<int32_t>> mUserDefinedEdgeIds;
    std::optional<pm::face_attribute<int32_t>> mUserDefinedFaceIds;
};

template <class F>
std::function<picking_result(int32_t, tg::pos3, tg::vec3)> makePickFunction(F&& f)
{
    // raw picking function
    if constexpr (std::is_invocable_r_v<picking_result, F, int32_t, tg::pos3, tg::vec3>)
        return std::forward<F>(f);
    else if constexpr (std::is_invocable_r_v<void, F, int32_t, tg::pos3, tg::vec3>)
        return [f = std::forward<F>(f)](int32_t id, tg::pos3 p, tg::vec3 n) -> picking_result {
            f(id, p, n);
            return {};
        };

    // faces
    else if constexpr (std::is_invocable_r_v<picking_result, F, pm::face_index, tg::pos3, tg::vec3>)
        return [f = std::forward<F>(f)](int32_t id, tg::pos3 p, tg::vec3 n) { return f(pm::face_index(id), p, n); };
    else if constexpr (std::is_invocable_r_v<void, F, pm::face_index, tg::pos3, tg::vec3>)
        return [f = std::forward<F>(f)](int32_t id, tg::pos3 p, tg::vec3 n) -> picking_result {
            f(pm::face_index(id), p, n);
            return {};
        };

    // vertices
    else if constexpr (std::is_invocable_r_v<picking_result, F, pm::vertex_index, tg::pos3, tg::vec3>)
        return [f = std::forward<F>(f)](int32_t id, tg::pos3 p, tg::vec3 n) { return f(pm::vertex_index(id), p, n); };
    else if constexpr (std::is_invocable_r_v<void, F, pm::vertex_index, tg::pos3, tg::vec3>)
        return [f = std::forward<F>(f)](int32_t id, tg::pos3 p, tg::vec3 n) -> picking_result {
            f(pm::vertex_index(id), p, n);
            return {};
        };

    // edges
    else if constexpr (std::is_invocable_r_v<picking_result, F, pm::edge_index, tg::pos3, tg::vec3>)
        return [f = std::forward<F>(f)](int32_t id, tg::pos3 p, tg::vec3 n) { return f(pm::edge_index(id), p, n); };
    else if constexpr (std::is_invocable_r_v<void, F, pm::edge_index, tg::pos3, tg::vec3>)
        return [f = std::forward<F>(f)](int32_t id, tg::pos3 p, tg::vec3 n) -> picking_result {
            f(pm::edge_index(id), p, n);
            return {};
        };

    else
        static_assert(tg::always_false<F>, "Picker: Callback function signature error!");
}

Picker pick();
Picker pick(pm::vertex_attribute<int32_t> va);
Picker pick(pm::edge_attribute<int32_t> ea);
Picker pick(pm::face_attribute<int32_t> fa);
}
