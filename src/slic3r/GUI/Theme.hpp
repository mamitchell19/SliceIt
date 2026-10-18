#ifndef slic3r_GUI_Theme_hpp_
#define slic3r_GUI_Theme_hpp_

#include <string>
#include <vector>
#include <wx/colour.h>

namespace Slic3r {

class AppConfig;

namespace GUI {

// A color theme is either a light or a dark theme. Each light theme has a dark partner (pair_id) so
// the plain light/dark switch keeps the same accent. The classic themes are the original look.
struct ColorTheme
{
    std::string id;
    std::string name;
    std::string pair_id;
    bool        dark    = false;
    bool        classic = false;

    wxColour accent;             // buttons, toggles, selected tabs
    wxColour accent_hover;
    wxColour accent_soft;        // ~10% accent over the surface
    wxColour accent_soft_strong; // ~25% accent over the surface
    wxColour accent_text;        // links and text drawn on the surface

    float tint_hue = 0.f;        // surface / build plate tint: hue 0-360 ...
    float tint_sat = 0.f;        // ... and saturation 0-1
};

class Theme
{
public:
    static const std::vector<ColorTheme> &all();

    // Falls back to the classic theme of the requested mode when the id is unknown or empty.
    static const ColorTheme &find(const std::string &id, bool dark);
    static const ColorTheme &current();

    // Reads "color_theme" (and "dark_color_mode" as the fallback mode) from the app config.
    static void load_from_config(const AppConfig &config);

    // Makes the theme current and stores it in the config together with its light/dark mode.
    static void select(AppConfig &config, const std::string &id);

    // Light/dark switch: selects the partner of the current theme in the other mode.
    static void select_mode(AppConfig &config, bool dark);

    // Pushes the current theme into the widget color tables, the icon recoloring, the ImGui accent
    // colors and the 3D scene (build plate, bed, grid, background).
    static void apply();

    // "#RRGGBB"
    static std::string hex(const wxColour &color);
};

} // namespace GUI
} // namespace Slic3r

#endif // slic3r_GUI_Theme_hpp_
