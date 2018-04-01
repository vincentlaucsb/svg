#define CATCH_CONFIG_MAIN
#include "catch.hpp"
#include "svg.hpp"

SVG::SVG two_circles(int x = 0, int y = 0, int r = 0);

SVG::SVG two_circles(int x, int y, int r) {
    // Return an SVG with two circles in a <g>
    SVG::SVG root;
    auto circ_container = root.add_child<SVG::Group>();
    (*circ_container) << SVG::Circle(x, y, r) << SVG::Circle(x, y, r);
    return root;
}

TEST_CASE("Proper Indentation", "[indent_test]") {
    SVG::SVG root;
    auto circ = root.add_child<SVG::Circle>();
    std::string correct = "<svg xmlns=\"http://www.w3.org/2000/svg\">\n"
        "\t<circle />\n"
        "</svg>";

    REQUIRE((std::string)root == correct);
}

TEST_CASE("Proper Indentation - Nested", "[indent_nest_test]") {
    SVG::SVG root = two_circles();
    std::string correct = "<svg xmlns=\"http://www.w3.org/2000/svg\">\n"
        "\t<g>\n"
        "\t\t<circle cx=\"0.0\" cy=\"0.0\" r=\"0.0\" />\n"
        "\t\t<circle cx=\"0.0\" cy=\"0.0\" r=\"0.0\" />\n"
        "\t</g>\n"
        "</svg>";

    REQUIRE((std::string)root == correct);
}

TEST_CASE("CSS Styling", "[test_css]") {
    SVG::SVG root = two_circles();
    root.style("circle").set_attr("fill", "#000000").set_attr("stroke", "#000000");
    std::string correct = "<svg xmlns=\"http://www.w3.org/2000/svg\">\n"
        "\t<g>\n"
        "\t\t<circle cx=\"0.0\" cy=\"0.0\" r=\"0.0\" />\n"
        "\t\t<circle cx=\"0.0\" cy=\"0.0\" r=\"0.0\" />\n"
        "\t</g>\n"
        "\t<style type=\"text/css\">\n"
        "\t\t<![CDATA[\n"
        "\t\t\tcircle {\n"
        "\t\t\t\tfill: #000000;\n"
        "\t\t\t\tstroke: #000000;\n"
        "\t\t\t}\n"
        "\t\t]]>\n"
        "\t</style>\n"
        "</svg>";

    REQUIRE(std::string(root) == correct);
}

TEST_CASE("One Decimal Place", "[decimal_place_test]") {
    SVG::SVG root;
    root.add_child<SVG::Line>(0.0, 0.0, PI, PI);
    std::string correct = "<svg xmlns=\"http://www.w3.org/2000/svg\">\n"
        "\t<line x1=\"0.0\" x2=\"0.0\" y1=\"3.1\" y2=\"3.1\" />\n"
        "</svg>";

    REQUIRE(std::string(root) == correct);
}

TEST_CASE("get_children() Test - Basic", "[test_get_children]") {
    SVG::SVG root;
    auto circ_ptr = root.add_child<SVG::Circle>();
    SVG::Element::ChildMap correct = {
        { "circle", std::vector<SVG::Element*>{circ_ptr} }
    };

    REQUIRE(root.get_children() == correct);
}

TEST_CASE("get_children() Test - Nested", "[test_get_children_nested]") {
    SVG::SVG root = two_circles();
    auto child_map = root.get_children();
    REQUIRE(child_map["g"].size() == 1);
    REQUIRE(child_map["circle"].size() == 2);
}

TEST_CASE("get_children() Test - Template", "[test_get_children_template]") {
    SVG::SVG root = two_circles();
    std::vector<SVG::SVG*> containers = root.get_children<SVG::SVG>();
    std::vector<SVG::Group*> groups = root.get_children<SVG::Group>();
    std::vector<SVG::Circle*> circles = root.get_children<SVG::Circle>();

    REQUIRE(containers.size() == 0);
    REQUIRE(groups.size() == 1);
    REQUIRE(circles.size() == 2);
}

TEST_CASE("autoscale() Test - Nested", "[test_autoscale_nested]") {
    SVG::SVG root;
    auto line_container = root.add_child<SVG::Group>(),
        circ_container = root.add_child<SVG::Group>();
    auto c1_ptr = circ_container->add_child<SVG::Circle>(-100, -100, 100),
        c2_ptr = circ_container->add_child<SVG::Circle>(100, 100, 100);

    // Lines shouldn't afect bounding box calculations because they're in between circles
    auto l1_ptr = line_container->add_child<SVG::Line>(0, 10, 0, 10),
      l2_ptr = line_container->add_child<SVG::Line>(0, 0, 0, 10);
    root.autoscale(SVG::NO_MARGINS);
    
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

TEST_CASE("merge() Test", "[merge_test]") {
    auto s1 = two_circles(200, 200, 200), s2 = two_circles(200, 200, 200);
    auto merged = SVG::merge(s1, s2);

    // Make sure there's an appropriate number of child elements
    auto child_map = merged.get_children();
    REQUIRE(child_map["svg"].size() == 2);
    REQUIRE(child_map["g"].size() == 2);
    REQUIRE(child_map["circle"].size() == 4);

    // Make sure this SVG has correct width/height
    REQUIRE(merged.width() == 840.0); // 800 + 40 for margins
    REQUIRE(merged.height() == 420.0); // 400 + 20 for margins
}

TEST_CASE("Implicit Point() Conversion", "point_conversion") {
    // Test that implicit conversions work correctly
    SVG::SVG root = two_circles();
    auto circ = root.get_children<SVG::Circle>();
    
    auto line = root.add_child<SVG::Line>(*circ[0], *circ[0]);
    REQUIRE(line->x1() == circ[0]->x());
}

TEST_CASE("Points Along a Circle Test", "[polar_points]") {
    auto points = SVG::util::polar_points(4, 0, 0, 100);
    REQUIRE(points.size() == 4);

    REQUIRE(APPROX_EQUALS(points[0].first, 100, 1));
    REQUIRE(APPROX_EQUALS(points[0].second, 0, 1));

    REQUIRE(APPROX_EQUALS(points[1].first, 0, 1));
    REQUIRE(APPROX_EQUALS(points[1].second, 100, 1));

    REQUIRE(APPROX_EQUALS(points[2].first, -100, 1));
    REQUIRE(APPROX_EQUALS(points[2].second, 0, 1));

    REQUIRE(APPROX_EQUALS(points[3].first, 0, 1));
    REQUIRE(APPROX_EQUALS(points[3].second, -100, 1));
}