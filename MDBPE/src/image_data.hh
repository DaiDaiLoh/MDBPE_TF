#pragma once

#include <clean-core/string.hh>
#include <clean-core/string_view.hh>

#include <image/image.hh>

namespace tp
{
/// image data for a single image
struct image_data
{
public:
    image_data() = default;
    image_data(image_data const&) = default;
    image_data(image_data&) = default;
    image_data& operator=(image_data const&) = default;
    image_data& operator=(image_data&&) = default;

    image_data(cc::string_view filename, int id, img::image<int> token_class)
      : filename{filename}, id{id}, m_initial_token_class{cc::move(token_class)}
    {
        current_token_class = m_initial_token_class; // copy initial classes

        // initialize current_token_id and ancor with the pixel position
        auto const width = m_initial_token_class.width();
        auto const height = m_initial_token_class.height();
        current_token_id.resize({width, height});
        for (auto y = 0; y < height; ++y)
            for (auto x = 0; x < width; ++x)
            {
                current_token_id(x, y) = next_token_id();
                token_ancor.push_back({x, y});
            }
    }

    cc::string filename;                 // input filename
    int id = -1;                         // image id (unique per image)
    img::image<int> current_token_class; // changes after rules are applied
    img::image<int> current_token_id;    // only unique inside this image
    cc::vector<tg::ipos2> token_ancor;   // token_ancor[token_id] gives the ancor of the token

    int next_token_id() { return m_next_token_id++; }

    int max_token_id() const { return m_next_token_id; }

    img::image<int> const& initial_token_class() const { return m_initial_token_class; }

private:
    img::image<int> m_initial_token_class; // never change after initial creation!
    int m_next_token_id = 0;
};
}
