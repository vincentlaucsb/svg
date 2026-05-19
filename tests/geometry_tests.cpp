#include <catch2/catch.hpp>
#include "svg.hpp"
#include "test_helpers.hpp"

TEST_CASE("Shapes convert implicitly to points", "[geometry]") {
    SVG::SVG root = two_circles();
    auto circ = root.get_children<SVG::Circle>();

    auto line = root.add_child<SVG::Line>(*circ[0], *circ[0]);
    REQUIRE(line->x1() == circ[0]->x());
}

TEST_CASE("polar_points returns points along a circle", "[geometry]") {
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
