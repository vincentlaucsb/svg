#pragma once
#define PI 3.14159265
#define SVG_TYPE_CHECK static_assert(std::is_base_of<Element, T>::value, "Child must be an SVG element.")
#include <iostream>
#include <algorithm> // min, max
#include <fstream>   // ofstream
#include <math.h>    // NAN
#include <map>
#include <deque>
#include <vector>
#include <string>
#include <sstream> // stringstream
#include <iomanip> // setprecision
#include <memory>
#include <type_traits> // is_base_of
#include <typeinfo>

namespace SVG {
    using SVGAttrib = std::map<std::string, std::string>;
    using Point = std::pair<double, double>;

    inline std::string to_string(const double& value);
    inline std::string to_string(const Point& point);

    struct QuadCoord {
        double x1;
        double x2;
        double y1;
        double y2;
    };

    using Margins = QuadCoord;
    const static Margins DEFAULT_MARGINS = { 10, 10, 10, 10 };
    const static Margins NO_MARGINS = { 0, 0, 0, 0 };

    namespace util {
        /** Various utility and mathematical functions */
        enum Orientation {
            COLINEAR, CLOCKWISE, COUNTERCLOCKWISE
        };

        inline Orientation orientation(Point& p1, Point& p2, Point& p3) {
            double value = ((p2.second - p1.second) * (p3.first - p2.first) -
                (p2.first - p1.first) * (p3.second - p2.second));
            
            if (value == 0) return COLINEAR;
            else if (value > 0) return CLOCKWISE;
            else return COUNTERCLOCKWISE;
        }

        inline std::vector<Point> convex_hull(std::vector<Point>& points) {
            /** Compute the convex hull of a set of points via Jarvis'
             *  gift wrapping algorithm
             *
             *  Ref: https://www.geeksforgeeks.org/convex-hull-set-1-jarviss-algorithm-or-wrapping/
             */

            if (points.size() < 3) return {}; // Need at least three points
            std::vector<Point> hull;

            // Find leftmost point (ties don't matter)
            int left = 0;
            for (size_t i = 0; i < points.size(); i++)
                if (points[i].first < points[left].first) left = (int)i;
            
            // While we don't reach leftmost point
            int current = left, next;
            do {
                // Add to convex hull
                hull.push_back(points[current]);

                // Keep moving counterclockwise
                next = (current + 1) % points.size();
                for (size_t i = 0; i < points.size(); i++) {
                    // We've found a more counterclockwise point --> update next
                    if (orientation(points[current], points[next], points[i]) == COUNTERCLOCKWISE)
                        next = (int)i;
                }

                current = next;
            } while (current != left);

            return hull;
        }

        /** Base class for anything that has attributes */
        class AttributeMap {
        public:
            AttributeMap() = default;
            AttributeMap(SVGAttrib _attr) : attr(_attr) {};
            SVGAttrib attr;

            template<typename T>
            AttributeMap& set_attr(const std::string key, T value) {
                this->attr[key] = std::to_string(value);
                return *this;
            }
        };

        template<>
        inline AttributeMap& AttributeMap::set_attr(const std::string key, const double value) {
            this->attr[key] = to_string(value);
            return *this;
        }

        template<>
        inline AttributeMap& AttributeMap::set_attr(const std::string key, const char * value) {
            this->attr[key] = value;
            return *this;
        }

        template<>
        inline AttributeMap& AttributeMap::set_attr(const std::string key, const std::string value) {
            this->attr[key] = value;
            return *this;
        }
    }

    inline std::string to_string(const double& value) {
        /** Trim off all but one decimal place when converting a double to string */
        std::stringstream ss;
        ss << std::fixed << std::setprecision(1);
        ss << value;
        return ss.str();
    }

    inline std::string to_string(const Point& point) {
        return to_string(point.first) + "," + to_string(point.second);
    }

    class Element: public util::AttributeMap {
    public:
        using BoundingBox = QuadCoord;
        using ChildList = std::vector<Element*>;
        using ChildMap = std::map<std::string, ChildList>;

