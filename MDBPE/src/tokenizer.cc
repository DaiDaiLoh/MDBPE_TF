#include "tokenizer.hh"

#include <chrono>

#include <clean-core/map.hh>
#include <clean-core/set.hh>

#include <typed-geometry/tg.hh>

#include <babel-serializer/data/byte_writer.hh>
#include <babel-serializer/file.hh>

#include <rich-log/log.hh>

#include <cpp-utils/filesystem.hh>

#include "io.hh"
#include "rule.hh"
#include "util.hh"

tp::constellation tp::get_most_common_constellation(cc::span<image_data const> images)
{
    cc::map<constellation, int> constellation_count;

    for (auto& image : images)
    {
        auto const width = image.initial_token_class().width();
        auto const height = image.initial_token_class().height();

        auto const& token_class = image.current_token_class;
        auto const& token_id = image.current_token_id;

        cc::set<cc::pair<tg::ipos2, tg::ipos2>> used; // must be per image!

        for (auto y = 0; y < height; ++y)
            for (auto x = 0; x < width; ++x)
                for (auto dir : {tg::ivec2(0, 1), tg::ivec2(1, 0)})
                {
                    auto const coords = tg::ipos2(x, y);
                    auto const neighbor_coords = coords + dir;

                    if (!token_id.contains(neighbor_coords)) // bounds check
                        continue;

                    auto const current_token_id = token_id[coords];
                    auto const neighbor_token_id = token_id[neighbor_coords];

                    // skip if they're the same unique ID (=we can't merge a single large token with itself)
                    if (current_token_id == neighbor_token_id)
                        continue;

                    auto const current_ancor = image.token_ancor[current_token_id];
                    auto const neighbor_ancor = image.token_ancor[neighbor_token_id];

                    // only do every centre pair once - if we already looked at two unique tokens, we don't need to look at them again (for this one
                    // image):   two unique tokens at specific positions are only counted once
                    // todo: only insert ancor after sorting, i.e. by token id,
                    if (used.contains({current_ancor, neighbor_ancor}) || used.contains({neighbor_ancor, current_ancor}))
                        continue;

                    used.add({current_ancor, neighbor_ancor});

                    auto const offset = neighbor_ancor - current_ancor;

                    auto const current_class = token_class[coords];
                    auto const neighbor_class = token_class[neighbor_coords];

                    constellation_count[{current_class, neighbor_class, offset}] += 1;
                }
    }

    // find constellation with maximal occurrances
    constellation max_constellation;
    auto max_constellation_count = -1;
    for (auto const [constellation, count] : constellation_count)
    {
        if (count > max_constellation_count)
        {
            max_constellation = constellation;
            max_constellation_count = count;
        }
    }

#if 0 // debug output
    {
        // write five most common constellations
        cc::vector<cc::pair<constellation, int>> constellations;
        for (auto [c, count] : constellation_count)
        {
            constellations.push_back({c, count});
        }
        cc::sort(constellations, [](auto const& a, auto const& b) { return a.second > b.second; });
        for (auto i = 0; i < tg::min(5, constellations.size()); ++i)
        {
            auto [con, count] = constellations[i];
            LOG("({} + {}: ({}, {})): {}", con.source_class_id, con.target_class_id, con.ancor_offset.x, con.ancor_offset.y, count);
        }
    }
#endif

    return max_constellation;
}

tp::token_data tp::combine_tokens(constellation const& rule, cc::span<token_data const> tokens)
{
    // generate the new token
    token_data new_token;
    new_token.class_id = int(tokens.size());

    // apply rule
    auto const& token_a = tokens[rule.source_class_id];
    auto const& token_b = tokens[rule.target_class_id];

    // one of the two token ancors will be the new token ancor, find out which one
    auto const offset = rule.ancor_offset;
    auto keep_token_a_ancor = !(offset.y < 0 || (offset.y == 0 && offset.x < 0));

    // combine tokens
    if (keep_token_a_ancor)
    {
        // map points from token_b into space of token_a
        new_token.positions.push_back_range(token_a.positions);
        new_token.position_class.push_back_range(token_a.position_class);
        for (auto i = 0; i < int(token_b.positions.size()); ++i)
        {
            auto const p = token_b.positions[i];
            auto const c = token_b.position_class[i];

            new_token.positions.push_back(p + offset);
            new_token.position_class.push_back(c);
        }
    }
    else
    {
        // map points from token_a into space of token_b
        new_token.positions.push_back_range(token_b.positions);
        new_token.position_class.push_back_range(token_b.position_class);
        for (auto i = 0; i < int(token_a.positions.size()); ++i)
        {
            auto const p = token_a.positions[i];
            auto const c = token_a.position_class[i];

            new_token.positions.push_back(p - offset);
            new_token.position_class.push_back(c);
        }
    }
    return new_token;
}


