#pragma once

#include <clean-core/vector.hh>

#include <typed-geometry/types/color.hh>
#include <typed-geometry/types/pos.hh>
#include <typed-geometry/types/size.hh>

namespace img
{
template <class ColorT>
struct image
{
public:
    image() = default;
    explicit image(tg::isize2 size) : _extents(size) { resize(size); }
    explicit image(int width, int height) : image(tg::isize2(width, height)) {}

    ColorT& operator()(int x, int y)
    {
        CC_CONTRACT(0 <= x && x < _extents.width);
        CC_CONTRACT(0 <= y && y < _extents.height);
        return _data[x + _extents.width * y];
    }

    ColorT const& operator()(int x, int y) const
    {
        CC_CONTRACT(0 <= x && x < _extents.width);
        CC_CONTRACT(0 <= y && y < _extents.height);
        return _data[x + _extents.width * y];
    }

    ColorT& operator[](tg::ipos2 position) { return (*this)(position.x, position.y); }

    ColorT const& operator[](tg::ipos2 position) const { return (*this)(position.x, position.y); }

    [[nodiscard]] tg::isize2 extents() const { return _extents; }

    [[nodiscard]] int width() const { return _extents.width; }

    [[nodiscard]] int height() const { return _extents.height; }

    void resize(tg::isize2 const& size)
    {
        _extents = size;
        _data.resize(size.width * size.height);
    }

    [[nodiscard]] bool contains(tg::ipos2 position) const
    {
        return 0 <= position.x && position.x < width() && 0 <= position.y && position.y < height();
    }

    [[nodiscard]] int pixel_count() const { return width() * height(); }

    size_t index_of(int x, int y) const { return x + _extents.width * y; }
    size_t index_of(tg::ipos2 position) const { return index_of(position.x, position.y); }

    ColorT* data_ptr() { return _data.data(); }

    ColorT const* data_ptr() const { return _data.data(); }

    size_t data_size() const { return _data.size(); }

    void clear(ColorT const& value = ColorT())
    {
        for (auto& v : _data)
            v = value;
    }

    /// apply functional to every pixel
    /// image.apply([](ColorT& p){...}); or image.apply([](tg::ipos2 pos, ColorT& p){...});
    template <class F>
    void apply(F&& f)
    {
        for (auto y = 0; y < _extents.height; ++y)
            for (auto x = 0; x < _extents.width; ++x)
            {
                if constexpr (std::is_invocable_v<F, ColorT&>)
                    f((*this)(x, y));
                else if constexpr (std::is_invocable_v<F, tg::ipos2, ColorT&>)
                    f(tg::ipos2(x, y), (*this)(x, y));
                else
                    static_assert(cc::always_false<F> && "F must accept either (ColorT&) or (tg::ipos2, ColorT&)");
            }
    }

    /// map one image to another
    /// auto new_image = image.map([](ColorT const& value){...});
    /// auto new_image = image.map([](tg::ipos2 pos, ColorT const& value){...});
    template <class F>
    auto map(F&& f) const
    {
        if constexpr (std::is_invocable_v<F, ColorT>)
        {
            image<std::decay_t<std::invoke_result_t<F, ColorT>>> output(extents());
#pragma omp parallel for
            for (auto y = 0; y < _extents.height; ++y)
                for (auto x = 0; x < _extents.width; ++x)
                    output[{x, y}] = f((*this)(x, y));
            return output;
        }
        else if constexpr (std::is_invocable_v<F, tg::ipos2, ColorT>)
        {
            image<std::decay_t<std::invoke_result_t<F, tg::ipos2, ColorT>>> output(extents());
#pragma omp parallel for
            for (auto y = 0; y < _extents.height; ++y)
                for (auto x = 0; x < _extents.width; ++x)
                    output[{x, y}] = f(tg::ipos2(x, y), (*this)(x, y));
            return output;
        }
        else
            static_assert(cc::always_false<F> && "F must accept either (ColorT ) or (tg::ipos2, ColorT)");
    }

    /// call f for every pixel
    template <class F>
    void for_each(F&& f) const
    {
        static_assert(std::is_invocable_v<F, ColorT>, "F must accept ColorT");
        for (auto const& p : _data)
            f(p);
    }

    /// call f for every pixel
    template <class F>
    void for_each(F&& f)
    {
        static_assert(std::is_invocable_v<F, ColorT&>, "F must accept ColorT");
        for (auto& p : _data)
            f(p);
    }

    /// Returns true, iff f returns true for every pixel, false otherwise
    template <class F>
    bool all(F&& f) const
    {
        for (auto const& p : _data)
            if (!f(p))
                return false;
        return true;
    }

    /// Returns true, iff f returns true for any pixel, false otherwise
    template <class F>
    bool any(F&& f) const
    {
        for (auto const& p : _data)
            if (f(p))
                return true;
        return false;
    }

    template <class TransformT = tg::identity_fun>
    auto max(TransformT&& transform = {}) const
    {
        auto v = transform(_data[0]);
        for (auto i = 1u; i < _data.size(); ++i)
        {
            auto const vv = transform(_data[i]);
            if (vv > v)
                v = vv;
        }
        return v;
    }

    template <class F>
    ColorT max_by(F&& f) const
    {
        auto v = f(_data[0]);
        for (auto i = 1u; i < _data.size(); ++i)
        {
            auto const vv = f(_data[i]);
            if (vv > v)
                v = vv;
        }
        return v;
    }

    template <class TransformT = tg::identity_fun>
    auto min(TransformT&& transform = {}) const
    {
        auto v = transform(_data[0]);
        for (auto i = 1u; i < _data.size(); ++i)
        {
            auto const vv = transform(_data[i]);
            if (vv < v)
                v = vv;
        }
        return v;
    }

    template <class F>
    ColorT min_by(F&& f) const
    {
        auto v = f(_data[0]);
        for (auto i = 1u; i < _data.size(); ++i)
        {
            auto const vv = f(_data[i]);
            if (vv < v)
                v = vv;
        }
        return v;
    }

private:
    cc::vector<ColorT> _data;
    tg::isize2 _extents;
};

using rgb_image = image<tg::color3>;
using grayscale_image = image<float>;
using binary_image = image<bool>;
}
