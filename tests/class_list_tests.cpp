#include <catch2/catch.hpp>
#include "svg.hpp"

TEST_CASE("ClassList manages class tokens", "[class-list]") {
    SVG::SVG root;
    auto circ = root.add_child<SVG::Circle>();

    circ->class_list().add("chart").add("selected").add("chart");
    REQUIRE(circ->get_attr("class") == "chart selected");
    REQUIRE(circ->class_list().contains("chart"));
    REQUIRE(circ->class_list().contains("selected"));

    circ->class_list().remove("chart");
    REQUIRE(circ->get_attr("class") == "selected");
    REQUIRE_FALSE(circ->class_list().contains("chart"));

    REQUIRE(circ->class_list().toggle("hidden"));
    REQUIRE(circ->get_attr("class") == "selected hidden");
    REQUIRE_FALSE(circ->class_list().toggle("hidden"));
    REQUIRE(circ->get_attr("class") == "selected");
}

TEST_CASE("ClassList normalizes class text", "[class-list]") {
    SVG::SVG root;
    auto group = root.add_child<SVG::Group>();

    group->class_list().set(" chart   primary chart ");

    REQUIRE(group->get_attr("class") == "chart primary");
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

    REQUIRE(first->get_attr("class") == "chart primary");
    REQUIRE(second->get_attr("class") == "chart muted");

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

    REQUIRE(rect.get_attr("class") == "workout completed");
    REQUIRE(rect.get_attr("fill") == "var(--bar-fill)");
    REQUIRE(rect.get_attr("data-date") == "2026-05-18");
    REQUIRE(rect.attrs().size() == 3);
}

TEST_CASE("get_attr supports string and numeric fallbacks", "[attributes]") {
    SVG::Rect rect;
    rect.set_attr("x", "12.5px")
        .set_attr("data-count", "7")
        .set_attr("data-bad-number", "calc(100% - 2px)");

    REQUIRE(rect.get_attr("x", "0") == "12.5px");
    REQUIRE(rect.get_attr("missing", "fallback") == "fallback");
    REQUIRE(rect.get_attr<double>("x", 0) == Approx(12.5));
    REQUIRE(rect.get_attr<int>("data-count", 0) == 7);
    REQUIRE(rect.get_attr<double>("missing-number", 4.5) == Approx(4.5));
    REQUIRE(rect.get_attr<double>("data-bad-number", 2.5) == Approx(2.5));
}

TEST_CASE("TransformList appends transform functions in order", "[transform-list]") {
    SVG::Text label(10, 20, "Workout");

    label.transform()
        .translate(5, 6)
        .rotate(-45, 10, 20)
        .scale(1.5)
        .skew_x(12);

    REQUIRE(label.get_attr("transform") ==
            "translate(5.0 6.0) rotate(-45.0 10.0 20.0) scale(1.5) skewX(12.0)");
}

TEST_CASE("TransformList can replace clear and inspect transform values", "[transform-list]") {
    SVG::Group group;

    group.transform_list().set("inherit");
    REQUIRE(group.transform_list().str() == "inherit");
    REQUIRE_THROWS_AS(group.transform_list().rotate(15), std::logic_error);

    group.transform_list().set("none").translate(10);
    REQUIRE(group.get_attr("transform") == "translate(10.0)");

    group.transform_list().clear();
    REQUIRE(group.get_attr("transform").empty());

    const SVG::Group& const_group = group;
    REQUIRE(const_group.transform_list().str().empty());
    REQUIRE_THROWS_AS(const_group.transform_list().scale(2), std::logic_error);
}
