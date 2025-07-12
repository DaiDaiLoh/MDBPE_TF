#pragma once

#include <typed-geometry/functions/basic/statistics.hh>
#include <typed-geometry/types/pos.hh>

#include <clean-core/array.hh>
#include <clean-core/functors.hh>
#include <clean-core/vector.hh>

namespace util
{
template <int D, class T, class... SizeTypeList /* this parameter pack contains D size_t */>
struct tensor_t
{
    static_assert(std::conjunction_v<std::is_same<SizeTypeList, size_t>...> && "dimensions must be integer valued");
    static_assert(sizeof...(SizeTypeList) == D, "must give a coordinate for each dimension");
    static_assert(D > 0, "Number of dimensions must be at least one");

public:
    tensor_t() = default;

    tensor_t(SizeTypeList... dimensions) { resize(dimensions...); }

    tensor_t(cc::span<size_t const> dimensions) { resize(dimensions); }

    void resize(cc::span<size_t const> dimensions)
    {
        CC_ASSERT(dimensions.size() == D && "dimension mismatch");
        auto size = dimensions[0];
        for (auto i = 1; i < dimensions.size(); ++i)
            size *= dimensions[i];
        _data.resize(size);
        dimensions.copy_to(_dimensions);
    }

    void resize(SizeTypeList... dimensions)
    {
        _dimensions = {dimensions...};
        size_t size = (dimensions * ...);
        _data.resize(size);
    }

    void clear()
    {
        _data.clear();
        for (auto d = 0; d < D; ++d)
            _dimensions[d] = 0;
    }

    T& operator()(SizeTypeList... coords) { return _data[index_of(coords...)]; }

    T const& operator()(SizeTypeList... coords) const { return _data[index_of(coords...)]; }

    size_t dimension(size_t index) const
    {
        CC_CONTRACT(index < D);
        return _dimensions[index];
    }

    cc::array<size_t, D> const& dimensions() const { return _dimensions; }

    bool is_valid_coordinate(SizeTypeList... coords) const
    {
        auto arr = cc::array{coords...};
        for (auto i = 0; i < D; ++i)
        {
            if (_dimensions[i] <= arr[i])
                return false;
        }
        return true;
    }

    T* data_ptr() { return _data.data(); }
    T const* data_ptr() const { return _data.data(); }

    size_t total_size() const { return _data.size(); }

    cc::vector<T>& values() { return _data; }
    cc::vector<T> const& values() const { return _data; }

    // special access operators for 2d and 3d

    template <class U = T>
    typename std::enable_if_t<D == 2, U&> operator()(tg::ipos2 const& pos)
    {
        return _data[index_of(pos.x, pos.y)];
    }

    template <class U = T>
    typename std::enable_if_t<D == 2, U const&> operator()(tg::ipos2 const& pos) const
    {
        return _data[index_of(pos.x, pos.y)];
    }

    template <class U = T>
    typename std::enable_if_t<D == 3, U&> operator()(tg::ipos3 const& pos)
    {
        return _data[index_of(pos.x, pos.y, pos.z)];
    }

    template <class U = T>
    typename std::enable_if_t<D == 3, U const&> operator()(tg::ipos3 const& pos) const
    {
        return _data[index_of(pos.x, pos.y, pos.z)];
    }

    template <int U = D>
    typename std::enable_if_t<U == 1 || U == 2 || U == 3, size_t> width() const
    {
        return _dimensions[0];
    }

    template <int U = D>
    typename std::enable_if_t<U == 2 || U == 3, size_t> height() const
    {
        return _dimensions[1];
    }

    template <int U = D>
    typename std::enable_if_t<U == 3, size_t> depth() const
    {
        return _dimensions[2];
    }

public: // range ops
    template <class F>
    void apply(F&& f)
    {
        static_assert(std::is_invocable_v<F, T> && "f must be callable with T");
        for (auto& v : _data)
            f(v);
    }

    template <class F>
    auto map(F&& f) -> tensor_t<D, std::invoke_result_t<F, T>, SizeTypeList...>
    {
        static_assert(std::is_invocable_v<F, T> && "f must be callable with T");
        tensor_t<D, std::invoke_result_t<F, T>, SizeTypeList...> result(_dimensions);
        for (size_t i = 0; i < _data.size(); ++i)
            result._data[i] = f(_data[i]);
        return result;
    }

    template <class TransformT = cc::identity_function>
    auto min(TransformT&& transform = {}) const
    {
        static_assert(std::is_invocable_v<TransformT, T> && "transform must be callable with T");
        return tg::min_by(_data, cc::forward(transform));
    }

    template <class TransformT = cc::identity_function>
    auto max(TransformT&& transform = {}) const
    {
        static_assert(std::is_invocable_v<TransformT, T> && "transform must be callable with T");
        return tg::max_by(_data, cc::forward(transform));
    }

    template <class TransformT = cc::identity_function>
    auto mean(TransformT&& transform = {}) const
    {
        static_assert(std::is_invocable_v<TransformT, T> && "transform must be callable with T");
        return tg::mean(_data, cc::forward(transform));
    }

private:
    cc::vector<T> _data;
    cc::array<size_t, D> _dimensions;

    size_t index_of(SizeTypeList... coords) const
    {
        CC_ASSERT(is_valid_coordinate(coords...));

        auto arr = cc::array{coords...};
        size_t index = 0;
        size_t factor = 1;

        for (size_t i = 0; i < D; ++i)
        {
            index += factor * arr[i];
            factor *= _dimensions[i];
        }
        return index;
    }
};

namespace detail
{
template <int D, class T, class>
struct impl_make_tensor_t;

template <int D, class T, class I, I... Indices>
struct impl_make_tensor_t<D, T, std::integer_sequence<I, Indices...>>
{
    using type = tensor_t<D, T, decltype(Indices)...>;
};
}
template <int D, class T>
using tensor = typename detail::impl_make_tensor_t<D, T, std::make_index_sequence<D>>::type;
}
