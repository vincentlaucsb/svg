#include "svg.hpp"
#include <fstream>
#include <iostream>
#include <string>
#include <vector>

enum class SignVar {
    panel,
    panel_stroke,
    text,
    tube,
    glow,
    word_stroke,
    scuff,
    screw_fill,
    screw_stroke
};

enum class SignClass {
    shell,
    panel,
    tube,
    word,
    v,
    scuff,
    subtitle,
    screw
};

struct SignTheme {
    std::string title;
    std::string output_path;
    std::string swatch_path;
    SVG::Color accent;

    SVG::Color panel() const { return accent.shade(0.92); }
    SVG::Color panel_stroke() const { return panel().mix(accent, 0.28); }
    SVG::Color text() const { return accent.tint(0.78); }
    SVG::Color glow() const { return accent.tint(0.12); }
    SVG::Color word_stroke() const { return accent.tint(0.92); }
    SVG::Color scuff() const { return text().mix(panel(), 0.16); }
    SVG::Color screw_fill() const { return panel().tint(0.08); }
    SVG::Color screw_stroke() const { return panel().mix(text(), 0.32); }
};

SVG::SVG make_sign(const SignTheme& theme) {
    SVG::SVG root({
        { "xmlns", "http://www.w3.org/2000/svg" },
        { "width", "720" },
        { "height", "260" },
        { "viewBox", "0 0 720 260" },
        { "role", "img" }
    });
    root.add_child<SVG::Title>("SVG for C++ " + theme.title + " neon sign");

    auto vars = root.set_vars<SignVar>({
        { SignVar::panel, "--svg-open-panel", theme.panel() },
        { SignVar::panel_stroke, "--svg-open-panel-stroke", theme.panel_stroke() },
        { SignVar::text, "--svg-open-text", theme.text() },
        { SignVar::tube, "--svg-open-tube", theme.accent },
        { SignVar::glow, "--svg-open-glow", theme.glow() },
        { SignVar::word_stroke, "--svg-open-word-stroke", theme.word_stroke() },
        { SignVar::scuff, "--svg-open-scuff", theme.scuff() },
        { SignVar::screw_fill, "--svg-open-screw-fill", theme.screw_fill() },
        { SignVar::screw_stroke, "--svg-open-screw-stroke", theme.screw_stroke() }
    });
    SVG::Classes<SignClass> classes({
        { SignClass::shell, "sign-shell" },
        { SignClass::panel, "sign-panel" },
        { SignClass::tube, "sign-tube" },
        { SignClass::word, "sign-word" },
        { SignClass::v, "sign-v" },
        { SignClass::scuff, "sign-scuff" },
        { SignClass::subtitle, "sign-subtitle" },
        { SignClass::screw, "sign-screw" }
    });

    root
        .style(classes.selector(SignClass::shell), SVG::Attrs{
            { "transform-box", "fill-box" },
            { "transform-origin", "center" },
            { "transform", "rotate(-1.2deg)" }
        })
        .style(classes.selector(SignClass::panel), SVG::Attrs{
            { "fill", vars.var(SignVar::panel) },
            { "stroke", vars.var(SignVar::panel_stroke) },
            { "stroke-width", "3" },
            { "filter", vars.format("drop-shadow(0 10px 16px {0})", SignVar::glow) }
        })
        .style(classes.selector(SignClass::tube), SVG::Attrs{
            { "animation", "svg-open-hum 5.4s ease-in-out infinite" },
            { "fill", "none" },
            { "opacity", "1" },
            { "stroke", vars.var(SignVar::tube) },
            { "stroke-linejoin", "round" },
            { "stroke-width", "4" }
        })
        .style(classes.selector(SignClass::word), SVG::Attrs{
            { "animation", "svg-open-hum 5.4s ease-in-out infinite" },
            { "dominant-baseline", "middle" },
            { "fill", vars.var(SignVar::text) },
            { "filter", vars.format("drop-shadow(0 0 16px {0})", SignVar::glow) },
            { "font-family", "Arial, Helvetica, sans-serif" },
            { "font-size", "150px" },
            { "font-weight", "700" },
            { "paint-order", "stroke fill" },
            { "stroke", vars.var(SignVar::word_stroke) },
            { "stroke-width", "1.8" },
            { "text-anchor", "middle" }
        })
        .style(classes.selector(SignClass::v), SVG::Attrs{
            { "animation", "svg-open-v-flicker 7.8s ease-in-out infinite" }
        })
        .style(classes.selector(SignClass::scuff), SVG::Attrs{
            { "opacity", "0.34" },
            { "stroke", vars.var(SignVar::scuff) },
            { "stroke-linecap", "round" },
            { "stroke-width", "1.4" }
        })
        .style(classes.selector(SignClass::subtitle), SVG::Attrs{
            { "dominant-baseline", "middle" },
            { "fill", vars.var(SignVar::text) },
            { "filter", vars.format("drop-shadow(0 0 8px {0})", SignVar::glow) },
            { "font-family", "cursive" },
            { "font-size", "34px" },
            { "font-weight", "400" },
            { "opacity", "0.82" },
            { "text-anchor", "start" }
        })
        .style(classes.selector(SignClass::screw), SVG::Attrs{
            { "fill", vars.var(SignVar::screw_fill) },
            { "opacity", "0.72" },
            { "stroke", vars.var(SignVar::screw_stroke) },
            { "stroke-width", "1.5" }
        })
        .media_style(
            "(prefers-reduced-motion: reduce)",
            classes.selector(SignClass::tube),
            SVG::Attrs{{ "animation", "none" }})
        .media_style(
            "(prefers-reduced-motion: reduce)",
            classes.selector(SignClass::word),
            SVG::Attrs{{ "animation", "none" }})
        .media_style(
            "(prefers-reduced-motion: reduce)",
            classes.selector(SignClass::v),
            SVG::Attrs{{ "animation", "none" }});
    root.keyframes("svg-open-hum")["0%, 100%"].set_attrs({
        { "opacity", "0.76" },
        { "filter", vars.format("drop-shadow(0 0 8px {0})", SignVar::glow) }
    });
    root.keyframes("svg-open-hum")["45%"].set_attrs({
        { "opacity", "1" },
        { "filter", vars.format("drop-shadow(0 0 22px {0})", SignVar::glow) }
    });
    root.keyframes("svg-open-hum")["62%"].set_attrs({
        { "opacity", "0.88" },
        { "filter", vars.format("drop-shadow(0 0 12px {0})", SignVar::glow) }
    });
    auto& v_flicker = root.keyframes("svg-open-v-flicker");
    v_flicker["0%"].set_attrs({
        { "opacity", "0.18" },
        { "filter", "none" }
    });
    v_flicker["10%, 72%, 82%, 100%"].set_attrs({
        { "opacity", "1" },
        { "filter", vars.format("drop-shadow(0 0 22px {0})", SignVar::glow) }
    });
    v_flicker["75%, 79%"].set_attrs({
        { "opacity", "0.18" },
        { "filter", "none" }
    });

    auto* sign = root.add_child<SVG::Group>(SVG::Attrs{{ "class", classes.classes(SignClass::shell) }});

    sign->add_child<SVG::Rect>(
        24, 24, 672, 212,
        SVG::Attrs{{ "class", classes.classes(SignClass::panel) }, { "rx", "28" }});

    sign->add_child<SVG::Rect>(
        44, 42, 632, 176,
        SVG::Attrs{{ "class", classes.classes(SignClass::tube) }, { "rx", "22" }});

    sign->add_child<SVG::Circle>(64, 58, 6, SVG::Attrs{{ "class", classes.classes(SignClass::screw) }});
    sign->add_child<SVG::Circle>(656, 58, 6, SVG::Attrs{{ "class", classes.classes(SignClass::screw) }});

    sign->add_child<SVG::Line>(86, 132, 190, 178, SVG::Attrs{{ "class", classes.classes(SignClass::scuff) }});
    sign->add_child<SVG::Line>(586, 632, 70, 62, SVG::Attrs{{ "class", classes.classes(SignClass::scuff) }});

    auto* word = sign->add_child<SVG::G>(SVG::Attrs{{ "class", classes.classes(SignClass::word) }});
    word->add_child<SVG::Text>(250, 128, "S");
    word->add_child<SVG::Text>(
        360, 128, "V",
        SVG::Attrs{{ "class", classes.classes(SignClass::v) }});
    word->add_child<SVG::Text>(470, 128, "G");
    sign->add_child<SVG::Text>(518, 182, "for C++", SVG::Attrs{{ "class", classes.classes(SignClass::subtitle) }});

    return root;
}

