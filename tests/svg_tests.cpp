#define CATCH_CONFIG_MAIN
#include "catch.hpp"
#include "svg.hpp"

TEST_CASE("Proper Indentation", "[indent_test]") {
    SVG::SVG root;
    SVG::Circle circ;
    root.add_child(circ);
    std::string correct = "<svg xmlns=\"http://www.w3.org/2000/svg\">\n"
        "\t<circle />\n"
        "</svg>";

    REQUIRE(root.to_string() == correct);
}

TEST_CASE("Proper Indentation - Nested", "[indent_nest_test]") {
    SVG::SVG root;
    SVG::Group circ_container;
    circ_container.add_child(SVG::Circle(), SVG::Circle());
    root.add_child(circ_container);

    std::string correct = "<svg xmlns=\"http://www.w3.org/2000/svg\">\n"
        "\t<g>\n"
        "\t\t<circle />\n"
        "\t\t<circle />\n"
        "\t</g>\n"
        "</svg>";

    REQUIRE(root.to_string() == correct);
}

TEST_CASE("get_children() Test - Basic", "[test_get_children]") {
    SVG::SVG root;
    SVG::Circle circ;
    auto circ_ptr = root.add_child(circ);
    SVG::Element::ChildMap correct = {
        { "circle", std::vector<SVG::Element*>{circ_ptr} }
    };

    REQUIRE(root.get_children() == correct);
}

TEST_CASE("get_children() Test - Nested", "[test_get_children_nested]") {
    SVG::SVG root;
    SVG::Group circ_container;
    circ_container.add_child(SVG::Circle(), SVG::Circle());
    root.add_child(circ_container);

    auto child_map = root.get_children();
    REQUIRE(child_map["g"].size() == 1);
    REQUIRE(child_map["circle"].size() == 2);
}