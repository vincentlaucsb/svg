#include <catch2/catch.hpp>
#include "svg.hpp"
#include "test_helpers.hpp"

namespace {
    class ClearableGroup : public SVG::Group {
    public:
        using SVG::Group::Group;

        void clear_for_test() {
            clear_children();
        }
    };
}

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

TEST_CASE("range-for iterates elements in depth-first document order", "[traversal]") {
    SVG::SVG root;
    auto* group = root.add_child<SVG::Group>();
    auto* rect = group->add_child<SVG::Rect>();
    auto* circle = group->add_child<SVG::Circle>();

    std::vector<SVG::Element*> elements;
    for (auto* element : root) elements.push_back(element);

    REQUIRE(elements == std::vector<SVG::Element*>({ &root, root.css, group, rect, circle }));
}

TEST_CASE("const range-for iterates elements in depth-first document order", "[traversal]") {
    SVG::SVG root;
    auto* group = root.add_child<SVG::Group>();
    auto* rect = group->add_child<SVG::Rect>();
    const SVG::SVG& const_root = root;

    std::vector<const SVG::Element*> elements;
    for (const auto* element : const_root) elements.push_back(element);

    REQUIRE(elements == std::vector<const SVG::Element*>({ &root, root.css, group, rect }));
}

TEST_CASE("const descendants iterates without including the root", "[traversal]") {
    SVG::SVG root;
    auto* group = root.add_child<SVG::Group>();
    auto* rect = group->add_child<SVG::Rect>();
    const SVG::SVG& const_root = root;

    std::vector<const SVG::Element*> elements;
    for (const auto* element : const_root.descendants()) elements.push_back(element);

    REQUIRE(elements == std::vector<const SVG::Element*>({ root.css, group, rect }));
}

TEST_CASE("depth_first can prune nested SVG descendants", "[traversal]") {
    SVG::SVG root;
    auto* nested = root.add_child<SVG::SVG>();
    auto* nested_rect = nested->add_child<SVG::Rect>();
    auto* circle = root.add_child<SVG::Circle>();

    std::vector<SVG::Element*> elements;
    for (auto* element : root.depth_first(SVG::Element::TraversalOptions(false))) {
        elements.push_back(element);
    }

    REQUIRE(elements == std::vector<SVG::Element*>({ &root, root.css, nested, circle }));
    REQUIRE(std::find(elements.begin(), elements.end(), nested_rect) == elements.end());
}

TEST_CASE("const pruned traversal preserves nested SVG transforms", "[traversal]") {
    SVG::SVG root;
    auto* nested = root.add_child<SVG::SVG>(SVG::Attrs{
        { "x", "5" },
        { "y", "7" },
        { "width", "10" },
        { "height", "10" }
    });
    nested->transform().translate(12, 14);
    nested->add_child<SVG::Rect>(0, 0, 100, 100);
    const SVG::SVG& const_root = root;

    auto elements = const_root.descendants(SVG::Element::TraversalOptions(false));
    auto it = elements.begin();
    REQUIRE(*it == root.css);
    ++it;
    REQUIRE(*it == nested);

    const auto origin = it.transform().apply({ 0, 0 });
    REQUIRE(origin.first == Approx(12));
    REQUIRE(origin.second == Approx(14));
}

TEST_CASE("autoscale traverses deeply nested documents without recursion", "[traversal]") {
    SVG::SVG root;
    SVG::Element* current = &root;
    for (int i = 0; i < 2048; ++i) {
        current = current->add_child<SVG::Group>();
    }
    current->add_child<SVG::Rect>(2, 3, 10, 5);

    root.autoscale({ 1, 1, 2, 2 });

    REQUIRE(root.get_attr("viewBox") == "1.0 1.0 12.0 9.0");
}

TEST_CASE("Templated get_children filters descendants by exact type", "[traversal]") {
    SVG::SVG root = two_circles();
    std::vector<SVG::SVG*> containers = root.get_children<SVG::SVG>();
    std::vector<SVG::Group*> groups = root.get_children<SVG::Group>();
    std::vector<SVG::Circle*> circles = root.get_children<SVG::Circle>();

    REQUIRE(SVG::tag_name(SVG::Circle::static_kind) == "circle");
    REQUIRE(SVG::tag_name(SVG::Title::static_kind) == "title");
    REQUIRE(groups.front()->kind() == SVG::ElementKind::Group);
    REQUIRE(circles.front()->kind() == SVG::ElementKind::Circle);
    REQUIRE(containers.size() == 0);
    REQUIRE(groups.size() == 1);
    REQUIRE(circles.size() == 2);
}

TEST_CASE("Built-in elements return their specific kinds", "[traversal]") {
    SVG::Title title("Chart summary");
    SVG::Circle circle;

    REQUIRE(title.kind() == SVG::ElementKind::Title);
    REQUIRE(circle.kind() == SVG::ElementKind::Circle);
}

TEST_CASE("G aliases Group", "[traversal]") {
    SVG::G group;

    REQUIRE(group.kind() == SVG::ElementKind::Group);
    REQUIRE(SVG::tag_name(group.kind()) == "g");
}

