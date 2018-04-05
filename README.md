# SVG for C++

## Purpose
This a header-only library for generating SVG files from a simple C++ interface. It can also perform non-trivial tasks such as calculating a bounding box for an SVG's elements, or merging several graphics together.

[Want to see more? Read the documentation.](https://vincentlaucsb.github.io/svg/)

## Basic Usage

```
#include "svg.hpp"
#include <fstream>

int main() {
    SVG::SVG root;

    // Basic CSS support
    root.style("circle").set_attr("fill", "#000000")
        .set_attr("stroke", "#000000");
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

### Output

```
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

## Simple Animations
This package supports creating basic animations via CSS keyframes via the frame_animate() function.