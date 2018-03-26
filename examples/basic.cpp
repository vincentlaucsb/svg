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

    // Reference elements by id (or class name)
    root.get_element_by_id("my_rectangle")
        ->set_attr("x", 20).set_attr("y", 20)
        .set_attr("width", 40).set_attr("height", 40);

    // Get information about SVG elements
    std::cout << "There are " << root.get_children<SVG::Circle>().size() <<
        " circles." << std::endl;

    // Automatically scale width and height to fit elements
    root.autoscale();

    // Output our drawing
    std::ofstream outfile("my_drawing.svg");
    outfile << std::string(root);
}