TEST_CASE("get_element_by_id finds nested elements", "[traversal]") {
    SVG::SVG root;
    auto group = root.add_child<SVG::Group>();
    auto rect = group->add_child<SVG::Rect>("workout");

    REQUIRE(root.get_element_by_id("workout") == rect);
    REQUIRE(root.get_element_by_id("missing") == nullptr);

    rect->id("sets");
    REQUIRE(root.get_element_by_id("workout") == nullptr);
    REQUIRE(root.get_element_by_id("sets") == rect);
    REQUIRE(root.get_element_by_id<SVG::Rect>("sets") == rect);
    REQUIRE(root.get_element_by_id<SVG::Circle>("sets") == nullptr);

    rect->set_attr("id") << "volume";
    REQUIRE(root.get_element_by_id("sets") == nullptr);
    REQUIRE(root.get_element_by_id("volume") == rect);

    rect->set_attr("id");
    REQUIRE(rect->get_attr("id").empty());
    REQUIRE(root.get_element_by_id("volume") == nullptr);

    rect->set_attr("id", "reps");
    REQUIRE(root.get_element_by_id("reps") == rect);

    auto* other = group->add_child<SVG::Rect>();
    REQUIRE_THROWS_AS(other->id("reps"), std::invalid_argument);
}

TEST_CASE("Elements expose their parent after add_child", "[traversal]") {
    SVG::SVG root;
    auto group = root.add_child<SVG::Group>();
    auto rect = group->add_child<SVG::Rect>();

    REQUIRE(root.parent() == nullptr);
    REQUIRE(root.css->parent() == &root);
    REQUIRE(group->parent() == &root);
    REQUIRE(rect->parent() == group);
}

TEST_CASE("Moved subtrees update parent pointers", "[traversal]") {
    SVG::SVG root;
    SVG::Group group;
    auto rect = group.add_child<SVG::Rect>();
    REQUIRE(rect->parent() == &group);

    root << std::move(group);

    const auto groups = root.get_immediate_children<SVG::Group>();
    REQUIRE(groups.size() == 1);
    const auto moved_group = groups.front();
    const auto rects = moved_group->get_immediate_children<SVG::Rect>();
    REQUIRE(rects.size() == 1);
    REQUIRE(moved_group->parent() == &root);
    REQUIRE(rects.front()->parent() == moved_group);
}

TEST_CASE("Moved subtrees register ids with their new SVG root", "[traversal]") {
    SVG::Group group;
    group.add_child<SVG::Rect>("detached");

    SVG::SVG root;
    REQUIRE(root.get_element_by_id("detached") == nullptr);

    root << std::move(group);

    auto* rect = root.get_element_by_id<SVG::Rect>("detached");
    REQUIRE(rect != nullptr);
    REQUIRE(root.get_element_by_id<SVG::Circle>("detached") == nullptr);

    rect->id("attached");
    REQUIRE(root.get_element_by_id("detached") == nullptr);
    REQUIRE(root.get_element_by_id("attached") == rect);
}

TEST_CASE("Clearing children unregisters descendant ids", "[traversal]") {
    SVG::SVG root;
    auto* group = root.add_child<ClearableGroup>();
    group->add_child<SVG::Rect>();
    auto* child = group->add_child<SVG::Group>("child");
    child->add_child<SVG::Rect>("leaf");

    REQUIRE(root.get_element_by_id("child") == child);
    REQUIRE(root.get_element_by_id("leaf") != nullptr);

    group->clear_for_test();

    REQUIRE(group->get_immediate_children<SVG::Element>().empty());
    REQUIRE(root.get_element_by_id("child") == nullptr);
    REQUIRE(root.get_element_by_id("leaf") == nullptr);
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

TEST_CASE("SVG clone deep copies elements and preserves id lookup", "[traversal]") {
    SVG::SVG root;
    root.style(".mark").set_attr("fill", SVG::Color::hex("#ff4fd8"));
    auto* group = root.add_child<SVG::Group>(SVG::Attrs{{ "id", "layer" }});
    auto* rect = group->add_child<SVG::Rect>(
        1, 2, 3, 4,
        SVG::Attrs{{ "id", "mark" }, { "class", "mark" }});

    auto copy = root.clone();
    auto* copied_rect = copy.get_element_by_id<SVG::Rect>("mark");
    const SVG::SVG& const_copy = copy;
    const auto* const_rect = const_copy.get_element_by_id<SVG::Rect>("mark");

    REQUIRE(copied_rect != nullptr);
    REQUIRE(copied_rect != rect);
    REQUIRE(const_rect == copied_rect);
    REQUIRE(const_copy.get_children<SVG::Rect>().size() == 1);
    REQUIRE(const_copy.get_elements_by_class("mark").size() == 1);

    copied_rect->set_attr("fill", SVG::Color::hex("#44ff88"));

    REQUIRE(rect->get_attr("fill").empty());
    REQUIRE(copied_rect->get_attr("fill") == "#44ff88");
    REQUIRE(std::string(const_copy).find("fill=\"#44ff88\"") != std::string::npos);
}

TEST_CASE("SVG copy construction deep copies documents", "[traversal]") {
    SVG::SVG root;
    root.add_child<SVG::Circle>("dot");

    SVG::SVG constructed(root);

    REQUIRE(constructed.get_element_by_id<SVG::Circle>("dot") != root.get_element_by_id<SVG::Circle>("dot"));
    REQUIRE(constructed.get_element_by_id<SVG::Circle>("dot") != nullptr);
}