void tp::apply_rule(rule const& rule, token_data const& new_token, cc::span<image_data> images)
{
    auto const offset = rule.constellation.ancor_offset;
    auto keep_token_a_ancor = !(offset.y < 0 || (offset.y == 0 && offset.x < 0));

    auto const n_images = images.size();
    // #pragma omp parallel for // does not improve stuff :(
    for (size_t i = 0; i < n_images; ++i)
    {
        auto& image = images[i];

        auto const width = image.initial_token_class().width();
        auto const height = image.initial_token_class().height();

        for (auto y = 0; y < height; ++y)
            for (auto x = 0; x < width; ++x)
            {
                auto const coords = tg::ipos2(x, y);

                auto const current_token_class = image.current_token_class[coords];
                if (current_token_class != rule.constellation.source_class_id) // not the right token to apply the rule
                    continue;

                auto const current_token_ancor = image.token_ancor[image.current_token_id[coords]];

                if (coords != current_token_ancor) // only apply rule to token ancors
                    continue;

                auto const other_token_coords = coords + rule.constellation.ancor_offset;

                if (!image.initial_token_class().contains(other_token_coords)) // bounds check
                    continue;

                auto const other_token_class = image.current_token_class[other_token_coords];
                if (other_token_class != rule.constellation.target_class_id) // not the right token to apply the rule
                    continue;

                auto const other_token_ancor = image.token_ancor[image.current_token_id[other_token_coords]];
                if (other_token_ancor != other_token_coords) // not the correct ancor
                    continue;

                // now: correct token classes, and correct ancors, therefore apply rule!
                // do so relative to the correct ancor

                auto const new_ancor = keep_token_a_ancor ? current_token_ancor : other_token_ancor;
                auto const new_id = image.next_token_id();
                image.token_ancor.push_back(new_ancor);
                for (auto const p : new_token.positions)
                {
                    auto const new_coords = new_ancor + tg::ivec2(p);
                    if (!image.initial_token_class().contains(new_coords))
                        continue;

                    image.current_token_class[new_coords] = new_token.class_id;
                    image.current_token_id[new_coords] = new_id;
                }
            }
    }
}

void tp::tokenize(int token_max, int tokens_to_create, tg::isize2 const& image_size, cc::string input_folder, cc::string output_folder, int output_folder_count)
{
    LOG("Tokenize");

    // ========================================== Initialization ==========================================

    auto const colors_to_create = tg::max(token_max + tokens_to_create + 1, 2 * image_size.width * image_size.height);
    auto class_colors = generate_colors(colors_to_create);

    LOG("Read input data");
    auto image_data = read_folder(input_folder);

    // not necessary, but nice for debugging purposes:
    // cc::sort(image_data, [](auto const& a, auto const& b) { return a.id < b.id; });

    // output folders
    LOG("Create output folders");
    auto const transcribed_data_folder = output_folder + "transcribed_data/";
    util::make_directories(transcribed_data_folder);
    auto const token_data_folder = output_folder + "tokens/";
    util::make_directories(token_data_folder);
    for (auto const& image : image_data)
    {
        auto const folder = transcribed_data_folder + cc::format("{:06}/{:06}/", image.id % output_folder_count, image.id);
        util::make_directories(folder);
    }

    LOG("Initialize data");
    // global data:
    cc::vector<rule> rules;
    cc::vector<token_data> tokens;

    // init token data, one for each class
    for (auto i = 0; i <= token_max; ++i)
    {
        tokens.push_back({{tg::ipos2(0, 0)}, {i}, i});
    }

    // output debug images
    // write_images(cc::span(image_data).subspan(0, 1), transcribed_data_folder, -1,output_folder_count, class_colors);

    // ========================================= Main Algorithm =========================================

    LOG("Compute tokenization...");
    std::chrono::steady_clock::time_point begin = std::chrono::steady_clock::now();

    for (auto iteration = 0; iteration < tokens_to_create; ++iteration)
    {
        LOG("Iteration {} of {}", iteration + 1, tokens_to_create);

        auto const max_constellation = get_most_common_constellation(image_data);
        auto new_token = combine_tokens(max_constellation, tokens);
        auto new_rule = rule{max_constellation, int(tokens.size())};
        rules.push_back(new_rule);
        tokens.push_back(new_token);
        apply_rule(new_rule, new_token, image_data);

        // output debug images
        // write_images(cc::span(image_data).subspan(0, 1), transcribed_data_folder, iteration,output_folder_count, class_colors);
    }
    std::chrono::steady_clock::time_point end = std::chrono::steady_clock::now();
    LOG("Computation finished. Took {} ms", std::chrono::duration_cast<std::chrono::milliseconds>(end - begin).count());

    LOG("Output token sequences");

    write_token_sequences(image_data, transcribed_data_folder, output_folder_count);

    LOG("Output token shapes");

    write_token_shapes(tokens, token_data_folder);

    LOG("Output token rules");

    write_rules(rules, output_folder);

    LOG("All done! Have a nice day!");
}

void tp::apply_rules(cc::span<rule const> rules, cc::span<token_data const> tokens, cc::span<image_data> images)
{
    for (size_t i = 0; i < rules.size(); ++i)
    {
        auto const& rule = rules[i];
        auto const& new_token = tokens[rule.new_token_id];
        apply_rule(rule, new_token, images);
    }
}

void tp::apply_rules_to_folder(cc::string rule_file, cc::string token_folder, cc::string input_folder, cc::string output_folder, int output_folder_count)
{
    LOG("Apply rules only");

    LOG("Read input files");
    auto rules = read_rules(rule_file);
    auto tokens = read_tokens(token_folder);
    auto images = read_folder(input_folder);

    // output folders
    LOG("Create output folders");
    auto const transcribed_data_folder = output_folder + "transcribed_data/";
    util::make_directories(transcribed_data_folder);
    auto const token_data_folder = output_folder + "tokens/";
    util::make_directories(token_data_folder);
    for (auto const& image : images)
    {
        auto const folder = transcribed_data_folder + cc::format("{:06}/{:06}/", image.id % output_folder_count, image.id);
        util::make_directories(folder);
    }

    LOG("Apply rules");
    apply_rules(rules, tokens, images);

    LOG("Output token sequences");

    write_token_sequences(images, transcribed_data_folder, output_folder_count);
    LOG("All done! Have a nice day!");
}
