#include <catch2/catch.hpp>
#include "svg.hpp"
#include "test_helpers.hpp"

TEST_CASE("autoscale computes nested child bounds", "[layout]") {
    SVG::SVG root;
    auto line_container = root.add_child<SVG::Group>();
    auto circ_container = root.add_child<SVG::Group>();
    auto c1_ptr = circ_container->add_child<SVG::Circle>(-100, -100, 100);
    auto c2_ptr = circ_container->add_child<SVG::Circle>(100, 100, 100);

    line_container->add_child<SVG::Line>(0, 10, 0, 10);
    line_container->add_child<SVG::Line>(0, 0, 0, 10);
    root.autoscale(SVG::NO_MARGINS);

    REQUIRE(c1_ptr->get_bbox().x1 == -200);
    REQUIRE(c1_ptr->get_bbox().x2 == 0);
    REQUIRE(c1_ptr->get_bbox().y1 == -200);
    REQUIRE(c1_ptr->get_bbox().y2 == 0);

    REQUIRE(c2_ptr->get_bbox().x1 == 0);
    REQUIRE(c2_ptr->get_bbox().x2 == 200);
    REQUIRE(c2_ptr->get_bbox().y1 == 0);
    REQUIRE(c2_ptr->get_bbox().y2 == 200);

    REQUIRE(root.get_attr("width") == "400.0");
    REQUIRE(root.get_attr("height") == "400.0");
    REQUIRE(root.get_attr("viewBox") == "-200.0 -200.0 400.0 400.0");
}

TEST_CASE("autoscale includes page margins around positive coordinates", "[layout]") {
    SVG::SVG root;
    root.add_child<SVG::Rect>(100, 50, 20, 10);

    root.autoscale({ 5, 15, 10, 20 });

    REQUIRE(root.get_attr("width") == "40.0");
    REQUIRE(root.get_attr("height") == "40.0");
    REQUIRE(root.get_attr("viewBox") == "95.0 40.0 40.0 40.0");
}

TEST_CASE("autoscale includes simple rotate transforms", "[layout]") {
    SVG::SVG root;
    auto* rect = root.add_child<SVG::Rect>(0, 0, 10, 20);
    rect->transform().rotate(90);

    root.autoscale(SVG::NO_MARGINS);

    REQUIRE(root.get_attr("width") == "20.0");
    REQUIRE(root.get_attr("height") == "10.0");
    REQUIRE(root.get_attr("viewBox") == "-20.0 0.0 20.0 10.0");
}

TEST_CASE("responsive_autoscale sets viewBox without width or height", "[layout]") {
    SVG::SVG root;
    root.add_child<SVG::Rect>(100, 50, 20, 10);

    root.responsive_autoscale({ 5, 15, 10, 20 });

    REQUIRE(root.get_attr("width").empty());
    REQUIRE(root.get_attr("height").empty());
    REQUIRE(root.get_attr("viewBox") == "95.0 40.0 40.0 40.0");
}

TEST_CASE("responsive_autoscale preserves explicit display size", "[layout]") {
    SVG::SVG root;
    root.set_attr("width", "100%");
    root.set_attr("height", "auto");
    root.add_child<SVG::Rect>(0, 0, 100, 50);

    root.responsive_autoscale(SVG::NO_MARGINS);

    REQUIRE(root.get_attr("width") == "100%");
    REQUIRE(root.get_attr("height") == "auto");
    REQUIRE(root.get_attr("viewBox") == "0.0 0.0 100.0 50.0");
}

TEST_CASE("merge combines SVG documents horizontally", "[layout]") {
    auto s1 = two_circles(200, 200, 200);
    auto s2 = two_circles(200, 200, 200);
    auto merged = SVG::merge(s1, s2);

    auto child_map = merged.get_children();
    REQUIRE(child_map["svg"].size() == 2);
    REQUIRE(child_map["g"].size() == 2);
    REQUIRE(child_map["circle"].size() == 4);

    REQUIRE(merged.width() == 840.0);
    REQUIRE(merged.height() == 420.0);
}