        Element() = default;
        Element(const char* id) : AttributeMap(
            SVGAttrib({ { "id", id } })) {};
        using util::AttributeMap::AttributeMap;

        // Implicit string conversion
        operator std::string() { return this->svg_to_string(0); };

        template<typename T, typename... Args>
        T* add_child(Args&&... args) {
            /** Also return a pointer to the element added */
            SVG_TYPE_CHECK;
            this->children.push_back(std::make_unique<T>(std::forward<Args>(args)...));
            return (T*)this->children.back().get();
        }

        template<typename T>
        Element& operator<<(T&& node) {
            SVG_TYPE_CHECK;
            this->children.push_back(std::make_unique<T>(std::move(node)));
            return *this;
        }

        template<typename T>
        std::vector<T*> get_children() {
            SVG_TYPE_CHECK;
            std::vector<T*> ret;
            auto child_elems = this->get_children_helper();
            
            for (auto& child: child_elems)
                if (typeid(*child) == typeid(T)) ret.push_back((T*)child);

            return ret;
        }

        Element* get_element_by_id(const std::string& id);
        std::vector<Element*> get_elements_by_class(const std::string& clsname);
        void autoscale(const Margins& margins=DEFAULT_MARGINS);
        virtual BoundingBox get_bbox();
        ChildList get_immediate_children(const std::string tag="");
        ChildMap get_children();

    protected:
        std::vector<std::unique_ptr<Element>> children;
        std::vector<Element*> get_children_helper();
        void get_bbox(Element::BoundingBox&);
        virtual std::string svg_to_string(const size_t indent_level);
        virtual std::string tag() = 0;

        double find_numeric(const std::string& key) {
            /** Return a numeric value or NAN */
            if (attr.find(key) != attr.end())
                return std::stof(attr[key]);
            return NAN;
        }
    };

    inline Element* Element::get_element_by_id(const std::string &id) {
        /** Return the SVG element that has a certain id */
        auto child_elems = this->get_children_helper();
        for (auto& current: child_elems)
            if (current->attr.find("id") != current->attr.end() && 
                current->attr.find("id")->second == id) return current;
        
        return nullptr;
    }

    inline std::vector<Element*> Element::get_elements_by_class(const std::string &clsname) {
        /** Return all SVG elements with a certain class name */
        std::vector<Element*> ret;
        auto child_elems = this->get_children_helper();
    
        for (auto& current: child_elems) {
            if ((current->attr.find("class") != current->attr.end())
                && (current->attr.find("class")->second == clsname))
                ret.push_back(current);
        }
    
        return ret;
    }

    inline Element::ChildList Element::get_immediate_children(const std::string tag) {
        /** Return immediate children that have a given tag (or return all otherwise) */
        Element::ChildList ret;
        for (auto& child: this->children)
            if (tag.empty() || child->tag() == tag) ret.push_back(child.get());
        return ret;
    }

    inline Element::BoundingBox Element::get_bbox() {
        /** Compute the bounding box necessary to contain this element */
        return { NAN, NAN, NAN, NAN };
    }

    class Shape: public Element {
        /** Abstract base class for any SVG elements that have a width and height */
    public:
        using Element::Element;

        virtual std::vector<Point> points() {
            /** Return a set of points used for calculating a bounding polygon for this object */
            auto bbox = this->get_bbox();
            return {
                Point(bbox.x1, bbox.y1), // Top left
                Point(bbox.x2, bbox.y1), // Top right
                Point(bbox.x1, bbox.y2), // Bottom left
                Point(bbox.x2, bbox.y2)  // Bottom right
            };
        }

        virtual double x() { return this->find_numeric("x"); }
        virtual double y() { return this->find_numeric("y"); }
        virtual double width() { return this->find_numeric("width"); }
        virtual double height() { return this->find_numeric("height"); }
    };

    std::vector<Point> bounding_polygon(const std::vector<Shape*>& shapes);

