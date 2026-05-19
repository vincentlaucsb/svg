#include <catch2/catch.hpp>
#include "svg.hpp"
#include "test_helpers.hpp"

TEST_CASE("Elements serialize with proper indentation", "[render]") {
    SVG::SVG root;
    root.add_child<SVG::Circle>();
    std::string correct = "<svg xmlns=\"http://www.w3.org/2000/svg\">\n"
        "\t<circle />\n"
        "</svg>";

    REQUIRE((std::string)root == correct);
}

TEST_CASE("Nested elements serialize with proper indentation", "[render]") {
    SVG::SVG root = two_circles();
    std::string correct = "<svg xmlns=\"http://www.w3.org/2000/svg\">\n"
        "\t<g>\n"
        "\t\t<circle cx=\"0.0\" cy=\"0.0\" r=\"0.0\" />\n"
        "\t\t<circle cx=\"0.0\" cy=\"0.0\" r=\"0.0\" />\n"
        "\t</g>\n"
        "</svg>";

    REQUIRE(std::string(root) == correct);
}

TEST_CASE("Doubles serialize to one decimal place", "[render]") {
    SVG::SVG root;
    root.add_child<SVG::Line>(0.0, 0.0, PI, PI);
    std::string correct = "<svg xmlns=\"http://www.w3.org/2000/svg\">\n"
        "\t<line x1=\"0.0\" x2=\"0.0\" y1=\"3.1\" y2=\"3.1\" />\n"
        "</svg>";

    REQUIRE(std::string(root) == correct);
}

TEST_CASE("Text serializes content between tags", "[render]") {
    SVG::SVG root;
    root.add_child<SVG::Text>(10, 20, "Workout");

    REQUIRE(std::string(root).find("<text x=\"10.0\" y=\"20.0\">Workout</text>") != std::string::npos);
}
