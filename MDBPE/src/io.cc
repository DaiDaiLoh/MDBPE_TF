#include "io.hh"

#include <filesystem>

#include <clean-core/from_string.hh>
#include <clean-core/set.hh>
#include <clean-core/vector.hh>

#include <babel-serializer/data/byte_reader.hh>
#include <babel-serializer/data/byte_writer.hh>
#include <babel-serializer/file.hh>
#include <clean-core/sort.hh>

#include <image/io.hh>

#include <rich-log/log.hh>

#include "rule.hh"
#include "util.hh"

img::image<int> tp::read_token_bin_data(cc::string_view filepath)
{
    auto const data = babel::file::read_all_bytes(filepath);

    babel::experimental::byte_reader reader(data);
    auto const width = reader.read_i32();
    auto const height = reader.read_i32();

    img::image<int> image(width, height);

    for (int y = 0; y < height; ++y)
    {
        for (int x = 0; x < width; ++x)
        {
            image(x, y) = reader.read_i32();
        }
    }
    return image;
}

cc::vector<tp::image_data> tp::read_folder(cc::string_view folder)
{
    auto const path = std::filesystem::path(folder.begin(), folder.end());

    if (!std::filesystem::exists(path))
    {
        LOG_ERROR("Input folder does not exist: {}", folder);
        return {};
    }

    cc::vector<image_data> images;

    for (auto const& entry : std::filesystem::recursive_directory_iterator(path))
    {
        if (entry.is_directory())
            continue;

        if (!entry.is_regular_file())
        {
            LOG_WARN("Skipping non-file: {}", entry.path().string());
            continue;
        }

        if (entry.path().extension() != ".dat")
        {
            LOG_WARN("Skipping non-.dat file: {}", entry.path().string());
            continue;
        }

        auto const filename = cc::string(entry.path().filename().string());
        auto const stem = cc::string(entry.path().stem().string());

        auto const filename_parts = cc::vector<cc::string_view>(stem.split('_'));
        int id = -1;
        if (filename_parts.size() == 1)
        {
            if (!cc::from_string(filename_parts[0], id))
            {
                LOG_WARN("File does not have the expected file-format: {}", entry.path().string());
                continue;
            }
        }
        else if (filename_parts.size() == 2)
        {
            if (!cc::from_string(filename_parts[1], id))
            {
                LOG_WARN("File does not have the expected file-format: {}", entry.path().string());
                continue;
            }
        }
        else
        {
            LOG_WARN("File does not have the expected file-format: {}", entry.path().string());
            continue;
        }

        auto image = read_token_bin_data(entry.path().string());

        images.push_back({filename, id, image});
    }

    return images;
}

void tp::write(cc::string_view filepath, img::image<int> const& image, cc::span<tg::color3 const> colors)
{
    // debug: larger images to make stuff easier to view, make smaller if too slow
    auto upscale = 20;
    // auto upscale = 1;

    img::rgb_image target = img::rgb_image(image.width() * upscale, image.height() * upscale);


    for (auto y = 0; y < image.height(); ++y)
        for (auto x = 0; x < image.width(); ++x)
        {
            auto value = image(x, y);
            auto color = colors[value];

            for (auto dy = 0; dy < upscale; ++dy)
                for (auto dx = 0; dx < upscale; ++dx)
                    target(x * upscale + dx, y * upscale + dy) = color;
        }

    img::write(target, filepath);
}

void tp::write(cc::string_view filepath, img::image<int> const& image, int max_colors)
{
    auto colors = generate_colors(max_colors);
    write(filepath, image, colors);
}

void tp::write_images(cc::span<image_data const> images, cc::string_view folder, int iteration, int output_folder_count, cc::span<tg::color3 const> class_color)
{
    for (auto const& image : images)
    {
        auto const path = cc::string(folder) + cc::format("{:06}/{:06}/", image.id % output_folder_count, image.id);
        auto const filename_class = path + cc::format("class_{:06}.png", iteration);
        auto const filename_id = path + cc::format("id_{:06}.png", iteration);
        write(filename_class, image.current_token_class, class_color);
        write(filename_id, image.current_token_id, class_color);
    }
}

void tp::write_token_sequences(cc::span<image_data const> image_data, cc::string output_folder, int folder_modulus)
{
    for (auto const& image : image_data)
    {
        auto raw_data = cc::vector<std::byte>();
        auto writer = babel::make_byte_writer(raw_data);

        auto const& class_image = image.current_token_class;
        auto const& id_image = image.current_token_id;
        cc::set<int> visited;

        for (auto y = 0; y < class_image.height(); ++y)
            for (auto x = 0; x < class_image.width(); ++x)
            {
                auto const id = id_image(x, y);
                if (visited.contains(id))
                    continue;
                visited.add(id);

                auto const token_class = class_image(x, y);
                writer.write_i32(token_class);

                auto const ancor = image.token_ancor[id];
                CC_ASSERT(ancor.x == x && ancor.y == y);
                writer.write_i32(ancor.x);
                writer.write_i32(ancor.y);
            }

        auto filepath
            = output_folder
              + cc::format("{:06}/{:06}/{}_sequence.dat", image.id % folder_modulus, image.id, image.filename.substring(0, image.filename.size() - 4));
        babel::file::write(filepath, raw_data);
    }
}