    class Style : public Element {
    public:
        Style() = default;
        using Element::Element;
        std::map<std::string, util::AttributeMap> css;

    protected:
        std::string svg_to_string(const size_t) override;
        std::string tag() override { return "style"; };
    };

    class SVG : public Shape {
    public:
        SVG(std::map < std::string, std::string > _attr =
                { { "xmlns", "http://www.w3.org/2000/svg" } }
        ) : Shape(_attr) {};
        void merge(SVG& right, const Margins& margins=DEFAULT_MARGINS);
        util::AttributeMap& style(const std::string& key);
        Style* css = nullptr;

    protected:
        std::string tag() override { return "svg"; }
    };

    class Path : public Shape {
    public:
        using Shape::Shape;

        template<typename T>
        inline void start(T x, T y) {
            /** Start line at (x, y)
            *  This function overwrites the current path if it exists
            */
            this->attr["d"] = "M " + std::to_string(x) + " " + std::to_string(y);
            this->x_start = x;
            this->y_start = y;
        }

        template<typename T>
        inline void line_to(T x, T y) {
            /** Draw a line to (x, y)
            *  If line has not been initialized by setting a starting point,
            *  then start() will be called with (x, y) as arguments
            */

            if (this->attr.find("d") == this->attr.end())
                start(x, y);
            else
                this->attr["d"] += " L " + std::to_string(x) +
                                   " " + std::to_string(y);
        }

        inline void line_to(std::pair<double, double> coord) {
            this->line_to(coord.first, coord.second);
        }

        inline void to_origin() {
            /** Draw a line back to the origin */
            this->line_to(x_start, y_start);
        }

    protected:
        std::string tag() override { return "path"; }

    private:
        double x_start;
        double y_start;
    };

    class Text : public Element {
    public:
        Text() = default;
        using Element::Element;

        Text(double x, double y, std::string _content) {
            set_attr("x", to_string(x));
            set_attr("y", to_string(y));
            content = _content;
        }

        Text(std::pair<double, double> xy, std::string _content) :
                Text(xy.first, xy.second, _content) {};

    protected:
        std::string content;
        std::string svg_to_string(const size_t) override;
        std::string tag() override { return "text"; }
    };

    class Group : public Element {
    public:
        using Element::Element;
    protected:
        std::string tag() override { return "g"; }
    };

    class Line : public Shape {
    public:
        Line() = default;
        using Shape::Shape;

        Line(double x1, double x2, double y1, double y2) : Shape({
                { "x1", to_string(x1) },
                { "x2", to_string(x2) },
                { "y1", to_string(y1) },
                { "y2", to_string(y2) }
        }) {};

        double x1() { return std::stof(this->attr["x1"]); }
        double x2() { return std::stof(this->attr["x2"]); }
        double y1() { return std::stof(this->attr["y1"]); }
        double y2() { return std::stof(this->attr["y2"]); }

        double width() override { return std::abs(x2() - x1()); }
        double height() override { return std::abs(y2() - y1()); }
        double get_length() { return std::sqrt(pow(width(), 2) + pow(height(), 2)); }
        double get_slope() { return (y2() - y1()) / (x2() - x1()); }

        std::pair<double, double> along(double percent);

    protected:
        Element::BoundingBox get_bbox() override;
        std::string tag() override { return "line"; }
    };

    class Rect : public Shape {
    public:
        Rect() = default;
        using Shape::Shape;

        Rect(
            double x, double y, double width, double height) :
            Shape({
                    { "x", to_string(x) },
                    { "y", to_string(y) },
                    { "width", to_string(width) },
                    { "height", to_string(height) }
            }) {};

        Element::BoundingBox get_bbox() override;
    protected:
        std::string tag() override { return "rect"; }
    };

    class Circle : public Shape {
    public:
        Circle() = default;
        using Shape::Shape;

        Circle(double cx, double cy, double radius) :
                Shape({
                        { "cx", to_string(cx) },
                        { "cy", to_string(cy) },
                        { "r", to_string(radius) }
                }) {
        };

