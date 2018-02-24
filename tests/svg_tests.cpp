#define CATCH_CONFIG_MAIN
#include "catch.hpp"
#include "svg.hpp"

TEST_CASE("Proper Indentation", "[indent_test]") {
  SVG::SVG root;
  SVG::Circle circ;
  root.add_child(circ);
  std::string correct = "<svg xmlns=\"http://www.w3.org/2000/svg\">\n"
    "\t<circle />\n"
    "</svg>";

  REQUIRE(root.to_string() == correct);
}

TEST_CASE("Proper Indentation - Nested", "[indent_nest_test]") {
  SVG::SVG root;
  SVG::Group circ_container;
  circ_container.add_child(SVG::Circle(), SVG::Circle());
  root.add_child(circ_container);

  std::string correct = "<svg xmlns=\"http://www.w3.org/2000/svg\">\n"
    "\t<g>\n"
    "\t\t<circle />\n"
    "\t\t<circle />\n"
    "\t</g>\n"
    "</svg>";

  REQUIRE(root.to_string() == correct);
}