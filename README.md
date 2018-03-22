# SVG for C++

## Purpose
This a header-only library for generating SVG files from a simple C++ interface. It can also perform non-trivial tasks such as calculating a bounding box for an SVG's elements, or merging several graphics together.

## Basic Example
```
#include <fstream>

int main() {
    SVG::SVG root;
    
    // Basic CSS support
    auto style = root.add_child<SVG::Style>();
    style->css["circle"].set_attr("fill", "#000000")
        .set_attr("stroke", "#000000");
    
    auto shapes = root.add_child<SVG::Group>();
    (*shapes) << SVG::Circle(-100, -100, 100) << SVG::Circle(100, 100, 100);
    
    // Automatically scale width and height to fit elements
    root.autoscale();

    std::ofstream outfile("my_drawing.svg");
    outfile << root.to_string();

}

```