        Circle(std::pair<double, double> xy, double radius) : Circle(xy.first, xy.second, radius) {};
        double radius() { return this->find_numeric("r"); }
        virtual double x() override { return this->find_numeric("cx"); }
        virtual double y() override { return this->find_numeric("cy"); }
        virtual double width() override { return this->radius() * 2; }
        virtual double height() override { return this->width(); }
        Element::BoundingBox get_bbox() override;

    protected:
        std::string tag() override { return "circle"; }
    };

    class Polygon : public Element {
    public:
        Polygon() = default;
        using Element::Element;

        Polygon(const std::vector<Point>& points) {
            // Quick and dirty
            std::string& point_str = this->attr["points"];
            for (auto& pt : points)
                point_str += to_string(pt) + " ";
        };

    protected:
        std::string tag() override { return "polygon"; }
    };

    inline Element::BoundingBox Line::get_bbox() {
        return { x1(), x2(), y1(), y2() };
    }

    inline Element::BoundingBox Rect::get_bbox() {
        double x = this->x(), y = this->y(),
            width = this->width(), height = this->height();
        return { x, x + width, y, y + height };
    }

    inline Element::BoundingBox Circle::get_bbox() {
        double x = this->x(), y = this->y(), radius = this->radius();

        return {
            x - radius,
            x + radius,
            y - radius,
            y + radius
        };
    }

    inline std::pair<double, double> Line::along(double percent) {
        /** Return the coordinates required to place an element along
         *   this line
         */

        double x_pos, y_pos;

        if (x1() != x2()) {
            double length = percent * this->get_length();
            double discrim = std::sqrt(4 * pow(length, 2) * (1 / (1 + pow(get_slope(), 2))));

            double x_a = (2 * x1() + discrim) / 2;
            double x_b = (2 * x1() - discrim) / 2;
            x_pos = x_a;

            if ((x_a > x1() && x_a > x2()) || (x_a < x1() && x_a < x2()))
                x_pos = x_b;

            y_pos = get_slope() * (x_pos - x1()) + y1();
        }
        else { // Edge case:: Completely vertical lines
            x_pos = x1();

            if (y1() > y2()) // Downward pointing
                y_pos = y1() - percent * this->get_length();
            else
                y_pos = y1() + percent * this->get_length();
        }

        return std::make_pair(x_pos, y_pos);
    }

    inline util::AttributeMap& SVG::style(const std::string& key) {
        if (!this->css) this->css = this->add_child<Style>();
        return this->css->css[key];
    }

    inline std::string Element::svg_to_string(const size_t indent_level) {
        auto indent = std::string(indent_level, '\t');
        std::string ret = indent + "<" + tag();

        // Set attributes
        for (auto& pair: attr)
            ret += " " + pair.first + "=" + "\"" + pair.second + "\"";

        if (!this->children.empty()) {
            ret += ">\n";

            // Recursively get strings for child elements
            for (auto& child: children)
                ret += child->svg_to_string(indent_level + 1) + "\n";

            return ret += indent + "</" + tag() + ">";
        }

        return ret += " />";
    }

    inline std::string Style::svg_to_string(const size_t indent_level) {
        auto indent = std::string(indent_level, '\t');
        std::string ret = indent + "<style type=\"text/css\">\n" +
            indent + "\t<![CDATA[\n";

        // Begin CSS stylesheet
        for (auto& selector: this->css) {
            // Loop over each selector's attribute/value pairs
            ret += indent + "\t\t" + selector.first + " {\n";
            for (auto& attr: selector.second.attr)
                ret += indent + "\t\t\t" + attr.first + ": " + attr.second + ";\n";
            ret += indent + "\t\t" + "}\n";
        }

        ret += indent + "\t]]>\n";
        return ret + indent + "</style>";
    }

