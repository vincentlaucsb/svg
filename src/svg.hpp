#pragma once
#define PI 3.14159265
#include <iostream>
#include <algorithm> // min, max
#include <fstream>   // ofstream
#include <math.h>    // NAN
#include <unordered_map>
#include <map>
#include <deque>
#include <vector>
#include <string>
#include <memory>

using std::deque;
using std::vector;
using std::string;

namespace SVG {
    class Element {
    public:
        struct BoundingBox {
            float x1;
            float x2;
            float y1;
            float y2;
        };

        using ChildMap = std::map<std::string, std::vector<Element*>>;

        Element() {};
        Element(std::map < std::string, std::string > _attr) : attr(_attr) {};

        template<typename T>
        inline Element& set_attr(std::string key, T value) {
            this->attr[key] = std::to_string(value);
            return *this;
        }

        template<typename T, typename... Args>
        inline void add_child(T node, Args... args) {
            add_child(node);
            add_child(args...);
        }

        template<typename T>
        inline Element* add_child(T node) {
            /** Also return a pointer to the element added */
            this->children.push_back(std::make_shared<T>(node));
            return this->children.back().get();
        }

        inline virtual float get_width() {
            if (attr.find("width") != attr.end())
                return std::stof(attr["width"]);
            else
                return NAN;
        }

        inline virtual float get_height() {
            if (attr.find("height") != attr.end())
                return std::stof(attr["height"]);
            else
                return NAN;
        }

        virtual std::string to_string(const size_t indent_level = 0);
        void set_bbox();
        virtual BoundingBox get_bbox();
        ChildMap get_children();
        std::map < std::string, std::string > attr;

    protected:
        std::vector<std::shared_ptr<Element>> children;

        void set_bbox(Element::BoundingBox&);
        void get_children(ChildMap&);
        virtual std::string tag() = 0;
    };

    template<>
    inline Element& Element::set_attr(std::string key, const char * value) {
        this->attr[key] = value;
        return *this;
    }

    template<>
    inline Element& Element::set_attr(std::string key, const std::string value) {
        this->attr[key] = value;
        return *this;
    }

    inline Element::BoundingBox Element::get_bbox() {
        /** Compute the bounding box necessary to contain this element */
        return { NAN, NAN, NAN, NAN };
    }

    class SVG : public Element {
    public:
        SVG(std::map < std::string, std::string > _attr =
                { { "xmlns", "http://www.w3.org/2000/svg" } }
        ) : Element(_attr) {};
    protected:
        std::string tag() override { return "svg"; }
    };

    class Path : public Element {
    public:
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

        inline void line_to(std::pair<float, float> coord) {
            this->line_to(coord.first, coord.second);
        }

        inline void to_origin() {
            /** Draw a line back to the origin */
            this->line_to(x_start, y_start);
        }

    protected:
        std::string tag() override { return "path"; }

    private:
        float x_start;
        float y_start;
    };

    class Text : public Element {
    public:
        Text(float x, float y, std::string _content) {
            set_attr("x", std::to_string(x));
            set_attr("y", std::to_string(y));
            content = _content;
        }

        Text(std::pair<float, float> xy, std::string _content) :
                Text(xy.first, xy.second, _content) {};

        std::string to_string(const size_t) override;

    protected:
        std::string content;
        std::string tag() override { return "text"; }
    };

    class Group : public Element {
    protected:
        std::string tag() override { return "g"; }
    };

    class Line : public Element {
    public:
        Line() {};
        Line(float x1, float x2, float y1, float y2) : Element({
                { "x1", std::to_string(x1) },
                { "x2", std::to_string(x2) },
                { "y1", std::to_string(y1) },
                { "y2", std::to_string(y2) }
        }) {};

        inline float x1() { return std::stof(this->attr["x1"]); }
        inline float x2() { return std::stof(this->attr["x2"]); }
        inline float y1() { return std::stof(this->attr["y1"]); }
        inline float y2() { return std::stof(this->attr["y2"]); }

        inline float get_width() override;
        inline float get_height() override;
        inline float get_length();
        inline float get_slope();

        std::pair<float, float> along(float percent);

    protected:
        std::string tag() override { return "line"; }
    };

    class Rect : public Element {
    public:
        Rect() {};
        Rect(
            float x, float y, float width, float height) :
            Element({
                    { "x", std::to_string(x) },
                    { "y", std::to_string(y) },
                    { "width", std::to_string(width) },
                    { "height", std::to_string(height) }
            }) {};

        Element::BoundingBox get_bbox() override;
    protected:
        std::string tag() override { return "rect"; }
    };

    class Circle : public Element {
    public:
        Circle() {};

