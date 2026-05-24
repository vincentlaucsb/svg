#include "svg.hpp"
#include <fstream>
#include <iostream>

enum class SignVar {
    panel,
    panel_stroke,
    text,
    tube,
    glow
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

int main() {
    SVG::SVG root({
        { "xmlns", "http://www.w3.org/2000/svg" },
        { "width", "720" },
        { "height", "260" },
        { "viewBox", "0 0 720 260" },
        { "role", "img" }
    });
    root.add_child<SVG::Title>("SVG for C++");

    auto vars = root.set_vars<SignVar>({
        { SignVar::panel, "--svg-open-panel", "#130715" },
        { SignVar::panel_stroke, "--svg-open-panel-stroke", "#56314d" },
        { SignVar::text, "--svg-open-text", "#ffd7f5" },
        { SignVar::tube, "--svg-open-tube", "#ff4fd8" },
        { SignVar::glow, "--svg-open-glow", "rgba(255, 79, 216, 0.42)" }
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
            { "stroke", "#e8fdff" },
            { "stroke-width", "1.8" },
            { "text-anchor", "middle" }
        })
        .style(classes.selector(SignClass::v), SVG::Attrs{
            { "animation", "svg-open-v-flicker 7.8s ease-in-out infinite" }
        })
        .style(classes.selector(SignClass::scuff), SVG::Attrs{
            { "opacity", "0.34" },
            { "stroke", "#f4d8ef" },
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
            { "fill", "#20111f" },
            { "opacity", "0.72" },
            { "stroke", "#76566f" },
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

    std::ofstream output("examples/svg-open-sign.svg", std::ios::binary);
    if (!output) {
        std::cerr << "Unable to open examples/svg-open-sign.svg for writing.\n";
        return 1;
    }

    output << std::string(root);
}
