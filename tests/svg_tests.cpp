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

TEST_CASE("set_bbox() Test - Nested", "[test_set_bbox_nested]") {
    SVG::SVG root;
    SVG::Group line_container, circ_container;
    auto c1_ptr = circ_container.add_child(SVG::Circle(-100, -100, 100)),
        c2_ptr = circ_container.add_child(SVG::Circle(100, 100, 100));

    // Lines shouldn't affect bounding box calculations because they're in between circles
    auto l1_ptr = line_container.add_child(SVG::Line(0, 10, 0, 10)),
      l2_ptr = line_container.add_child(SVG::Line(0, 0, 0, 10));

    root.add_child(circ_container);
    root.add_child(line_container);
    root.set_bbox();
    
    // Make sure intermediate get_bbox() calls are correct
    REQUIRE(c1_ptr->get_bbox().x1 == -200);
    REQUIRE(c1_ptr->get_bbox().x2 == 0);
    REQUIRE(c1_ptr->get_bbox().y1 == -200);
    REQUIRE(c1_ptr->get_bbox().y2 == 0);
    
    REQUIRE(c2_ptr->get_bbox().x1 == 0);
    REQUIRE(c2_ptr->get_bbox().x2 == 200);
    REQUIRE(c2_ptr->get_bbox().y1 == 0);
    REQUIRE(c2_ptr->get_bbox().y2 == 200);

    // Make sure final results are correct
    REQUIRE(root.attr["width"] == "400.0");
    REQUIRE(root.attr["height"] == "400.0");
    REQUIRE(root.attr["viewBox"] == "-200.0 -200.0 400.0 400.0");
}