        Circle(float cx, float cy, float radius) :
                Element({
                        { "cx", std::to_string(cx) },
                        { "cy", std::to_string(cy) },
                        { "r", std::to_string(radius) }
                }) {
        };

        Circle(std::pair<float, float> xy, float radius) : Circle(xy.first, xy.second, radius) {};
        Element::BoundingBox get_bbox() override;
    protected:
        std::string tag() override { return "circle"; }
    };

    Element::BoundingBox Rect::get_bbox() {
        using std::stof;
        float x = stof(this->attr["x"]), y = stof(this->attr["y"]),
            width = stof(this->attr["width"]), height = stof(this->attr["height"]);

        return { x, x + width, y, y + height };
    }

    Element::BoundingBox Circle::get_bbox() {
        using std::stof;
        float x = stof(this->attr["cx"]), y = stof(this->attr["cy"]),
                radius = stof(this->attr["r"]);

        return {
            x - radius,
            x + radius,
            y - radius,
            y + radius
        };
    }

    float Line::get_slope() {
        return (y2() - y1()) / (x2() - x1());
    }

    float Line::get_length() {
        return std::sqrt(pow(get_width(), 2) + pow(get_height(), 2));
    }

    float Line::get_width() {
        return std::abs(x2() - x1());
    }

    float Line::get_height() {
        return std::abs(y2() - y1());
    }

    std::pair<float, float> Line::along(float percent) {
        /** Return the coordinates required to place an element along
         *   this line
         */

        float x_pos, y_pos;

        if (x1() != x2()) {
            float length = percent * this->get_length();
            float discrim = std::sqrt(4 * pow(length, 2) * (1 / (1 + pow(get_slope(), 2))));

            float x_a = (2 * x1() + discrim) / 2;
            float x_b = (2 * x1() - discrim) / 2;
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

# define to_string_attrib for (auto it = attr.begin(); it != attr.end(); ++it) \
    ret += " " + it->first + "=" + "\"" + it->second += "\""

    std::string Element::to_string(const size_t indent_level) {
        auto indent = std::string(indent_level, '\t');
        std::string ret = indent + "<" + tag();

        // Set attributes
        to_string_attrib;
        
        if (!this->children.empty()) {
            ret += ">\n";

            // Recursively get strings for child elements
            for (auto it = children.begin(); it != children.end(); ++it)
                ret += (*it)->to_string(indent_level + 1) + "\n";

            return ret += indent + "</" + tag() + ">";
        }

        return ret += " />";
    }

    std::string Text::to_string(const size_t indent_level) {
        auto indent = std::string(indent_level, '\t');
        std::string ret = indent + "<text";
        to_string_attrib;
        return ret += ">" + this->content + "</text>";
    }

    void Element::set_bbox() {
        /** Modify this element's attributes so it can hold all of its child elements */
        using std::to_string;
        using std::stof;

        Element::BoundingBox bbox = this->get_bbox();
        this->set_bbox(bbox); // Compute the bounding box (recursive)
        float width = abs(bbox.x1) + abs(bbox.x2),
            height = abs(bbox.y1) + abs(bbox.y2),
            x1 = bbox.x1, y1 = bbox.y1;

        this->set_attr("width", width)
                .set_attr("height", height);

        if (x1 < 0 || y1 < 0)
            this->set_attr("viewBox",
                           to_string(x1) + " " + // min-x
                           to_string(y1) + " " + // min-y
                           to_string(width) + " " +
                           to_string(height)
            );
    }

    void Element::set_bbox(Element::BoundingBox& box) {
        // Recursively compute a bounding box
        auto this_bbox = this->get_bbox();       
        if (isnan(box.x1) || this_bbox.x1 < box.x1) box.x1 = this_bbox.x1;
        if (isnan(box.x2) || this_bbox.x2 > box.x2) box.x2 = this_bbox.x2;
        if (isnan(box.y1) || this_bbox.y1 < box.y1) box.y1 = this_bbox.y1;
        if (isnan(box.y2) || this_bbox.y2 > box.y2) box.y2 = this_bbox.y2;

        // Recursion
        for (auto& child: this->children) child->set_bbox(box);
    }

    Element::ChildMap Element::get_children() {
        /** Recursively compute all of the children of an SVG element */
        Element::ChildMap child_map;
        this->get_children(child_map);
        return child_map;
    }

    void Element::get_children(Element::ChildMap& child_map) {
        // Helper function for get_children() which actually populates the map
        for (auto& child: this->children) {
            child_map[child->tag()].push_back(child.get());
            child->get_children(child_map); // Recursion
        }
    };
}