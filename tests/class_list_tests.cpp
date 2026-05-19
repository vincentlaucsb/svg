#include <catch2/catch.hpp>
#include "svg.hpp"

TEST_CASE("ClassList manages class tokens", "[class-list]") {
    SVG::SVG root;
    auto circ = root.add_child<SVG::Circle>();

    circ->class_list().add("chart").add("selected").add("chart");
    REQUIRE(circ->attr["class"] == "chart selected");
    REQUIRE(circ->class_list().contains("chart"));
    REQUIRE(circ->class_list().contains("selected"));

    circ->class_list().remove("chart");
    REQUIRE(circ->attr["class"] == "selected");
    REQUIRE_FALSE(circ->class_list().contains("chart"));

    REQUIRE(circ->class_list().toggle("hidden"));
    REQUIRE(circ->attr["class"] == "selected hidden");
    REQUIRE_FALSE(circ->class_list().toggle("hidden"));
    REQUIRE(circ->attr["class"] == "selected");
}

TEST_CASE("ClassList normalizes class text", "[class-list]") {
    SVG::SVG root;
    auto group = root.add_child<SVG::Group>();

    group->class_list().set(" chart   primary chart ");

    REQUIRE(group->attr["class"] == "chart primary");
    REQUIRE(group->class_list().str() == "chart primary");
    REQUIRE(group->class_list().tokens() == std::vector<std::string>{ "chart", "primary" });
}

TEST_CASE("Const ClassList can inspect but not mutate class tokens", "[class-list]") {
    SVG::Circle circle;
    circle.set_attr("class", "chart selected");
    const SVG::Circle& const_circle = circle;

    REQUIRE(const_circle.class_list().contains("chart"));
    REQUIRE(const_circle.class_list().str() == "chart selected");
    REQUIRE_THROWS_AS(const_circle.class_list().add("hidden"), std::logic_error);
}

TEST_CASE("Class attributes normalize through set_attr APIs", "[class-list]") {
    SVG::SVG root;
    auto group = root.add_child<SVG::Group>();
    auto first = group->add_child<SVG::Circle>();
    auto second = group->add_child<SVG::Circle>();

    first->set_attr("class", " chart   primary chart ");
    second->set_attr("class") << "chart" << " muted";

    REQUIRE(first->attr["class"] == "chart primary");
    REQUIRE(second->attr["class"] == "chart muted");

    const auto matches = root.get_elements_by_class("chart");
    REQUIRE(matches.size() == 2);

    REQUIRE_THROWS_AS(first->class_list().add("bad token"), std::invalid_argument);
}

TEST_CASE("set_attrs applies multiple attributes and normalizes class values", "[attributes]") {
    SVG::Rect rect;

    rect.set_attrs({
        { "class", " workout   completed workout " },
        { "fill", "var(--bar-fill)" },
        { "data-date", "2026-05-18" }
    });

    REQUIRE(rect.attr["class"] == "workout completed");
    REQUIRE(rect.attr["fill"] == "var(--bar-fill)");
    REQUIRE(rect.attr["data-date"] == "2026-05-18");
}
