#pragma once

#include <clean-core/string_view.hh>

namespace util
{
/// Uses ffmpeg to combine all images in a folder to a video
void render_video(cc::string_view input_folder, int fps, cc::string_view filename = "animation.mp4");
}
