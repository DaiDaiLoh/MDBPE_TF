#include "render_video.hh"

#include <cstdlib>

#include <clean-core/format.hh>

void util::render_video(cc::string_view input_folder, int fps, cc::string_view filename)
{
    // make sure folder ends in /, but ONLY iff it is not empty!
    // otherwise we might let the folder empty and then write to the filesystem root unintended.
    // instead prefer the default behaviour to write to the working directory
    cc::string folder = input_folder;
    if (!folder.empty() && !folder.ends_with("/"))
        folder += "/";

    auto command = cc::format("ffmpeg -r {} -f image2 -i {}'%*.png' -c:v libx264 -pix_fmt yuv420p -y {}{}", fps, folder, folder, filename);

    std::system(command.c_str());
}
