#include <catch2/catch.hpp>
#include "svg.hpp"
#include "test_helpers.hpp"

namespace {
    class CustomFilter : public SVG::Element {
    public:
        using SVG::Element::Element;

    protected:
        std::string tag() override { return "feGaussianBlur"; }
    };

    class CloneableCustomFilter : public CustomFilter {
    public:
        using CustomFilter::CustomFilter;

    protected:
        std::unique_ptr<SVG::Element> clone_element_impl() const override {
            return clone_as<CloneableCustomFilter>();
        }
    };
}

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

TEST_CASE("add_child applies trailing Attrs after construction", "[render]") {
    SVG::SVG root;
    auto* group = root.add_child<SVG::Group>(SVG::Attrs{{ "class", "plot marks" }});
    auto* label = group->add_child<SVG::Text>(
        12, 24, "Volume",
        SVG::Attrs{{ "class", "axis-label" }, { "data-side", "left" }});

    const std::string svg = root;

    REQUIRE(group->get_attr("class") == "plot marks");
    REQUIRE(label->get_attr("data-side") == "left");
    REQUIRE(svg.find("<g class=\"plot marks\">") != std::string::npos);
    REQUIRE(svg.find("<text class=\"axis-label\" data-side=\"left\" x=\"12.0\" y=\"24.0\">Volume</text>") !=
            std::string::npos);
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

TEST_CASE("Title serializes escaped content between tags", "[render]") {
    SVG::SVG root;
    root.add_child<SVG::Title>("A & B < C \"D\" 'E'");

    REQUIRE(std::string(root).find("<title>A &amp; B &lt; C &quot;D&quot; &apos;E&apos;</title>") !=
            std::string::npos);
}

TEST_CASE("XML-sensitive values are escaped when rendering", "[render]") {
    SVG::SVG root({{"xmlns", "http://www.w3.org/2000/svg"},
                   {"aria-label", "A & \"B\" <C> 'D'"}});
    auto* group = root.add_child<SVG::Group>();
    group->set_attr("data-label", "nested & \"quoted\"");
    group->add_child<SVG::Text>(10, 20, "A & B < C");

    const std::string svg = root;

    REQUIRE(svg.find("aria-label=\"A &amp; &quot;B&quot; &lt;C&gt; &apos;D&apos;\"") !=
            std::string::npos);
    REQUIRE(svg.find("data-label=\"nested &amp; &quot;quoted&quot;\"") != std::string::npos);
    REQUIRE(svg.find(">A &amp; B &lt; C</text>") != std::string::npos);
}

TEST_CASE("Symbols and uses serialize reusable marks", "[render]") {
    SVG::SVG root;
    auto* symbol = root.defs()->symbol("dot");
    REQUIRE(root.defs()->symbol("dot") == symbol);
    REQUIRE(root.get_element_by_id<SVG::Symbol>("dot") == symbol);

    symbol->set_attr("viewBox", "0 0 6.4 6.4");
    symbol->add_child<SVG::Circle>(3.2, 3.2, 3.2);

    auto* use = root.add_child<SVG::Use>(symbol->use(10, 20, 6.4, 6.4));
    use->class_list().add("marker");

    const std::string svg = root;

    REQUIRE(svg.find("<defs>") != std::string::npos);
    REQUIRE(svg.find("<defs>") < svg.find("<use"));
    REQUIRE(svg.find("<symbol id=\"dot\" viewBox=\"0 0 6.4 6.4\">") != std::string::npos);
    REQUIRE(svg.find("<circle cx=\"3.2\" cy=\"3.2\" r=\"3.2\" />") != std::string::npos);
    REQUIRE(svg.find("<use class=\"marker\" height=\"6.4\" href=\"#dot\" width=\"6.4\" x=\"10.0\" y=\"20.0\" />") !=
            std::string::npos);
}

TEST_CASE("Defs serialize after root styles", "[render]") {
    SVG::SVG root;
    auto* line = root.add_child<SVG::Line>(0, 0, 10, 10);
    root.style(".line").set_attr("stroke", "black");

    auto* symbol = root.defs()->symbol("dot");
    symbol->add_child<SVG::Circle>(3.2, 3.2, 3.2);
    line->class_list().add("line");

    const std::string svg = root;
    const auto style_pos = svg.find("<style");
    const auto defs_pos = svg.find("<defs>");
    const auto line_pos = svg.find("<line");

    REQUIRE(style_pos != std::string::npos);
    REQUIRE(defs_pos != std::string::npos);
    REQUIRE(line_pos != std::string::npos);
    REQUIRE(style_pos < defs_pos);
    REQUIRE(defs_pos < line_pos);
}

TEST_CASE("Custom elements serialize custom tags and support untyped lookup", "[render]") {
    SVG::SVG root;
    auto* blur = root.add_child<CustomFilter>("blur");
    blur->set_attr("stdDeviation", "2");

    const std::string svg = root;

    REQUIRE(root.get_element_by_id("blur") == blur);
    REQUIRE(blur->kind() == SVG::ElementKind::Custom);
    REQUIRE(svg.find("<feGaussianBlur id=\"blur\" stdDeviation=\"2\" />") != std::string::npos);
    REQUIRE_THROWS_AS(root.clone(), std::logic_error);
}

TEST_CASE("Custom elements opt into clone with copy construction", "[render]") {
    SVG::SVG root;
    auto* blur = root.add_child<CloneableCustomFilter>("blur");
    blur->set_attr("stdDeviation", "2");

    auto copy = root.clone();
    const std::string copied_svg = copy;

    REQUIRE(copy.get_element_by_id("blur") != blur);
    REQUIRE(copied_svg.find("<feGaussianBlur id=\"blur\" stdDeviation=\"2\" />") != std::string::npos);
}
