#include "debug.hh"

#include <sstream>

#include "image2D.hh"

std::string glow::vector::to_string(const image2D& img)
{
    static char const* caps[] = {"butt", "round", "square"};
    static char const* joins[] = {"miter", "round", "bevel"};

    std::stringstream ss;
    struct printer
    {
        std::stringstream& ss;

        void begin_path() { ss << "begin_path\n"; }
        void close_path() { ss << "close_path\n"; }

        void move_to(float x, float y) { ss << "move_to " << x << " " << y << "\n"; }
        void line_to(float x, float y) { ss << "line_to " << x << " " << y << "\n"; }
        void bezier_to(float c1x, float c1y, float c2x, float c2y, float x, float y)
        {
            ss << "bezier_to " << c1x << " " << c1y << " " << c2x << " " << c2y << " " << x << " " << y << "\n";
        }
        void quad_to(float cx, float cy, float x, float y) { ss << "quad_to " << cx << " " << cy << " " << x << " " << y << "\n"; }
        void arc_to(float x1, float y1, float x2, float y2, float radius)
        {
            ss << "arc_to " << x1 << " " << y1 << " " << x2 << " " << y2 << " " << radius << "\n";
        }

        // TODO: arc, rounded_rect_varying, ellipse
        void rect(float x, float y, float w, float h) { ss << "rect " << x << " " << y << " " << w << " " << h << "\n"; }
        void rounded_rect(float x, float y, float w, float h, float r)
        {
            ss << "rounded_rect " << x << " " << y << " " << w << " " << h << " " << r << "\n";
        }
        void circle(float cx, float cy, float r) { ss << "circle " << cx << " " << cy << " " << r << "\n"; }

        void stroke() { ss << "stroke\n"; }
        void fill() { ss << "fill\n"; }

        void set_stroke_width(float s) { ss << "set_stroke_width " << s << "\n"; }
        void set_stroke_color(float r, float g, float b, float a) { ss << "set_stroke_color " << r << " " << g << " " << b << " " << a << "\n"; }
        void set_fill_color(float r, float g, float b, float a) { ss << "set_fill_color " << r << " " << g << " " << b << " " << a << "\n"; }
        void set_line_cap(line_cap cap) { ss << "set_line_cap " << caps[int(cap)] << "\n"; }
        void set_line_join(line_join join) { ss << "set_line_join " << joins[int(join)] << "\n"; }
        void set_miter_limit(float limit) { ss << "set_miter_limit " << limit << "\n"; }
        void set_stroke_paint(paint2D const& p) { ss << "set_stroke_paint [TODO]\n"; }
        void set_fill_paint(paint2D const& p) { ss << "set_fill_paint [TODO]\n"; }

        void set_font_size(float s) { ss << "set_font_size " << s << "\n"; }
        void set_font_blur(float s) { ss << "set_font_blur " << s << "\n"; }
        void set_text_letter_spacing(float s) { ss << "set_text_letter_spacing " << s << "\n"; }
        void set_text_line_height(float s) { ss << "set_text_line_height " << s << "\n"; }
        void set_text_align(text_align a) { ss << "set_text_align " << int(a) << "\n"; }
        void set_font_face(char const* f) { ss << "set_font_face \"" << f << "\"\n"; }
        void text(float x, float y, char const* s) { ss << "text " << x << " " << y << " \"" << s << "\"\n"; }
        void text_box(float x, float y, float max_w, char const* s) { ss << "text " << x << " " << y << " " << max_w << " \"" << s << "\"\n"; }

        void clip_rect(float x, float y, float w, float h) { ss << "clip_rect " << x << " " << y << " " << w << " " << h << "\n"; }
        void reset_clip() { ss << "reset_clip\n"; }

        void global_alpha(float a) { ss << "global_alpha " << a << "\n"; }
    };
    visit(img, printer{ss});
    return ss.str();
}
