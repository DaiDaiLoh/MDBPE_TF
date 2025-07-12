#include <cstdlib>

#include <clean-core/string.hh>

#include <typed-geometry/types/size.hh>

#include <tokenizer.hh>

int main(int /*argc*/, char** /*args*/)
{
    // ============================================== Config ==============================================

    auto const token_max = 255;                       // of input tokens; 255 means we have 256 tokens, i.e. [0, 255] (e.g. for MNIST without VQ-VAE)
    auto const tokens_to_create = 32;                 // number of rules / new tokens to create (usually, something between 128 and 512 is good)
    auto const image_dimensions = tg::isize2(12, 12); // width and height of the images
    auto const output_folder_count = 128;            // number of folders to create in the output folder. for ImageNet, you may want this to be 1024 or something; make sure we don't put 500000 files into one folder :)

    cc::string const input_folder = "../data/data_cpp/"; // this should be the exported sequence of tokens, e.g. exported VQ-VAE tokens (see what the python processor does for reference!)
    cc::string const output_folder = "../data/data_cpp_out/";

    tp::tokenize(token_max, tokens_to_create, image_dimensions, input_folder, output_folder, output_folder_count);

    // ============================================== Apply Rules =========================================

    cc::string const test_set_input_folder = input_folder;
    cc::string const test_set_output_folder = "../data/data_cpp_test_out/";
    cc::string const rule_file = "../data/data_cpp_out/rules.dat";
    cc::string const token_folder = "../data/data_cpp_out/tokens/";

    tp::apply_rules_to_folder(rule_file, token_folder, test_set_input_folder, test_set_output_folder, output_folder_count);

    return EXIT_SUCCESS;
}