void tp::write_token_shapes(cc::span<token_data const> tokens, cc::string token_data_folder)
{
    for (auto const& token : tokens)
    {
        auto raw_data = cc::vector<std::byte>();
        auto writer = babel::make_byte_writer(raw_data);

        writer.write_i32(token.class_id);
        writer.write_i32(int(token.positions.size()));
        for (auto i = 0; i < int(token.positions.size()); ++i)
        {
            auto const p = token.positions[i];
            writer.write_i32(p.x);
            writer.write_i32(p.y);
            writer.write_i32(token.position_class[i]);
        }

        auto filepath = token_data_folder + cc::format("token_{:04}.dat", token.class_id);
        babel::file::write(filepath, raw_data);
    }
}

void tp::write_rules(cc::span<rule const> rules, cc::string output_folder)
{
    auto filepath = output_folder + "rules.dat";
    auto raw_data = cc::vector<std::byte>();
    auto writer = babel::make_byte_writer(raw_data);
    for (auto i = 0; i < rules.size(); ++i)
    {
        auto const& rule = rules[i];
        writer.write_i32(rule.constellation.source_class_id);
        writer.write_i32(rule.constellation.target_class_id);
        writer.write_i32(rule.constellation.ancor_offset.x);
        writer.write_i32(rule.constellation.ancor_offset.y);
        writer.write_i32(rule.new_token_id);
    }
    babel::file::write(filepath, raw_data);
}

cc::vector<tp::token_data> tp::read_tokens(cc::string token_folder)
{
    if (!std::filesystem::exists(token_folder.c_str()))
    {
        LOG_ERROR("Token folder does not exist: {}", token_folder);
        return {};
    }
    cc::vector<tp::token_data> tokens;
    for (auto& entry : std::filesystem::directory_iterator(token_folder.c_str()))
    {
        if (!entry.is_regular_file())
        {
            LOG_WARN("Skipping non-file: {}", entry.path().string());
            continue;
        }

        if (entry.path().extension() != ".dat")
        {
            LOG_WARN("Skipping non-.dat file: {}", entry.path().string());
            continue;
        }

        if (!entry.path().filename().string().starts_with("token_"))
        {
            LOG_WARN("Skipping non-token file: {}", entry.path().string());
            continue;
        }

        auto const raw_data = babel::file::read_all_bytes(entry.path().string());
        auto reader = babel::experimental::byte_reader(raw_data);
        token_data token;
        if (!reader.read_i32(token.class_id))
        {
            LOG_ERROR("Failed to read class_id");
        }
        int num_positions;
        if (!reader.read_i32(num_positions))
        {
            LOG_ERROR("Failed to read num_positions");
            return {};
        }
        for (auto i = 0; i < num_positions; ++i)
        {
            tg::ipos2 p;
            if (!reader.read_i32(p.x))
            {
                LOG_ERROR("Failed to read position.x");
            }
            if (!reader.read_i32(p.y))
            {
                LOG_ERROR("Failed to read position.y");
            }
            token.positions.push_back(p);
            int class_id;
            if (!reader.read_i32(class_id))
            {
                LOG_ERROR("Failed to read position_class");
            }
            token.position_class.push_back(class_id);
        }
        tokens.push_back(token);
    }

    // tokens are needed in order
    cc::sort(tokens, [](auto const& a, auto const& b) { return a.class_id < b.class_id; });

    for (auto i = 0; i < tokens.size(); ++i)
    {
        CC_ASSERT(tokens[i].class_id == i);
    }

    return tokens;
}

cc::vector<tp::rule> tp::read_rules(cc::string rule_file)
{
    if (!std::filesystem::exists(rule_file.c_str()))
    {
        LOG_ERROR("Rule file does not exist: {}", rule_file);
        return {};
    }

    cc::vector<tp::rule> rules;

    auto raw_data = babel::file::read_all_bytes(rule_file);
    auto reader = babel::experimental::byte_reader(raw_data);
    while (reader.has_remaining_bytes())
    {
        rule rule;
        if (!reader.read_i32(rule.constellation.source_class_id))
        {
            LOG_ERROR("Failed to read source_class_id");
        }
        if (!reader.read_i32(rule.constellation.target_class_id))
        {
            LOG_ERROR("Failed to read target_class_id");
        }
        if (!reader.read_i32(rule.constellation.ancor_offset.x))
        {
            LOG_ERROR("Failed to read ancor_offset.x");
        }
        if (!reader.read_i32(rule.constellation.ancor_offset.y))
        {
            LOG_ERROR("Failed to read ancor_offset.y");
        }
        if (!reader.read_i32(rule.new_token_id))
        {
            LOG_ERROR("Failed to read new_token_id");
        }
        rules.push_back(rule);
    }
    return rules;
}
