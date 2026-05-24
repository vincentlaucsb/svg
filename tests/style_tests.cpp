#include <catch2/catch.hpp>
#include "svg.hpp"
#include "test_helpers.hpp"

namespace {
    enum class PlotVar {
        axis,
        background,
        text_size,
        header_multiplier
    };

    enum class PlotClass {
        axis_line,
        label,
        muted
    };
}

TEST_CASE("CSS styles serialize before document content", "[style]") {
    SVG::SVG root = two_circles();
    root.style("circle").set_attr("fill", "#000000").set_attr("stroke", "#000000");
    std::string correct = "<svg xmlns=\"http://www.w3.org/2000/svg\">\n"
        "\t<style type=\"text/css\">\n"
        "\t\t<![CDATA[\n"
        "\t\t\tcircle {\n"
        "\t\t\t\tfill: #000000;\n"
        "\t\t\t\tstroke: #000000;\n"
        "\t\t\t}\n"
        "\t\t]]>\n"
        "\t</style>\n"
        "\t<g>\n"
        "\t\t<circle cx=\"0.0\" cy=\"0.0\" r=\"0.0\" />\n"
        "\t\t<circle cx=\"0.0\" cy=\"0.0\" r=\"0.0\" />\n"
        "\t</g>\n"
        "</svg>";

    REQUIRE(std::string(root) == correct);
}

TEST_CASE("Media query styles serialize into embedded CSS", "[style]") {
    SVG::SVG root;
    root.style(":root").set_attr("--text", "#111827");
    root.media_style("(prefers-color-scheme: dark)", ":root").set_attr("--text", "#e5e7eb");

    const auto output = std::string(root);
    REQUIRE(output.find(":root {\n\t\t\t\t--text: #111827;\n\t\t\t}") != std::string::npos);
    REQUIRE(output.find("@media (prefers-color-scheme: dark) {\n") != std::string::npos);
    REQUIRE(output.find(":root {\n\t\t\t\t\t--text: #e5e7eb;\n\t\t\t\t}") != std::string::npos);
}

TEST_CASE("Style Attrs overloads chain on the SVG root", "[style]") {
    SVG::SVG root;
    root
        .style(".axis", SVG::Attrs{{ "stroke", "#374151" }, { "stroke-width", "1.2" }})
        .style(".label", SVG::Attrs{{ "fill", "#111827" }})
        .media_style("(prefers-color-scheme: dark)", ".label", SVG::Attrs{{ "fill", "#f9fafb" }});

    const auto output = std::string(root);

    REQUIRE(output.find(".axis {\n\t\t\t\tstroke: #374151;\n\t\t\t\tstroke-width: 1.2;\n\t\t\t}") !=
            std::string::npos);
    REQUIRE(output.find(".label {\n\t\t\t\tfill: #111827;\n\t\t\t}") != std::string::npos);
    REQUIRE(output.find("@media (prefers-color-scheme: dark) {\n") != std::string::npos);
    REQUIRE(output.find(".label {\n\t\t\t\t\tfill: #f9fafb;\n\t\t\t\t}") != std::string::npos);
}

TEST_CASE("Keyframes serialize alongside regular styles", "[style]") {
    SVG::SVG root;
    root.style(".bar").set_attr("fill", "var(--bar-fill)");
    root.keyframes("fade")["0%"].set_attr("opacity", 0);
    root.keyframes("fade")["100%"].set_attr("opacity", 1);

    const auto output = std::string(root);
    REQUIRE(output.find(".bar {\n\t\t\t\tfill: var(--bar-fill);\n\t\t\t}") != std::string::npos);
    REQUIRE(output.find("@keyframes fade {\n") != std::string::npos);
    REQUIRE(output.find("0% {\n\t\t\t\t\topacity: 0;\n\t\t\t\t}") != std::string::npos);
    REQUIRE(output.find("100% {\n\t\t\t\t\topacity: 1;\n\t\t\t\t}") != std::string::npos);
}

TEST_CASE("Typed CSS variables serialize and produce typed references", "[style]") {
    SVG::SVG root;
    auto vars = root.set_vars<PlotVar>({
        { PlotVar::axis, "--svgplot-axis", "#374151" },
        { PlotVar::background, "svgplot-background", "#fff" },
        { PlotVar::text_size, "--svgplot-text-size" },
        { PlotVar::header_multiplier, "--svgplot-header-multiplier", "2" }
    });
    vars.set(PlotVar::text_size, "12px");

    auto* rect = root.add_child<SVG::Rect>();
    rect->set_attr("fill", vars.var(PlotVar::background));
    auto* text = root.add_child<SVG::Text>(0, 0, "Header");
    text->set_attr("font-size", vars.format("calc({0} * {1})",
                                            PlotVar::text_size,
                                            PlotVar::header_multiplier));

    const auto output = std::string(root);

    REQUIRE(vars.name(PlotVar::background) == "--svgplot-background");
    REQUIRE(vars.var(PlotVar::axis) == "var(--svgplot-axis)");
    REQUIRE(output.find(":root {\n") != std::string::npos);
    REQUIRE(output.find("--svgplot-axis: #374151;") != std::string::npos);
    REQUIRE(output.find("--svgplot-background: #fff;") != std::string::npos);
    REQUIRE(output.find("--svgplot-text-size: 12px;") != std::string::npos);
    REQUIRE(output.find("fill=\"var(--svgplot-background)\"") != std::string::npos);
    REQUIRE(output.find("font-size=\"calc(var(--svgplot-text-size) * var(--svgplot-header-multiplier))\"") !=
            std::string::npos);
}

