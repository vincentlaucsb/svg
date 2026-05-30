#include <catch2/catch.hpp>
#include "svg.hpp"
#include "test_helpers.hpp"

namespace {
    class BadBBoxElement : public SVG::Element {
    public:
        using SVG::Element::Element;

    protected:
        SVG::Element::BoundingBox get_bbox() const override {
            return { NAN, NAN, NAN, NAN };
        }

        std::string tag() override { return "bad-bbox"; }
    };
}

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

TEST_CASE("shape bbox includes visible stroke width", "[layout]") {
    SVG::Rect rect(0, 0, 10, 20);
    rect.set_attr("stroke", "#111")
        .set_attr("stroke-width", 4);

    const auto bbox = rect.get_bbox();

    REQUIRE(bbox.x1 == Approx(-2));
    REQUIRE(bbox.x2 == Approx(12));
    REQUIRE(bbox.y1 == Approx(-2));
    REQUIRE(bbox.y2 == Approx(22));
}

TEST_CASE("stroke width without a visible stroke does not affect bbox", "[layout]") {
    SVG::Circle circle(10, 20, 5);
    circle.set_attr("stroke-width", 8);

    const auto bbox = circle.get_bbox();

    REQUIRE(bbox.x1 == Approx(5));
    REQUIRE(bbox.x2 == Approx(15));
    REQUIRE(bbox.y1 == Approx(15));
    REQUIRE(bbox.y2 == Approx(25));
}

TEST_CASE("autoscale includes visible stroke width", "[layout]") {
    SVG::SVG root;
    auto* rect = root.add_child<SVG::Rect>(0, 0, 10, 10);
    rect->set_attr("stroke", "#111")
        .set_attr("stroke-width", 4);

    root.autoscale(SVG::NO_MARGINS);

    REQUIRE(root.get_attr("viewBox") == "-2.0 -2.0 14.0 14.0");
}

TEST_CASE("use bbox includes referenced stroked symbol bounds", "[layout]") {
    SVG::SVG root;
    auto* symbol = root.defs()->symbol("marker");
    auto* rect = symbol->add_child<SVG::Rect>(0, 0, 10, 10);
    rect->set_attr("stroke", "#111")
        .set_attr("stroke-width", 4);
    auto* use = root.add_child<SVG::Use>(symbol->href(), 20, 30);

    const auto bbox = use->get_bbox();
    root.autoscale(SVG::NO_MARGINS);

    REQUIRE(bbox.x1 == Approx(18));
    REQUIRE(bbox.x2 == Approx(32));
    REQUIRE(bbox.y1 == Approx(28));
    REQUIRE(bbox.y2 == Approx(42));
    REQUIRE(root.get_attr("viewBox") == "18.0 28.0 14.0 14.0");
}

TEST_CASE("use bbox scales referenced symbol viewBox into its viewport", "[layout]") {
    SVG::SVG root;
    auto* symbol = root.defs()->symbol("marker");
    symbol->set_attr("viewBox", "0 0 10 10");
    symbol->add_child<SVG::Rect>(0, 0, 10, 10);
    auto* use = root.add_child<SVG::Use>(symbol->href(), 20, 30, 40, 80);

    const auto bbox = use->get_bbox();
    root.autoscale(SVG::NO_MARGINS);

    REQUIRE(bbox.x1 == Approx(20));
    REQUIRE(bbox.x2 == Approx(60));
    REQUIRE(bbox.y1 == Approx(30));
    REQUIRE(bbox.y2 == Approx(110));
    REQUIRE(root.get_attr("viewBox") == "20.0 30.0 40.0 80.0");
}

TEST_CASE("rotate_about_bbox rotates text around bottom-right bounds", "[layout]") {
    SVG::Text label(10, 20, "May");
    label.set_attr("font-size", 10);

    label.transform().rotate_about_bbox(-45, SVG::Anchor::End, SVG::Anchor::End);

    REQUIRE(label.get_attr("transform").find("rotate(-45.0 32.0 24.0)") != std::string::npos);
}

