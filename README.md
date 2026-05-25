<p align="center">
  <img src="examples/svg-open-sign.svg" alt="SVG for C++" width="680">
  <br>
  <a href="examples/open_sign.cpp">See the C++ code for this logo</a>
  <br>
  <a href="examples/svg-open-sign.svg"><img src="examples/svg-open-sign-swatch.svg" alt="pink logo" width="64"></a>
  <a href="examples/svg-open-sign-green.svg"><img src="examples/svg-open-sign-green-swatch.svg" alt="green logo" width="64"></a>
  <a href="examples/svg-open-sign-blue.svg"><img src="examples/svg-open-sign-blue-swatch.svg" alt="blue logo" width="64"></a>
  <a href="examples/svg-open-sign-red.svg"><img src="examples/svg-open-sign-red-swatch.svg" alt="red logo" width="64"></a>
</p>

# SVG for C++

[![C++](https://github.com/vincentlaucsb/svg/actions/workflows/cpp.yml/badge.svg)](https://github.com/vincentlaucsb/svg/actions/workflows/cpp.yml) [![codecov](https://codecov.io/gh/vincentlaucsb/svg/branch/master/graph/badge.svg)](https://codecov.io/gh/vincentlaucsb/svg)

## Purpose
This a header-only library for generating SVG files from a simple C++ interface. It can also perform non-trivial tasks such as calculating a bounding box for an SVG's elements, or merging several graphics together.

[Want to see more? Read the documentation.](https://vincentlaucsb.github.io/svg/)

## Basic Usage

```cpp
#include "svg.hpp"
#include <fstream>

int main() {
    SVG::SVG root;

    // Basic CSS support
    auto black = SVG::Color::hex("#000000");
    root.style("circle").set_attr("fill", black)
        .set_attr("stroke", black);
    root.style("rect#my_rectangle").set_attr("fill", "red");

    // Method 1 of adding elements - add_child<>()
    auto shapes = root.add_child<SVG::Group>();
    auto rect = shapes->add_child<SVG::Rect>("my_rectangle");

    // Method 2 of adding elements - operator<<
    *shapes << SVG::Circle(-100, -100, 100) << SVG::Circle(100, 100, 100);

    // Reference elements by id, tag, class name, etc...
    root.get_element_by_id("my_rectangle")
        ->set_attr("x", 20).set_attr("y", 20)
        .set_attr("width", 40).set_attr("height", 40);

    std::cout << "There are " << root.get_children<SVG::Circle>().size() <<
        " circles." << std::endl;

    // Automatically scale width and height to fit elements
    root.autoscale();

    // Output our drawing
    std::ofstream outfile("my_drawing.svg");
    outfile << std::string(root);
}
```

`autoscale()` measures element geometry, including simple SVG `rotate(...)` transforms and approximate `Text` bounds from `x`, `y`, `dx`, `dy`, `font-size`, `text-anchor`, and baseline attributes. It sets `width`, `height`, and `viewBox`; use `responsive_autoscale()` when the SVG should calculate only `viewBox` and let its container control display size. It does not inspect CSS transforms, rendered effects such as filters or shadows, stroke joins, font metrics, or external stylesheets; pass `SVG::Margins` when the drawing needs extra page space for those effects.

### Output

```svg
<svg height="420.0" viewBox="-210.0 -210.0 420.0 420.0" width="420.0" xmlns="http://www.w3.org/2000/svg">
	<style type="text/css">
		<![CDATA[
			circle {
				fill: #000000;
				stroke: #000000;
			}
			rect#my_rectangle {
				fill: red;
			}
		]]>
	</style>
	<g>
		<rect height="40" id="my_rectangle" width="40" x="20" y="20" />
		<circle cx="-100.0" cy="-100.0" r="100.0" />
		<circle cx="100.0" cy="100.0" r="100.0" />
	</g>
</svg>
```

## Higher-Level Workflows

### Typed CSS Variables and Classes

Use `set_vars()` and `SVG::Classes` when a drawing has reusable theme tokens or repeated class names. Variable names are normalized with a leading `--`, class selectors can be built without hand-writing `"."`, and `vars.format()` is useful for CSS expressions that combine several variables.

```cpp
enum class PlotVar { axis, background, text_size };
enum class PlotClass { axis_line, label, muted };

SVG::SVG root;
auto vars = root.set_vars<PlotVar>({
    { PlotVar::axis, "plot-axis", "#374151" },
    { PlotVar::background, "--plot-background", "#ffffff" },
    { PlotVar::text_size, "--plot-text-size", "12px" }
});
SVG::Classes<PlotClass> classes({
    { PlotClass::axis_line, "axis-line" },
    { PlotClass::label, ".axis-label" },
    { PlotClass::muted, "muted" }
});

root.style(classes.selector(PlotClass::axis_line))
    .set_attr("stroke", vars.var(PlotVar::axis));
root.style(classes.selector(PlotClass::label, PlotClass::muted))
    .set_attr("font-size", vars.var(PlotVar::text_size))
    .set_attr("opacity", "0.72");

root.add_child<SVG::Line>(
    0, 0, 100, 0,
    SVG::Attrs{{ "class", classes.classes(PlotClass::axis_line) }});
```

### Class Lists and Queries

`class_list()` treats the `class` attribute as a normalized token list, so repeated whitespace and duplicate class tokens are cleaned up. Use `get_elements_by_class()` when you want token-aware matches instead of substring checks.

```cpp
auto* group = root.add_child<SVG::Group>();
auto* first = group->add_child<SVG::Circle>();
auto* second = group->add_child<SVG::Rect>();

first->class_list().add("chart").add("selected");
second->set_attr("class", " chart-muted ");

for (auto* element : root.get_elements_by_class("chart")) {
    element->set_attr("opacity", "0.9");
}
```

### Depth-First Iteration

Range-for over any SVG element visits that element and its descendants in depth-first document order. Use `descendants()` when you want to skip the current element. The iterator also exposes the accumulated supported SVG transform for each visited element through `it.transform()`.

```cpp
for (auto* element : root) {
    if (element->has_attr("data-debug")) {
        element->set_attr("stroke", "#ff00aa");
    }
}

auto pass = root.descendants();
for (auto it = pass.begin(); it != pass.end(); ++it) {
    SVG::Element* element = *it;
    const auto origin = it.transform().apply({ 0, 0 });

    if (element->has_attr("data-debug")) {
        element->set_attr("data-origin-x", origin.first);
        element->set_attr("data-origin-y", origin.second);
        element->set_attr("stroke", "#ff00aa");
    }
}
```

For type-specific searches, `get_children<T>()` returns matching descendants and `get_immediate_children<T>()` limits the lookup to direct children.

```cpp
for (auto* circle : root.get_children<SVG::Circle>()) {
    circle->set_attr("r", 8);
}
```

### Responsive ViewBoxes

Use `autoscale()` when the SVG should set its own display `width`, `height`, and `viewBox`. Use `responsive_autoscale()` when you want only the `viewBox` updated, leaving sizing to CSS or the embedding page.

```cpp
root.set_attr("width", "100%").set_attr("height", "auto");
root.responsive_autoscale({ 8, 8, 8, 8 });
```

When an element uses SVG features the library cannot measure well enough, provide an explicit layout bounding box for autoscale without changing the element's own `get_bbox()` result.

```cpp
auto* label = root.add_child<SVG::Text>(0, 0, user_supplied_label);
label->layout_bbox({ -4, 96, -18, 6 });
root.autoscale();
```

Use `snap_to()` to position measured elements against each other with SVG transforms. Passing only `RelativeAlignment` uses `Anchor::Center`, so offsets do not require spelling out the center anchor. Combine the two enums with `|` when you need start or end alignment along the shared edge. Use `align_to()` when elements should share an axis without becoming neighbors.

```cpp
legend->layout_bbox({ 0, 120, 0, 28 });
legend->snap_to(plot_area, SVG::RelativeAlignment::Right, { 12, 0 });

title->layout_bbox({ 0, 240, 0, 24 });
title->snap_to(plot_area, SVG::RelativeAlignment::Top | SVG::Anchor::Start, { 0, -8 });

callout->layout_bbox({ 0, 80, 0, 18 });
callout->align_to(plot_area, SVG::Axis::Y, SVG::Anchor::Center, { 16, 0 });
```

## Simple Animations
This package supports creating basic animations via CSS keyframes via the frame_animate() function.
