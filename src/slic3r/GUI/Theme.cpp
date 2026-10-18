#include "Theme.hpp"

#include <algorithm>
#include <cmath>
#include <cstdio>

#include "libslic3r/AppConfig.hpp"
#include "libslic3r/Color.hpp"
#include "3DBed.hpp"
#include "GLCanvas3D.hpp"
#include "ImGuiWrapper.hpp"
#include "PartPlate.hpp"
#include "Widgets/StateColor.hpp"

namespace Slic3r {
namespace GUI {

namespace {

// Dark surface the accent tints are mixed into (the classic dark window color).
const wxColour kDarkSurface(45, 45, 49);

wxColour mix(const wxColour &a, const wxColour &b, float t) // t = share of b
{
    auto ch = [t](unsigned char x, unsigned char y) { return (unsigned char) std::lround(x * (1.f - t) + y * t); };
    return wxColour(ch(a.Red(), b.Red()), ch(a.Green(), b.Green()), ch(a.Blue(), b.Blue()));
}

wxColour shift_lightness(const wxColour &c, float delta)
{
    float h, s, l;
    StateColor::ToHSL(c, h, s, l);
    return StateColor::FromHSL(h, s, l + delta);
}

ColorTheme classic(const char *id, const char *name, const char *pair, bool dark)
{
    ColorTheme t;
    t.id = id; t.name = name; t.pair_id = pair; t.dark = dark; t.classic = true;
    t.accent = wxColour("#009688");
    t.accent_hover = wxColour("#26A69A");
    t.accent_soft = wxColour("#E5F0EE");
    t.accent_soft_strong = wxColour("#BFE1DE");
    t.accent_text = wxColour("#009688");
    return t;
}

ColorTheme light_theme(const char *id, const char *name, const char *pair, const char *accent_hex)
{
    ColorTheme t;
    t.id = id; t.name = name; t.pair_id = pair; t.dark = false;
    t.accent = wxColour(accent_hex);
    t.accent_hover = shift_lightness(t.accent, 0.10f);
    t.accent_soft = mix(wxColour(255, 255, 255), t.accent, 0.10f);
    t.accent_soft_strong = mix(wxColour(255, 255, 255), t.accent, 0.25f);
    t.accent_text = t.accent;
    float l, s;
    StateColor::ToHSL(t.accent, t.tint_hue, s, l);
    t.tint_sat = 0.14f; // plates, bed and 3D background pick up a light touch of the accent hue
    return t;
}

ColorTheme dark_theme(const char *id, const char *name, const char *pair, const char *accent_hex, float hue, float sat)
{
    ColorTheme t;
    t.id = id; t.name = name; t.pair_id = pair; t.dark = true;
    t.tint_hue = hue; t.tint_sat = sat;
    t.accent = wxColour(accent_hex);
    t.accent_hover = shift_lightness(t.accent, 0.07f);
    const wxColour surface = StateColor::FromHSL(hue, sat, 0.18f);
    t.accent_soft = mix(surface, t.accent, 0.12f);
    t.accent_soft_strong = mix(surface, t.accent, 0.25f);
    float h, s, l;
    StateColor::ToHSL(t.accent, h, s, l);
    t.accent_text = StateColor::FromHSL(h, std::min(s, 0.85f), 0.60f);
    return t;
}

std::vector<ColorTheme> make_themes()
{
    std::vector<ColorTheme> v;
    v.push_back(classic("classic_light", "Classic Light", "classic_dark", false));
    v.push_back(light_theme("stratum_light", "Stratum Teal", "midnight_teal", "#0D9488"));
    v.push_back(light_theme("ocean_light", "Ocean Blue", "deep_navy", "#2563EB"));
    v.push_back(light_theme("violet_light", "Violet", "nebula_purple", "#7C3AED"));
    v.push_back(light_theme("ember_light", "Ember Orange", "graphite_amber", "#EA580C"));
    v.push_back(classic("classic_dark", "Classic Dark", "classic_light", true));
    v.push_back(dark_theme("midnight_teal", "Midnight Teal", "stratum_light", "#0F766E", 185.f, 0.25f));
    v.push_back(dark_theme("deep_navy", "Deep Navy", "ocean_light", "#1D4ED8", 220.f, 0.32f));
    v.push_back(dark_theme("nebula_purple", "Nebula Purple", "violet_light", "#6D28D9", 268.f, 0.26f));
    v.push_back(dark_theme("graphite_amber", "Graphite Amber", "ember_light", "#B45309", 30.f, 0.05f));
    return v;
}

std::string g_current_id;

ColorRGBA hsl_rgba(float hue, float sat, float light)
{
    const wxColour c = StateColor::FromHSL(hue, sat, light);
    return ColorRGBA(c.Red() / 255.f, c.Green() / 255.f, c.Blue() / 255.f, 1.f);
}

ImVec4 to_imvec4(const wxColour &c)
{
    return ImVec4(c.Red() / 255.f, c.Green() / 255.f, c.Blue() / 255.f, 1.f);
}

// The 3D scene colors a theme changes. Captured once, before the first theme is applied, so the
// classic themes can put the original values back.
struct SceneColors
{
    ColorRGBA select, unselect, unselect_dark, line_top, line_top_dark, line_top_sel, line_top_sel_dark;
    ColorRGBA bed_model, bed_model_dark;
};

const SceneColors &scene_defaults()
{
    static const SceneColors d = {
        PartPlate::SELECT_COLOR, PartPlate::UNSELECT_COLOR, PartPlate::UNSELECT_DARK_COLOR,
        PartPlate::LINE_TOP_COLOR, PartPlate::LINE_TOP_DARK_COLOR,
        PartPlate::LINE_TOP_SEL_COLOR, PartPlate::LINE_TOP_SEL_DARK_COLOR,
        Bed3D::DEFAULT_MODEL_COLOR, Bed3D::DEFAULT_MODEL_COLOR_DARK};
    return d;
}

void apply_scene_colors(const ColorTheme &t)
{
    const SceneColors &d = scene_defaults();
    PartPlate::SELECT_COLOR = d.select;
    PartPlate::UNSELECT_COLOR = d.unselect;
    PartPlate::UNSELECT_DARK_COLOR = d.unselect_dark;
    PartPlate::LINE_TOP_COLOR = d.line_top;
    PartPlate::LINE_TOP_DARK_COLOR = d.line_top_dark;
    PartPlate::LINE_TOP_SEL_COLOR = d.line_top_sel;
    PartPlate::LINE_TOP_SEL_DARK_COLOR = d.line_top_sel_dark;
    Bed3D::DEFAULT_MODEL_COLOR = d.bed_model;
    Bed3D::DEFAULT_MODEL_COLOR_DARK = d.bed_model_dark;
    GLCanvas3D::set_theme_background(nullptr, nullptr);

    if (t.classic)
        return;

    // Same lightness as the classic colors, tinted with the theme hue, so the plate, its grid lines,
    // the bed and the 3D backdrop all belong to the theme.
    const float h = t.tint_hue, s = t.tint_sat;
    PartPlate::SELECT_COLOR = hsl_rgba(h, s, 0.27f);
    if (t.dark) {
        PartPlate::UNSELECT_DARK_COLOR = hsl_rgba(h, s, 0.40f);
        PartPlate::LINE_TOP_DARK_COLOR = hsl_rgba(h, s, 0.45f);
        PartPlate::LINE_TOP_SEL_DARK_COLOR = hsl_rgba(h, s, 0.32f);
        Bed3D::DEFAULT_MODEL_COLOR_DARK = hsl_rgba(h, s, 0.27f);
        const ColorRGBA bg = hsl_rgba(h, s, 0.35f);
        GLCanvas3D::set_theme_background(nullptr, &bg);
    } else {
        PartPlate::UNSELECT_COLOR = hsl_rgba(h, s, 0.82f);
        PartPlate::LINE_TOP_COLOR = hsl_rgba(h, s, 0.89f);
        PartPlate::LINE_TOP_SEL_COLOR = hsl_rgba(h, s * 0.8f, 0.54f);
        Bed3D::DEFAULT_MODEL_COLOR = hsl_rgba(h, s * 1.2f, 0.33f);
        const ColorRGBA bg = hsl_rgba(h, s * 0.9f, 0.91f);
        GLCanvas3D::set_theme_background(&bg, nullptr);
    }
}

void apply_imgui_colors(const ColorTheme &t)
{
    static const ImVec4 orca = ImGuiWrapper::COL_ORCA, orca_dark = ImGuiWrapper::COL_ORCA_DARK,
                        orca_hover = ImGuiWrapper::COL_ORCA_HOVER, orca_hover_dark = ImGuiWrapper::COL_ORCA_HOVER_DARK;
    if (t.classic) {
        ImGuiWrapper::COL_ORCA = orca;
        ImGuiWrapper::COL_ORCA_DARK = orca_dark;
        ImGuiWrapper::COL_ORCA_HOVER = orca_hover;
        ImGuiWrapper::COL_ORCA_HOVER_DARK = orca_hover_dark;
        return;
    }
    ImGuiWrapper::COL_ORCA = to_imvec4(t.accent_text);
    if (t.dark) {
        ImGuiWrapper::COL_ORCA_DARK = to_imvec4(t.accent);
        ImGuiWrapper::COL_ORCA_HOVER = to_imvec4(shift_lightness(t.accent_text, 0.08f));
        ImGuiWrapper::COL_ORCA_HOVER_DARK = to_imvec4(t.accent_hover);
    } else {
        ImGuiWrapper::COL_ORCA_DARK = to_imvec4(shift_lightness(t.accent, -0.08f));
        ImGuiWrapper::COL_ORCA_HOVER = to_imvec4(t.accent_hover);
        ImGuiWrapper::COL_ORCA_HOVER_DARK = to_imvec4(t.accent);
    }
}

} // namespace

const std::vector<ColorTheme> &Theme::all()
{
    static const std::vector<ColorTheme> themes = make_themes();
    return themes;
}

const ColorTheme &Theme::find(const std::string &id, bool dark)
{
    for (const ColorTheme &t : all())
        if (t.id == id)
            return t;
    return all()[dark ? 5 : 0]; // classic_dark / classic_light
}

const ColorTheme &Theme::current()
{
    return find(g_current_id, false);
}

void Theme::load_from_config(const AppConfig &config)
{
    const bool dark = config.get("dark_color_mode") == "1";
    const ColorTheme &t = find(config.get("color_theme"), dark);
    // A theme of the other mode (e.g. dark_color_mode was changed elsewhere) gives way to its partner.
    g_current_id = (t.dark == dark) ? t.id : find(t.pair_id, dark).id;
}

void Theme::select(AppConfig &config, const std::string &id)
{
    const ColorTheme &t = find(id, config.get("dark_color_mode") == "1");
    g_current_id = t.id;
    config.set("color_theme", t.id);
    config.set("dark_color_mode", t.dark ? "1" : "0");
}

void Theme::select_mode(AppConfig &config, bool dark)
{
    const ColorTheme &cur = find(config.get("color_theme"), !dark);
    const ColorTheme &t = (cur.dark == dark) ? cur : find(cur.pair_id, dark);
    g_current_id = t.id;
    config.set("color_theme", t.id);
}

std::string Theme::hex(const wxColour &c)
{
    char buf[8];
    std::snprintf(buf, sizeof(buf), "#%02X%02X%02X", c.Red(), c.Green(), c.Blue());
    return buf;
}

void Theme::apply()
{
    const ColorTheme &t = current();

    StateColorTheme sc;
    sc.enabled = !t.classic;
    sc.dark = t.dark;
    sc.accent = t.accent;
    sc.accent_hover = t.accent_hover;
    sc.accent_soft = t.accent_soft;
    sc.accent_soft_strong = t.accent_soft_strong;
    sc.accent_text = t.accent_text;
    sc.tint_hue = t.tint_hue;
    sc.tint_sat = t.tint_sat;
    StateColor::SetTheme(sc);

    apply_imgui_colors(t);
    apply_scene_colors(t);
}

} // namespace GUI
} // namespace Slic3r
