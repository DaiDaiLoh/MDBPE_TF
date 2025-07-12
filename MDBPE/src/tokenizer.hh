#pragma once

#include <clean-core/span.hh>

#include "constellation.hh"
#include "image_data.hh"
#include "rule.hh"
#include "token_data.hh"

namespace tp
{
/// tokenize the given images
void tokenize(int token_max, int tokens_to_create, tg::isize2 const& image_size, cc::string input_folder, cc::string output_folder, int output_folder_count);

/// apply a set of already computed tokens to a set of input images
void apply_rules(cc::span<const rule> rules, cc::span<token_data const> tokens, cc::span<image_data> images);

token_data combine_tokens(constellation const& rule, cc::span<token_data const> tokens);

/// same as above, but reads the rules from a file
void apply_rules_to_folder(cc::string rule_file, cc::string token_folder, cc::string input_folder, cc::string output_folder, int output_folder_count);

/// returns the most common constellation in the given images
constellation get_most_common_constellation(cc::span<image_data const> images);

/// applies the given rule to all images
void apply_rule(rule const& rule, token_data const& new_token, cc::span<image_data> images);
}
