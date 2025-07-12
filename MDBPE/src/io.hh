#pragma once

#include "clean-core/string_view.hh"

#include <image/image.hh>

#include "image_data.hh"
#include "rule.hh"
#include "token_data.hh"

namespace tp
{
/// reads a single .dat file into a integer class image
img::image<int> read_token_bin_data(cc::string_view filepath);

/// read all .dat files in the given folder into images
cc::vector<image_data> read_folder(cc::string_view folder);

/// write an integer image, mapping each integer to a color given by 'colors'
void write(cc::string_view filepath, img::image<int> const& image, cc::span<tg::color3 const> colors);

/// same as above, but generate 'max_colors' colors
void write(cc::string_view filepath, img::image<int> const& image, int max_colors);

/// write all debug images
void write_images(cc::span<image_data const> images, cc::string_view folder, int iteration, int output_folder_count, cc::span<tg::color3 const> class_color);

/// write all token sequences into the output folder, using 'folder_modulus' folders
void write_token_sequences(cc::span<image_data const> image_dat, cc::string output_folder, int folder_modulus);

/// write all token shapes into the token data folder
void write_token_shapes(cc::span<token_data const> tokens, cc::string token_data_folder);

/// write all rules into the output folder
void write_rules(cc::span<rule const> rules, cc::string output_folder);

/// read all tokens in the given folder
cc::vector<token_data> read_tokens(cc::string token_folder);

/// read all rules in the given file
cc::vector<rule> read_rules(cc::string rule_file);
}