TEST_CASE("Typed CSS variables support media scoped values", "[style]") {
    SVG::SVG root;
    auto vars = root.set_vars<PlotVar>("(prefers-color-scheme: dark)", ":root", {
        { PlotVar::axis, "--svgplot-axis", "#9ca3af" },
        { PlotVar::background, "--svgplot-background", "#111827" }
    });

    const auto output = std::string(root);

    REQUIRE(vars.var(PlotVar::background) == "var(--svgplot-background)");
    REQUIRE(output.find("@media (prefers-color-scheme: dark) {\n") != std::string::npos);
    REQUIRE(output.find("--svgplot-axis: #9ca3af;") != std::string::npos);
    REQUIRE(output.find("--svgplot-background: #111827;") != std::string::npos);
}

TEST_CASE("Typed CSS variables reject ambiguous mappings and bad formats", "[style]") {
    SVG::SVG root;
    REQUIRE_THROWS_AS(root.set_vars<PlotVar>({
        { PlotVar::axis, "--svgplot-axis" },
        { PlotVar::axis, "--svgplot-axis-copy" }
    }), std::invalid_argument);
    REQUIRE_THROWS_AS(root.set_vars<PlotVar>({
        { PlotVar::axis, "--svgplot-axis" },
        { PlotVar::background, "svgplot-axis" }
    }), std::invalid_argument);

    auto vars = root.set_vars<PlotVar>({
        { PlotVar::axis, "svgplot-axis" },
        { PlotVar::text_size, "--svgplot-text-size" }
    });

    REQUIRE(vars.format("{{{0}}}", PlotVar::axis) == "{var(--svgplot-axis)}");
    REQUIRE_THROWS_AS(vars.var(PlotVar::background), std::invalid_argument);
    REQUIRE_THROWS_AS(vars.format("calc({0} * {1})", PlotVar::axis), std::invalid_argument);
    REQUIRE_THROWS_AS(vars.format("calc({0})", PlotVar::axis, PlotVar::text_size), std::invalid_argument);
    REQUIRE_THROWS_AS(vars.format("calc({0)", PlotVar::axis), std::invalid_argument);
}

TEST_CASE("Typed CSS classes produce selectors and class attributes", "[style]") {
    SVG::SVG root;
    SVG::Classes<PlotClass> classes({
        { PlotClass::axis_line, ".axis-line" },
        { PlotClass::label, "axis-label" },
        { PlotClass::muted, "muted" }
    });
    root.style(classes.selector(PlotClass::axis_line))
        .set_attr("stroke", "#374151");
    root.style(classes.selector(PlotClass::label, PlotClass::muted))
        .set_attr("opacity", "0.72");
    root.add_child<SVG::Line>(
        0, 10, 0, 10,
        SVG::Attrs{{ "class", classes.classes(PlotClass::axis_line, PlotClass::muted) }});

    const auto output = std::string(root);

    REQUIRE(classes.name(PlotClass::axis_line) == "axis-line");
    REQUIRE(classes.selector(PlotClass::axis_line) == ".axis-line");
    REQUIRE(classes.selector(PlotClass::label, PlotClass::muted) == ".axis-label.muted");
    REQUIRE(classes.classes(PlotClass::axis_line, PlotClass::muted) == "axis-line muted");
    REQUIRE(output.find(".axis-line {\n\t\t\t\tstroke: #374151;\n\t\t\t}") != std::string::npos);
    REQUIRE(output.find(".axis-label.muted {\n\t\t\t\topacity: 0.72;\n\t\t\t}") != std::string::npos);
    REQUIRE(output.find("class=\"axis-line muted\"") != std::string::npos);
}

TEST_CASE("Typed CSS classes reject ambiguous or invalid names", "[style]") {
    SVG::Classes<PlotClass> classes({
        { PlotClass::axis_line, "axis-line" },
        { PlotClass::label, "axis-label" }
    });

    REQUIRE_THROWS_AS(classes.name(PlotClass::muted), std::invalid_argument);
    REQUIRE_THROWS_AS(SVG::Classes<PlotClass>({
        { PlotClass::axis_line, "axis-line" },
        { PlotClass::axis_line, "axis-line-copy" }
    }), std::invalid_argument);
    REQUIRE_THROWS_AS(SVG::Classes<PlotClass>({
        { PlotClass::axis_line, ".axis-line" },
        { PlotClass::label, "axis-line" }
    }), std::invalid_argument);
    REQUIRE_THROWS_AS(SVG::Classes<PlotClass>({
        { PlotClass::axis_line, "axis line" }
    }), std::invalid_argument);
}