SVG::SVG make_swatch(const SignTheme& theme) {
    SVG::SVG root({
        { "xmlns", "http://www.w3.org/2000/svg" },
        { "width", "64" },
        { "height", "18" },
        { "viewBox", "0 0 64 18" },
        { "role", "img" }
    });
    root.add_child<SVG::Title>(theme.title + " logo color");
    root.add_child<SVG::Rect>(
        1, 1, 62, 16,
        SVG::Attrs{
            { "fill", theme.accent.str() },
            { "rx", "4" },
            { "stroke", theme.accent.tint(0.68).str() },
            { "stroke-width", "2" }
        });
    return root;
}

int write_svg(const std::string& output_path, SVG::SVG root) {
    std::ofstream output(output_path, std::ios::binary);
    if (!output) {
        std::cerr << "Unable to open " << output_path << " for writing.\n";
        return 1;
    }

    output << std::string(root);
    return 0;
}

int write_sign(const SignTheme& theme) {
    return write_svg(theme.output_path, make_sign(theme));
}

int write_swatch(const SignTheme& theme) {
    return write_svg(theme.swatch_path, make_swatch(theme));
}

int main() {
    const std::vector<SignTheme> themes = {
        { "pink", "examples/svg-open-sign.svg", "examples/svg-open-sign-swatch.svg", SVG::Color::hex("#ff4fd8") },
        { "green", "examples/svg-open-sign-green.svg", "examples/svg-open-sign-green-swatch.svg", SVG::Color::hex("#44ff88") },
        { "blue", "examples/svg-open-sign-blue.svg", "examples/svg-open-sign-blue-swatch.svg", SVG::Color::hex("#4fa8ff") },
        { "red", "examples/svg-open-sign-red.svg", "examples/svg-open-sign-red-swatch.svg", SVG::Color::hex("#ff4f5e") }
    };

    for (const auto& theme : themes) {
        if (write_sign(theme) != 0) {
            return 1;
        }
        if (write_swatch(theme) != 0) {
            return 1;
        }
    }
}