    inline std::string Text::svg_to_string(const size_t indent_level) {
        auto indent = std::string(indent_level, '\t');
        std::string ret = indent + "<text";
        for (auto& pair: attr)
            ret += " " + pair.first + "=" + "\"" + pair.second + "\"";
        return ret += ">" + this->content + "</text>";
    }

    inline void Element::autoscale(const Margins& margins) {
        /** Modify this element's attributes so it can hold all of its child elements */
        using std::stof;

        Element::BoundingBox bbox = this->get_bbox();
        this->get_bbox(bbox); // Compute the bounding box (recursive)
        double width = abs(bbox.x1) + abs(bbox.x2) + margins.x1 + margins.x2,
            height = abs(bbox.y1) + abs(bbox.y2) + margins.y1 + margins.y2,
            x1 = bbox.x1 - margins.x1, y1 = bbox.y1 - margins.y1;

        this->set_attr("width", width)
             .set_attr("height", height);

        if (x1 < 0 || y1 < 0) {
            std::stringstream viewbox;
            viewbox << std::fixed << std::setprecision(1)
                << x1 << " " // min-x
                << y1 << " " // min-y
                << width << " "
                << height;
            this->set_attr("viewBox", viewbox.str());
        }
    }

    inline void Element::get_bbox(Element::BoundingBox& box) {
        /** Recursively compute a bounding box */
        auto this_bbox = this->get_bbox();
        if (isnan(box.x1) || this_bbox.x1 < box.x1) box.x1 = this_bbox.x1;
        if (isnan(box.x2) || this_bbox.x2 > box.x2) box.x2 = this_bbox.x2;
        if (isnan(box.y1) || this_bbox.y1 < box.y1) box.y1 = this_bbox.y1;
        if (isnan(box.y2) || this_bbox.y2 > box.y2) box.y2 = this_bbox.y2;

        // Recursion
        for (auto& child: this->children) child->get_bbox(box);
    }

    inline Element::ChildMap Element::get_children() {
        /** Recursively compute all of the children of an SVG element */
        Element::ChildMap child_map;
        for (auto& child : this->get_children_helper())
            child_map[child->tag()].push_back(child);
        return child_map;
    }

    inline std::vector<Element*> Element::get_children_helper() {
        /** Helper function which populates a std::deque with all of an Element's children */
        std::deque<Element*> temp;
        std::vector<Element*> ret;

        for (auto& child : this->children) { temp.push_back(child.get()); }
        while (!temp.empty()) {
            ret.push_back(temp.front());
            for (auto& child : temp.front()->children) { temp.push_back(child.get()); }
            temp.pop_front();
        }

        return ret;
    };

    inline void SVG::merge(SVG& right, const Margins& margins) {
        /** Merge two SVG documents together horizontally with a uniform margin */

        // Move left
        auto left = std::make_unique<SVG>(SVG());
        for (auto& child: this->children)
            left->children.push_back(std::move(child));
        this->children.clear();
        this->children.push_back(std::move(left));

        // Move right
        this->children.push_back(std::make_unique<SVG>(std::move(right)));

        // Set bounding box of individual pieces
        for (auto& svg_child: this->get_immediate_children("svg"))
            svg_child->autoscale(margins);

        // Set x position for child SVG elements, and compute width/height for this
        double x = 0, height = 0;
        for (auto& svg_child: this->get_immediate_children("svg")) {
            svg_child->set_attr("x", x).set_attr("y", 0);
            x += ((SVG*)svg_child)->width();
            height = std::max(height, ((SVG*)svg_child)->height());
        }

        this->set_attr("width", x).set_attr("height", height);
    }

    inline std::vector<Point> bounding_polygon(std::vector<Shape*>& shapes) {
        // Convert shapes into sets of points, aggregate them, and then calculate
        // convex hull for aggregate set
        std::vector<Point> points;
        for (auto& shp : shapes) {
            auto temp_points = shp->points();
            std::move(temp_points.begin(), temp_points.end(), std::back_inserter(points));
        }

        return util::convex_hull(points);
    }
}