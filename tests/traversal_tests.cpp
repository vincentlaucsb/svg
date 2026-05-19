#include <catch2/catch.hpp>
#include "svg.hpp"
#include "test_helpers.hpp"

TEST_CASE("get_children returns descendants by tag", "[traversal]") {
    SVG::SVG root;
    auto circ_ptr = root.add_child<SVG::Circle>();
    SVG::Element::ChildMap correct = {
        { "style", std::vector<SVG::Element*>{root.css} },
        { "circle", std::vector<SVG::Element*>{circ_ptr} }
    };

    REQUIRE(root.get_children() == correct);
}

TEST_CASE("get_children includes nested descendants", "[traversal]") {
    SVG::SVG root = two_circles();
    auto child_map = root.get_children();

    REQUIRE(child_map["g"].size() == 1);
    REQUIRE(child_map["circle"].size() == 2);
}

TEST_CASE("Templated get_children filters descendants by exact type", "[traversal]") {
    SVG::SVG root = two_circles();
    std::vector<SVG::SVG*> containers = root.get_children<SVG::SVG>();
    std::vector<SVG::Group*> groups = root.get_children<SVG::Group>();
    std::vector<SVG::Circle*> circles = root.get_children<SVG::Circle>();

    REQUIRE(containers.size() == 0);
    REQUIRE(groups.size() == 1);
    REQUIRE(circles.size() == 2);
}

TEST_CASE("get_element_by_id finds nested elements", "[traversal]") {
    SVG::SVG root;
    auto group = root.add_child<SVG::Group>();
    auto rect = group->add_child<SVG::Rect>("workout");

    REQUIRE(root.get_element_by_id("workout") == rect);
    REQUIRE(root.get_element_by_id("missing") == nullptr);
}

TEST_CASE("get_elements_by_class matches tokenized classes", "[traversal]") {
    SVG::SVG root;
    auto group = root.add_child<SVG::Group>();
    auto first = group->add_child<SVG::Circle>();
    auto second = group->add_child<SVG::Rect>();
    auto third = group->add_child<SVG::Circle>();

    first->set_attr("class", "chart completed");
    second->set_attr("class", "chart-muted");
    third->set_attr("class", "completed chart");

    auto matches = root.get_elements_by_class("chart");
    REQUIRE(matches.size() == 2);
    REQUIRE(matches[0] == first);
    REQUIRE(matches[1] == third);
}