TEST_CASE("autoscale includes text rotated around its measured bbox", "[layout]") {
    SVG::SVG root;
    auto* label = root.add_child<SVG::Text>(10, 20, "May");
    label->set_attr("font-size", 10);
    label->transform().rotate_about_bbox(-90, SVG::Anchor::End, SVG::Anchor::End);

    root.autoscale(SVG::NO_MARGINS);

    REQUIRE(root.get_attr("viewBox") == "17.8 24.0 14.2 22.0");
}

TEST_CASE("rotate_about_bbox supports rect bounds", "[layout]") {
    SVG::Rect rect(10, 20, 30, 40);

    rect.transform().rotate_about_bbox(30, SVG::Anchor::Start, SVG::Anchor::Center);

    REQUIRE(rect.get_attr("transform") == "rotate(30.0 10.0 40.0)");
}

TEST_CASE("rotate_about_bbox rejects unmeasurable bounds", "[layout]") {
    SVG::Group group;

    REQUIRE_THROWS_AS(
        group.transform().rotate_about_bbox(15, SVG::Anchor::Center, SVG::Anchor::Center),
        std::logic_error);
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

TEST_CASE("nested SVG bbox uses viewport bounds", "[layout]") {
    SVG::SVG nested(SVG::Attrs{
        { "x", "100" },
        { "y", "50" },
        { "width", "5" },
        { "height", "7" }
    });
    nested.add_child<SVG::Rect>(0, 0, 100, 100);

    const auto bbox = nested.get_bbox();

    REQUIRE(bbox.x1 == Approx(100));
    REQUIRE(bbox.x2 == Approx(105));
    REQUIRE(bbox.y1 == Approx(50));
    REQUIRE(bbox.y2 == Approx(57));
}

TEST_CASE("autoscale automatically scales nested SVG children before measuring parent", "[layout]") {
    SVG::SVG root;
    auto* nested = root.add_child<SVG::SVG>(SVG::Attrs{
        { "x", "100" },
        { "y", "50" },
        { "width", "1" },
        { "height", "1" }
    });
    nested->add_child<SVG::Rect>(0, 0, 20, 10);

    root.autoscale(SVG::NO_MARGINS);

    REQUIRE(nested->get_attr("width") == "20.0");
    REQUIRE(nested->get_attr("height") == "10.0");
    REQUIRE(nested->get_attr("viewBox") == "0.0 0.0 20.0 10.0");
    REQUIRE(root.get_attr("viewBox") == "100.0 50.0 20.0 10.0");
}

TEST_CASE("AutoscaleOptions can preserve nested SVG viewport sizes", "[layout]") {
    SVG::SVG root;
    auto* nested = root.add_child<SVG::SVG>(SVG::Attrs{
        { "x", "100" },
        { "y", "50" },
        { "width", "1" },
        { "height", "1" }
    });
    nested->add_child<SVG::Rect>(0, 0, 20, 10);

    root.autoscale(SVG::AutoscaleOptions(SVG::NO_MARGINS, false));

    REQUIRE(nested->get_attr("width") == "1");
    REQUIRE(nested->get_attr("height") == "1");
    REQUIRE(nested->get_attr("viewBox").empty());
    REQUIRE(root.get_attr("viewBox") == "100.0 50.0 1.0 1.0");
}

TEST_CASE("text bbox approximates baseline text bounds", "[layout]") {
    SVG::Text label(10, 20, "May");
    label.set_attr("font-size", 10);

    const auto bbox = label.get_bbox();

    REQUIRE(bbox.x1 == Approx(10));
    REQUIRE(bbox.x2 == Approx(32));
    REQUIRE(bbox.y1 == Approx(9.8));
    REQUIRE(bbox.y2 == Approx(24));
}

TEST_CASE("text bbox respects middle anchor and baseline", "[layout]") {
    SVG::Text title(50, 25, "Header");
    title.set_attr("font-size", 20)
        .set_attr("text-anchor", "middle")
        .set_attr("dominant-baseline", "middle");

    const auto bbox = title.get_bbox();

    REQUIRE(bbox.x1 == Approx(11.4));
    REQUIRE(bbox.x2 == Approx(88.6));
    REQUIRE(bbox.y1 == Approx(10.8));
    REQUIRE(bbox.y2 == Approx(39.2));
}

TEST_CASE("text bbox falls back for non-numeric font sizes", "[layout]") {
    SVG::Text label(0, 0, "OK");
    label.set_attr("font-size", "var(--chart-label-size)");

    const auto bbox = label.get_bbox();

    REQUIRE(bbox.x2 - bbox.x1 == Approx(23.68));
    REQUIRE(bbox.y2 - bbox.y1 == Approx(22.72));
}

TEST_CASE("text bbox distinguishes narrow and wide ASCII advances", "[layout]") {
    SVG::Text narrow(0, 0, "ill...");
    SVG::Text wide(0, 0, "MWm");
    narrow.set_attr("font-size", 10);
    wide.set_attr("font-size", 10);

    const auto narrow_bbox = narrow.get_bbox();
    const auto wide_bbox = wide.get_bbox();

    REQUIRE(narrow_bbox.x2 - narrow_bbox.x1 == Approx(22.6));
    REQUIRE(wide_bbox.x2 - wide_bbox.x1 == Approx(28));
}

TEST_CASE("large bold text bbox is padded conservatively", "[layout]") {
    SVG::Text normal(0, 100, "MWM Title");
    SVG::Text bold(0, 100, "MWM Title");
    normal.set_attr("font-size", 100);
    bold.set_attr("font-size", 100)
        .set_attr("font-weight", "bold");

    const auto normal_bbox = normal.get_bbox();
    const auto bold_bbox = bold.get_bbox();

    REQUIRE(bold_bbox.x2 - bold_bbox.x1 > normal_bbox.x2 - normal_bbox.x1);
    REQUIRE(bold_bbox.y2 - bold_bbox.y1 > normal_bbox.y2 - normal_bbox.y1);
    REQUIRE(bold_bbox.y1 < 0);
    REQUIRE(bold_bbox.y2 > 140);
}

TEST_CASE("text bbox decodes UTF-8 accented text by code point", "[layout]") {
    SVG::Text label(0, 0, std::string("caf\xc3\xa9"));
    label.set_attr("font-size", 10);

    const auto bbox = label.get_bbox();

    REQUIRE(bbox.x2 - bbox.x1 == Approx(25.6));
}

TEST_CASE("text bbox gives combining marks zero advance", "[layout]") {
    SVG::Text plain(0, 0, "e");
    SVG::Text combining(0, 0, std::string("e\xcc\x81"));
    plain.set_attr("font-size", 10);
    combining.set_attr("font-size", 10);

    const auto plain_bbox = plain.get_bbox();
    const auto combining_bbox = combining.get_bbox();

    REQUIRE(combining_bbox.x2 - combining_bbox.x1 == Approx(plain_bbox.x2 - plain_bbox.x1));
}

TEST_CASE("text bbox estimates CJK characters as full-width", "[layout]") {
    SVG::Text label(0, 0, std::string("\xe6\x96\x87\xe6\x9c\xac"));
    label.set_attr("font-size", 10);

    const auto bbox = label.get_bbox();

    REQUIRE(bbox.x2 - bbox.x1 == Approx(21.4));
}

TEST_CASE("text bbox estimates emoji as wide symbols", "[layout]") {
    SVG::Text label(0, 0, std::string("\xf0\x9f\x98\x80"));
    label.set_attr("font-size", 10);

    const auto bbox = label.get_bbox();

    REQUIRE(bbox.x2 - bbox.x1 == Approx(11.4));
}

TEST_CASE("multiline text bbox uses longest line width", "[layout]") {
    SVG::Text label(0, 0, "Hi\nMMMM");
    label.set_attr("font-size", 10);

    const auto bbox = label.get_bbox();

    REQUIRE(bbox.x2 - bbox.x1 == Approx(37.2));
    REQUIRE(bbox.y1 == Approx(-10.2));
    REQUIRE(bbox.y2 == Approx(17.2));
    REQUIRE(bbox.y2 - bbox.y1 == Approx(27.4));
}

TEST_CASE("multiline text bbox handles CRLF and blank lines", "[layout]") {
    SVG::Text crlf(0, 0, "OK\r\nM");
    SVG::Text blank_line(0, 0, "W\n\nW");
    crlf.set_attr("font-size", 10);
    blank_line.set_attr("font-size", 10);

    const auto crlf_bbox = crlf.get_bbox();
    const auto blank_line_bbox = blank_line.get_bbox();

    REQUIRE(crlf_bbox.x2 - crlf_bbox.x1 == Approx(14.8));
    REQUIRE(crlf_bbox.y2 - crlf_bbox.y1 == Approx(27.4));
    REQUIRE(blank_line_bbox.x2 - blank_line_bbox.x1 == Approx(9.6));
    REQUIRE(blank_line_bbox.y2 - blank_line_bbox.y1 == Approx(40.6));
}

TEST_CASE("autoscale includes text bounds", "[layout]") {
    SVG::SVG root;
    root.add_child<SVG::Rect>(0, 0, 100, 50);
    auto* title = root.add_child<SVG::Text>(50, -6, "Attendance");
    title->set_attr("font-size", 20)
        .set_attr("text-anchor", "middle");

    root.autoscale(SVG::NO_MARGINS);

    REQUIRE(root.get_attr("viewBox") == "-13.4 -26.4 126.8 76.4");
}

TEST_CASE("explicit layout bbox lets autoscale use caller measurements", "[layout]") {
    SVG::SVG root;
    auto* measured = root.add_child<BadBBoxElement>();
    measured->layout_bbox({ 10, 40, 20, 60 });

    root.autoscale(SVG::NO_MARGINS);

    REQUIRE(measured->has_layout_bbox());
    const auto bad_bbox = static_cast<SVG::Element*>(measured)->get_bbox();
    REQUIRE(bad_bbox.x1 != bad_bbox.x1);
    REQUIRE(measured->layout_bbox().x1 == 10);
    REQUIRE(root.get_attr("viewBox") == "10.0 20.0 30.0 40.0");
}

TEST_CASE("explicit text layout bbox wins over measured padding", "[layout]") {
    SVG::SVG root;
    auto* label = root.add_child<SVG::Text>(0, 0, "Padded");
    label->bbox_padding(100)
        .layout_bbox({ 1, 2, 3, 4 });

    root.autoscale(SVG::NO_MARGINS);

    REQUIRE(label->layout_bbox().x1 == 1);
    REQUIRE(label->layout_bbox().x2 == 2);
    REQUIRE(root.get_attr("viewBox") == "1.0 3.0 1.0 1.0");
}

TEST_CASE("bbox padding inflates measured layout bounds", "[layout]") {
    SVG::Text label(10, 20, "OK");
    label.set_attr("font-size", 10);
    label.bbox_padding({ 1, 2, 3, 4 });

    const auto geometry = label.get_bbox();
    const auto layout = label.layout_bbox();

    REQUIRE(layout.x1 == Approx(geometry.x1 - 1));
    REQUIRE(layout.x2 == Approx(geometry.x2 + 2));
    REQUIRE(layout.y1 == Approx(geometry.y1 - 3));
    REQUIRE(layout.y2 == Approx(geometry.y2 + 4));
}

TEST_CASE("clearing layout bbox restores built-in autoscale measurement", "[layout]") {
    SVG::SVG root;
    auto* rect = root.add_child<SVG::Rect>(0, 0, 10, 20);
    rect->layout_bbox({ -100, 100, -50, 50 });

    root.autoscale(SVG::NO_MARGINS);
    REQUIRE(root.get_attr("viewBox") == "-100.0 -50.0 200.0 100.0");

    rect->clear_layout_bbox();
    root.autoscale(SVG::NO_MARGINS);

    REQUIRE_FALSE(rect->has_layout_bbox());
    REQUIRE(root.get_attr("viewBox") == "0.0 0.0 10.0 20.0");
}

TEST_CASE("layout bbox does not replace element geometry", "[layout]") {
    SVG::Rect rect(0, 0, 10, 20);
    rect.layout_bbox({ -100, 100, -50, 50 });

    const auto geometry = rect.get_bbox();
    const auto layout = rect.layout_bbox();

    REQUIRE(geometry.x1 == 0);
    REQUIRE(geometry.x2 == 10);
    REQUIRE(geometry.y1 == 0);
    REQUIRE(geometry.y2 == 20);
    REQUIRE(layout.x1 == -100);
    REQUIRE(layout.x2 == 100);
    REQUIRE(layout.y1 == -50);
    REQUIRE(layout.y2 == 50);
}

TEST_CASE("snap_to positions element to target side with centered edge alignment", "[layout]") {
    SVG::SVG root;
    auto* plot = root.add_child<SVG::Rect>(10, 20, 100, 50);
    auto* legend = root.add_child<SVG::Group>();
    legend->layout_bbox({ 0, 20, 0, 10 });

    legend->snap_to(*plot, SVG::RelativeAlignment::Right);
    root.autoscale(SVG::NO_MARGINS);

    REQUIRE(legend->get_attr("transform") == "translate(110.0 40.0)");
    REQUIRE(root.get_attr("viewBox") == "10.0 20.0 120.0 50.0");
}

TEST_CASE("snap_to supports centered relative alignment with offset", "[layout]") {
    SVG::Rect plot(10, 20, 100, 50);
    SVG::Group legend;
    legend.layout_bbox({ 0, 20, 0, 10 });

    legend.snap_to(plot, SVG::RelativeAlignment::Right, { 12, 0 });

    REQUIRE(legend.get_attr("transform") == "translate(122.0 40.0)");
}

TEST_CASE("snap_to supports explicit anchor and offset", "[layout]") {
    SVG::Rect plot(10, 20, 100, 50);
    SVG::Group legend;
    legend.layout_bbox({ 0, 20, 0, 10 });

    legend.snap_to(plot, SVG::RelativeAlignment::Bottom | SVG::Anchor::End, { 0, 8 });

    REQUIRE(legend.get_attr("transform") == "translate(90.0 78.0)");
}

TEST_CASE("snap_to accepts bitwise relative and edge alignment", "[layout]") {
    SVG::Rect plot(10, 20, 100, 50);
    SVG::Group title;
    title.layout_bbox({ 0, 40, 0, 12 });

    title.snap_to(plot, SVG::RelativeAlignment::Top | SVG::Anchor::Start, { 0, -4 });

    REQUIRE(title.get_attr("transform") == "translate(10.0 4.0)");
}

TEST_CASE("snap_to rejects ambiguous alignment", "[layout]") {
    SVG::Rect plot(0, 0, 10, 10);
    SVG::Group legend;
    legend.layout_bbox({ 0, 10, 0, 10 });

    const auto ambiguous = static_cast<SVG::Alignment>(SVG::RelativeAlignment::Left) |
        static_cast<SVG::Alignment>(SVG::RelativeAlignment::Right) |
        static_cast<SVG::Alignment>(SVG::Anchor::Center);

    REQUIRE_THROWS_AS(legend.snap_to(plot, ambiguous), std::invalid_argument);
}

TEST_CASE("align_to aligns anchors along the selected axis", "[layout]") {
    SVG::Rect plot(10, 20, 100, 50);
    SVG::Group title;
    title.layout_bbox({ 0, 40, 0, 12 });

    title.align_to(plot, SVG::Axis::X, SVG::Anchor::Center, { 0, -8 });

    REQUIRE(title.get_attr("transform") == "translate(40.0 -8.0)");
}

TEST_CASE("align_to defaults to center anchor", "[layout]") {
    SVG::Rect plot(10, 20, 100, 50);
    SVG::Group legend;
    legend.layout_bbox({ 0, 20, 0, 10 });

    legend.align_to(plot, SVG::Axis::Y, { 12, 0 });

    REQUIRE(legend.get_attr("transform") == "translate(12.0 40.0)");
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
