#include <catch2/catch.hpp>
#include "svg.hpp"
#include "test_helpers.hpp"

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
