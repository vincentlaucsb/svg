#pragma once

#include "svg.hpp"

inline SVG::SVG two_circles(int x = 0, int y = 0, int r = 0) {
    SVG::SVG root;
    auto circ_container = root.add_child<SVG::Group>();
    (*circ_container) << SVG::Circle(x, y, r) << SVG::Circle(x, y, r);
    return root;
}
