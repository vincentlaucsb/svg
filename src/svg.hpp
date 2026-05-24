/*
   ____ __     ______
  / ___|\ \   / / ___|
  \___ \ \ \ / / |  _
   ___) | \ V /| |_| |
  |____/   \_/  \____|

  SVG for C++ v0.3.0

  Copyright (c) 2018-2026 Vincent La
  SPDX-License-Identifier: MIT

  Permission is hereby granted, free of charge, to any person obtaining a copy
  of this software and associated documentation files (the "Software"), to deal
  in the Software without restriction, including without limitation the rights
  to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
  copies of the Software, and to permit persons to whom the Software is
  furnished to do so, subject to the following conditions:

  The above copyright notice and this permission notice shall be included in all
  copies or substantial portions of the Software.

  THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
  IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
  FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
  AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
  LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
  OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
  SOFTWARE.
*/

/** @file */
#pragma once
#define PI 3.14159265
#define RAD_TO_DEG (180/PI)
#define SVG_TYPE_CHECK static_assert(std::is_base_of<Element, T>::value, "Child must be an SVG element.")
#define APPROX_EQUALS(x, y, tol) bool(abs(x - y) < tol)
#include <iostream>
#include <algorithm> // min, max
#include <cctype>
#include <cmath>
#include <cstdint>
#include <fstream>   // ofstream
#include <functional>
#include <math.h>    // NAN
#include <map>
#include <deque>
#include <vector>
#include <string>
#include <sstream> // stringstream
#include <iomanip> // setprecision
#include <memory>
#include <stdexcept>
#include <type_traits> // is_base_of
#include <tuple>
#include <utility>

namespace SVG {
    /** @namespace SVG
     *  @brief Main namespace for SVG for C++
     */
    class AttributeMap;
    class SVG;
    class Shape;
    class Symbol;
    class Use;

    /** Stable element categories used for typed lookup without requiring RTTI. */
    enum class ElementKind {
        Custom,
        Defs,
        Symbol,
        Use,
        SVG,
        Style,
        Path,
        Text,
        Title,
        Group,
        Line,
        Rect,
        Circle,
        Polygon
    };

    /** Return the native SVG tag name for a built-in kind, or an empty string for Custom. */
    inline std::string tag_name(ElementKind kind) {
        switch (kind) {
            case ElementKind::Defs: return "defs";
            case ElementKind::Symbol: return "symbol";
            case ElementKind::Use: return "use";
            case ElementKind::SVG: return "svg";
            case ElementKind::Style: return "style";
            case ElementKind::Path: return "path";
            case ElementKind::Text: return "text";
            case ElementKind::Title: return "title";
            case ElementKind::Group: return "g";
            case ElementKind::Line: return "line";
            case ElementKind::Rect: return "rect";
            case ElementKind::Circle: return "circle";
            case ElementKind::Polygon: return "polygon";
            case ElementKind::Custom: return "";
        }
        return "";
    }

    struct QuadCoord {
        double x1;
        double x2;
        double y1;
        double y2;
    };

    /** A mapping of CSS selectors to their corresponding style attributes */
    using SelectorProperties = std::map<std::string, AttributeMap>;
    using SVGAttrib = std::map<std::string, std::string>;
    /** Explicit wrapper for trailing add_child() attributes. */
    struct Attrs : SVGAttrib {
        using SVGAttrib::SVGAttrib;
    };
    using Point = std::pair<double, double>;
    using Margins = QuadCoord;
    const static Margins DEFAULT_MARGINS = { 10, 10, 10, 10 };
    const static Margins NO_MARGINS = { 0, 0, 0, 0 };

    /** CSS color value stored as concrete sRGB channels for palette math. */
    class Color {
    public:
        /** Build an rgb() color from 8-bit channel values. */
        static Color rgb(int red, int green, int blue) {
            return Color(channel(red), channel(green), channel(blue));
        }

        /** Build a normalized hex color from 3 or 6 hexadecimal digits. */
        static Color hex(std::string value) {
            if (!value.empty() && value[0] == '#') {
                value.erase(value.begin());
            }
            if (value.size() != 3 && value.size() != 6) {
                throw std::invalid_argument("Hex colors must use 3 or 6 digits");
            }

            for (auto& ch : value) {
                if (!std::isxdigit(static_cast<unsigned char>(ch))) {
                    throw std::invalid_argument("Hex colors may only contain hexadecimal digits");
                }
                ch = static_cast<char>(std::tolower(static_cast<unsigned char>(ch)));
            }

            if (value.size() == 3) {
                value = std::string{ value[0], value[0],
                                     value[1], value[1],
                                     value[2], value[2] };
            }

            return Color(from_hex_pair(value, 0),
                         from_hex_pair(value, 2),
                         from_hex_pair(value, 4));
        }

        /** Build an sRGB color from hue degrees and saturation/lightness percentages. */
        static Color hsl(double hue, double saturation, double lightness) {
            validate_percentage(saturation, "Saturation");
            validate_percentage(lightness, "Lightness");

            hue = std::fmod(hue, 360.0);
            if (hue < 0) {
                hue += 360.0;
            }

            const auto s = saturation / 100.0;
            const auto l = lightness / 100.0;
            const auto c = (1.0 - std::fabs(2.0 * l - 1.0)) * s;
            const auto x = c * (1.0 - std::fabs(std::fmod(hue / 60.0, 2.0) - 1.0));
            const auto m = l - c / 2.0;
            double red = 0;
            double green = 0;
            double blue = 0;

            if (hue < 60) {
                red = c;
                green = x;
            } else if (hue < 120) {
                red = x;
                green = c;
            } else if (hue < 180) {
                green = c;
                blue = x;
            } else if (hue < 240) {
                green = x;
                blue = c;
            } else if (hue < 300) {
                red = x;
                blue = c;
            } else {
                red = c;
                blue = x;
            }

            return Color(channel_from_unit(red + m),
                         channel_from_unit(green + m),
                         channel_from_unit(blue + m));
        }

        /** Mix this color toward another color by a 0.0-1.0 amount. */
        Color mix(const Color& other, double amount) const {
            validate_unit(amount, "Mix amount");
            return Color(mix_channel(this->red_, other.red_, amount),
                         mix_channel(this->green_, other.green_, amount),
                         mix_channel(this->blue_, other.blue_, amount));
        }

        /** Mix this color toward white. */
        Color tint(double amount) const {
            return mix(white(), amount);
        }

        /** Mix this color toward black. */
        Color shade(double amount) const {
            return mix(black(), amount);
        }

        /** Common palette anchor for tinting and mixing. */
        static Color white() {
            return rgb(255, 255, 255);
        }

        /** Common palette anchor for shading and mixing. */
        static Color black() {
            return rgb(0, 0, 0);
        }

        /** Allow Color to be used anywhere an attribute accepts std::string. */
        operator std::string() const {
            return serialize();
        }

    private:
        Color(uint8_t red, uint8_t green, uint8_t blue) : red_(red), green_(green), blue_(blue) {}

        std::string serialize() const {
            std::ostringstream ss;
            ss << '#'
               << std::hex << std::setfill('0') << std::nouppercase
               << std::setw(2) << static_cast<int>(red_)
               << std::setw(2) << static_cast<int>(green_)
               << std::setw(2) << static_cast<int>(blue_);
            return ss.str();
        }

        static uint8_t channel(int value) {
            if (value < 0 || value > 255) {
                throw std::invalid_argument("RGB channels must be between 0 and 255");
            }
            return static_cast<uint8_t>(value);
        }

        static uint8_t channel_from_unit(double value) {
            value = std::max(0.0, std::min(1.0, value));
            return channel(static_cast<int>(std::round(value * 255.0)));
        }

        static uint8_t from_hex_pair(const std::string& value, size_t offset) {
            return static_cast<uint8_t>(std::stoi(value.substr(offset, 2), nullptr, 16));
        }

        static uint8_t mix_channel(uint8_t from, uint8_t to, double amount) {
            return channel(static_cast<int>(std::round(from + (to - from) * amount)));
        }

        static void validate_percentage(double value, const std::string& label) {
            if (value < 0 || value > 100) {
                throw std::invalid_argument(label + " must be between 0 and 100");
            }
        }

        static void validate_unit(double value, const std::string& label) {
            if (value < 0 || value > 1) {
                throw std::invalid_argument(label + " must be between 0 and 1");
            }
        }

        uint8_t red_;
        uint8_t green_;
        uint8_t blue_;
    };

#if __cplusplus < 201402L
    namespace detail {
        template<typename T, typename... Args>
        std::unique_ptr<T> make_unique(Args&&... args) {
            return std::unique_ptr<T>(new T(std::forward<Args>(args)...));
        }
    }
#else
    namespace detail {
        using std::make_unique;
    }
#endif

    /** @cond */
    namespace detail {
        template<size_t... I>
        struct index_sequence {};

        template<size_t N, size_t... I>
        struct make_index_sequence : make_index_sequence<N - 1, N - 1, I...> {};

        template<size_t... I>
        struct make_index_sequence<0, I...> {
            using type = index_sequence<I...>;
        };

        template<typename T>
        struct is_attrs : std::is_same<typename std::decay<T>::type, Attrs> {};

        template<typename... Args>
        struct last_is_attrs : std::false_type {};

        template<typename T>
        struct last_is_attrs<T> : is_attrs<T> {};

        template<typename T, typename... Rest>
        struct last_is_attrs<T, Rest...> : last_is_attrs<Rest...> {};

        template<typename T, typename Tuple, size_t... I>
        std::unique_ptr<T> make_child_with_attrs(Tuple&& tuple, index_sequence<I...>) {
            const auto attrs = std::get<sizeof...(I)>(tuple);
            auto child = detail::make_unique<T>(std::get<I>(std::move(tuple))...);
            for (const auto& attr : attrs) {
                child->set_attr(attr.first, attr.second);
            }
            return child;
        }

        template<typename T, typename... Args>
        std::unique_ptr<T> make_child_impl(std::false_type, Args&&... args) {
            return detail::make_unique<T>(std::forward<Args>(args)...);
        }

        template<typename T, typename... Args>
        std::unique_ptr<T> make_child_impl(std::true_type, Args&&... args) {
            auto tuple = std::forward_as_tuple(std::forward<Args>(args)...);
            return make_child_with_attrs<T>(
                std::move(tuple),
                typename make_index_sequence<sizeof...(Args) - 1>::type{});
        }

        template<typename T, typename... Args>
        std::unique_ptr<T> make_child(Args&&... args) {
            return make_child_impl<T>(last_is_attrs<Args...>{}, std::forward<Args>(args)...);
        }

        struct AffineTransform {
            AffineTransform() = default;
            AffineTransform(double a_, double b_, double c_, double d_, double e_, double f_) :
                    a(a_), b(b_), c(c_), d(d_), e(e_), f(f_) {}

            double a = 1;
            double b = 0;
            double c = 0;
            double d = 1;
            double e = 0;
            double f = 0;

            Point apply(Point point) const {
                return {
                    a * point.first + c * point.second + e,
                    b * point.first + d * point.second + f
                };
            }
        };

        inline AffineTransform multiply(const AffineTransform& left, const AffineTransform& right) {
            return {
                left.a * right.a + left.c * right.b,
                left.b * right.a + left.d * right.b,
                left.a * right.c + left.c * right.d,
                left.b * right.c + left.d * right.d,
                left.a * right.e + left.c * right.f + left.e,
                left.b * right.e + left.d * right.f + left.f
            };
        }

        inline AffineTransform rotate_transform(double degrees, double cx = 0, double cy = 0) {
            const auto radians = degrees * PI / 180.0;
            const auto cos_value = std::cos(radians);
            const auto sin_value = std::sin(radians);
            return {
                cos_value,
                sin_value,
                -sin_value,
                cos_value,
                cx - cos_value * cx + sin_value * cy,
                cy - sin_value * cx - cos_value * cy
            };
        }

        inline std::vector<double> parse_transform_args(std::string args) {
            for (auto& ch : args) {
                if (ch == ',') {
                    ch = ' ';
                }
            }

            std::istringstream stream(args);
            std::vector<double> values;
            double value;
            while (stream >> value) {
                values.push_back(value);
            }
            return values;
        }

        inline AffineTransform parse_supported_transform(const std::string& transform) {
            AffineTransform current;
            size_t pos = 0;
            while (pos < transform.size()) {
                while (pos < transform.size() && std::isspace(static_cast<unsigned char>(transform[pos]))) {
                    ++pos;
                }
                const auto name_start = pos;
                while (pos < transform.size() && std::isalpha(static_cast<unsigned char>(transform[pos]))) {
                    ++pos;
                }
                if (name_start == pos) {
                    break;
                }

                const auto name = transform.substr(name_start, pos - name_start);
                while (pos < transform.size() && std::isspace(static_cast<unsigned char>(transform[pos]))) {
                    ++pos;
                }
                if (pos >= transform.size() || transform[pos] != '(') {
                    break;
                }
                const auto args_start = ++pos;
                const auto args_end = transform.find(')', args_start);
                if (args_end == std::string::npos) {
                    break;
                }
                pos = args_end + 1;

                const auto args = parse_transform_args(transform.substr(args_start, args_end - args_start));
                if (name == "rotate" && (args.size() == 1 || args.size() == 3)) {
                    current = multiply(
                        current,
                        args.size() == 1 ? rotate_transform(args[0])
                                         : rotate_transform(args[0], args[1], args[2]));
                }
            }
            return current;
        }
    }
    /** @endcond */

    inline std::string to_string(const double& value);
    inline std::string to_string(const Point& point);
    /** Return the serialized CSS color token. */
    inline std::string to_string(const Color& color);
    inline std::string to_string(const std::map<std::string, AttributeMap>& css, const size_t indent_level=0);
    /** Escape text for XML element content or attribute values. */
    inline std::string escape_xml(const std::string& text);

    std::vector<Point> bounding_polygon(const std::vector<Shape*>& shapes);
    SVG frame_animate(std::vector<SVG>& frames, const double fps);
    SVG merge(SVG& left, SVG& right, const Margins& margins = DEFAULT_MARGINS);
    SVG merge(std::vector<SVG>& frames, const double width, const int max_frame_width);

    /** @class ClassList
     *  @brief Ordered token list for managing the class attribute
     */
    class ClassList {
    public:
        ClassList() = default;
        explicit ClassList(std::string& value) : mutable_value_(&value), value_(&value) {}
        explicit ClassList(const std::string& value) : value_(&value) {}

        /** Return true when the class token exists */
        bool contains(const std::string& token) const {
            validate_token(token);
            const auto values = tokens();
            return std::find(values.begin(), values.end(), token) != values.end();
        }

        /** Add a class token if it does not already exist */
        ClassList& add(const std::string& token) {
            validate_token(token);
            auto values = tokens();
            if (std::find(values.begin(), values.end(), token) == values.end()) {
                values.push_back(token);
                write(values);
            }
            return *this;
        }

        /** Remove a class token if it exists */
        ClassList& remove(const std::string& token) {
            validate_token(token);
            auto values = tokens();
            const auto original_size = values.size();
            values.erase(std::remove(values.begin(), values.end(), token), values.end());
            if (values.size() != original_size) {
                write(values);
            }
            return *this;
        }

        /** Add a missing token or remove an existing token, returning true when present */
        bool toggle(const std::string& token) {
            if (contains(token)) {
                remove(token);
                return false;
            }

            add(token);
            return true;
        }

        /** Replace the class attribute with a whitespace-normalized token list */
        ClassList& set(const std::string& class_names) {
            write(parse(class_names));
            return *this;
        }

        /** Remove all classes */
        ClassList& clear() {
            write({});
            return *this;
        }

        /** Return normalized class text */
        std::string str() const {
            return join(tokens());
        }

        /** Return the current tokens in order */
        std::vector<std::string> tokens() const {
            return parse(value());
        }

    private:
        static bool is_space(char ch) {
            return std::isspace(static_cast<unsigned char>(ch)) != 0;
        }

        static void validate_token(const std::string& token) {
            if (token.empty()) {
                throw std::invalid_argument("class token cannot be empty");
            }
            if (std::find_if(token.begin(), token.end(), is_space) != token.end()) {
                throw std::invalid_argument("class token cannot contain whitespace");
            }
        }

        static std::vector<std::string> parse(const std::string& class_names) {
            std::vector<std::string> result;
            std::string token;
            for (const auto ch : class_names) {
                if (is_space(ch)) {
                    if (!token.empty()) {
                        if (std::find(result.begin(), result.end(), token) == result.end()) {
                            result.push_back(token);
                        }
                        token.clear();
                    }
                    continue;
                }
                token.push_back(ch);
            }
            if (!token.empty() && std::find(result.begin(), result.end(), token) == result.end()) {
                result.push_back(token);
            }
            return result;
        }

        static std::string join(const std::vector<std::string>& values) {
            std::string result;
            for (std::size_t i = 0; i < values.size(); ++i) {
                if (i > 0) {
                    result += ' ';
                }
                result += values[i];
            }
            return result;
        }

        const std::string& value() const {
            static const std::string empty;
            return value_ ? *value_ : empty;
        }

        void write(const std::vector<std::string>& values) {
            if (!mutable_value_) {
                throw std::logic_error("cannot mutate a const class list");
            }
            *mutable_value_ = join(values);
            value_ = mutable_value_;
        }

        std::string* mutable_value_ = nullptr;
        const std::string* value_ = nullptr;
    };

    /** @class TransformList
     *  @brief Ordered function list for managing the transform attribute
     */
    class TransformList {
    public:
        TransformList() = default;
        explicit TransformList(std::string& value) : mutable_value_(&value), value_(&value) {}
        explicit TransformList(const std::string& value) : value_(&value) {}

        /** Append a raw transform function or function list */
        TransformList& append(const std::string& transform) {
            validate_appendable();
            if (transform.empty()) {
                throw std::invalid_argument("transform cannot be empty");
            }

            if (str().empty() || str() == "none") {
                write(transform);
            } else {
                write(str() + " " + transform);
            }
            return *this;
        }

        /** Replace the transform attribute */
        TransformList& set(const std::string& transform) {
            write(transform);
            return *this;
        }

        /** Remove all transforms */
        TransformList& clear() {
            write("");
            return *this;
        }

        TransformList& matrix(double a, double b, double c, double d, double e, double f) {
            std::stringstream ss;
            ss << "matrix(" << to_string(a) << " " << to_string(b) << " " << to_string(c)
               << " " << to_string(d) << " " << to_string(e) << " " << to_string(f) << ")";
            return append(ss.str());
        }

        TransformList& translate(double x) {
            return append("translate(" + to_string(x) + ")");
        }

        TransformList& translate(double x, double y) {
            return append("translate(" + to_string(x) + " " + to_string(y) + ")");
        }

        TransformList& scale(double factor) {
            return append("scale(" + to_string(factor) + ")");
        }

        TransformList& scale(double x, double y) {
            return append("scale(" + to_string(x) + " " + to_string(y) + ")");
        }

        TransformList& rotate(double degrees) {
            return append("rotate(" + to_string(degrees) + ")");
        }

        TransformList& rotate(double degrees, double cx, double cy) {
            return append("rotate(" + to_string(degrees) + " " + to_string(cx) + " " +
                          to_string(cy) + ")");
        }

        TransformList& skew_x(double degrees) {
            return append("skewX(" + to_string(degrees) + ")");
        }

        TransformList& skew_y(double degrees) {
            return append("skewY(" + to_string(degrees) + ")");
        }

        /** Return the current transform text */
        std::string str() const {
            return value();
        }

    private:
        static bool is_keyword(const std::string& transform) {
            return transform == "inherit" || transform == "initial" || transform == "revert" ||
                   transform == "revert-layer" || transform == "unset";
        }

        const std::string& value() const {
            static const std::string empty;
            return value_ ? *value_ : empty;
        }

        void validate_appendable() const {
            if (is_keyword(value())) {
                throw std::logic_error("cannot append transform functions to a transform keyword");
            }
        }

        void write(const std::string& value) {
            if (!mutable_value_) {
                throw std::logic_error("cannot mutate a const transform list");
            }
            *mutable_value_ = value;
            value_ = mutable_value_;
        }

        std::string* mutable_value_ = nullptr;
        const std::string* value_ = nullptr;
    };

    /** @namespace util
     *  @brief Various utility and mathematical functions
     */
    namespace util {
        enum Orientation {
            COLINEAR, CLOCKWISE, COUNTERCLOCKWISE
        };

        inline std::vector<Point> polar_points(int n, int a, int b, double radius);
        
        template<typename T>
        inline T min_or_not_nan(T first, T second) {
            /** Return the smallest number or the number that is not NAN
             *  Returns NAN if both are NAN
             */
            if (isnan(first) && isnan(second))
                return NAN;
            else if (isnan(first) || isnan(second))
                return isnan(first) ? second : first;
            else
                return std::min(first, second);
        }

        template<typename T>
        inline T max_or_not_nan(T first, T second) {
            /** Return the largest number or the number that is not NAN
            *  Returns NAN if both are NAN
            */
            if (isnan(first) && isnan(second))
                return NAN;
            else if (isnan(first) || isnan(second))
                return isnan(first) ? second : first;
            else
                return std::max(first, second);
        }

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

        inline std::vector<Point> polar_points(int n, int a, int b, double radius) {
            /** Return n equidistant points (oriented counterclockwise) located on
             *  the perimeter of a circle of radius r centered at (a, b)  
             *
             *  Note: Drawing an edge between each consecutive pair of points creates
             *  a convex polygon
             */
            std::vector<Point> ret;
            for (double degree = 0; degree < 360; degree += 360/n) {
                ret.push_back(Point(
                    a + radius * cos(degree * (PI/180)), // 1 degree = pi/180 radians
                    b + radius * sin(degree * (PI/180))
                ));
            }

            return ret;
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
        /** Return a string representation of a point as "x,y" */
        return to_string(point.first) + "," + to_string(point.second);
    }

    inline std::string to_string(const Color& color) {
        /** Return the serialized CSS color token. */
        return static_cast<std::string>(color);
    }

    inline std::string escape_xml(const std::string& text) {
        std::string out;
        out.reserve(text.size());
        for (const char ch : text) {
            switch (ch) {
                case '&':
                    out += "&amp;";
                    break;
                case '<':
                    out += "&lt;";
                    break;
                case '>':
                    out += "&gt;";
                    break;
                case '"':
                    out += "&quot;";
                    break;
                case '\'':
                    out += "&apos;";
                    break;
                default:
                    out.push_back(ch);
                    break;
            }
        }
        return out;
    }

    /** @class AttributeMap
     *  @brief Base class for anything that has attributes (e.g. SVG elements, CSS stylesheets)
     */
    class AttributeMap {
    public:
        struct AttrSetter {
            AttrSetter(SVGAttrib::mapped_type& _attr,
                       bool _normalize_class = false,
                       std::function<void(const std::string&)> _on_update = {}) :
                    attr_(_attr), normalize_class_(_normalize_class), on_update_(_on_update) {};

            /** Append a serialized color token to the attribute value. */
            AttrSetter& operator<<(const Color& value) {
                attr_ += static_cast<std::string>(value);
                normalize();
                notify();
                return *this;
            }

            template<typename T>
            AttrSetter& operator<<(T value) {
                attr_ += std::to_string(value);
                normalize();
                notify();
                return *this;
            }

        private:
            void normalize() {
                if (normalize_class_) {
                    ClassList(attr_).set(attr_);
                }
            }

            void notify() {
                if (on_update_) {
                    on_update_(attr_);
                }
            }

            SVGAttrib::mapped_type& attr_;
            bool normalize_class_ = false;
            std::function<void(const std::string&)> on_update_;
        };

        AttributeMap() = default;
        virtual ~AttributeMap() = default;
        AttributeMap(SVGAttrib _attr) : attr_(std::move(_attr)) {};

        const SVGAttrib& attrs() const {
            return this->attr_;
        }

        bool has_attr(const std::string& key) const {
            return this->attr_.find(key) != this->attr_.end();
        }

        std::string get_attr(const std::string& key, const std::string& fallback = "") const {
            const auto found = this->attr_.find(key);
            return found == this->attr_.end() ? fallback : found->second;
        }

        template<typename T>
        AttributeMap& set_attr(const std::string key, T value) {
            this->set_attr_value(key, std::to_string(value));
            return *this;
        }

        /** Set an attribute to a serialized color token. */
        AttributeMap& set_attr(const std::string key, const Color& value);

        /** Set multiple attributes at once */
        AttributeMap& set_attrs(std::initializer_list<std::pair<std::string, std::string>> values) {
            for (const auto& pair : values) {
                this->set_attr_value(pair.first, pair.second);
            }

            return *this;
        }

        /** Set multiple attributes from an existing attribute map. */
        AttributeMap& set_attrs(const SVGAttrib& values) {
            for (const auto& pair : values) {
                this->set_attr_value(pair.first, pair.second);
            }

            return *this;
        }

        AttrSetter set_attr(const std::string key) {
            return this->make_attr_setter(key);
        };

        ClassList class_list() {
            return ClassList(this->attr_["class"]);
        }

        ClassList class_list() const {
            const auto found = this->attr_.find("class");
            return found == this->attr_.end() ? ClassList() : ClassList(found->second);
        }

        TransformList transform_list() {
            return TransformList(this->attr_["transform"]);
        }

        TransformList transform_list() const {
            const auto found = this->attr_.find("transform");
            return found == this->attr_.end() ? TransformList() : TransformList(found->second);
        }

        TransformList transform() {
            return transform_list();
        }

        TransformList transform() const {
            return transform_list();
        }

    protected:
        virtual void set_attr_value(const std::string& key, const std::string& value) {
            if (key == "class") {
                ClassList(this->attr_[key]).set(value);
                return;
            }
            this->attr_[key] = value;
        }

        virtual AttrSetter make_attr_setter(const std::string& key) {
            if (this->attr_.find(key) == this->attr_.end()) this->attr_[key] = "";
            return AttrSetter(this->attr_.at(key), key == "class");
        }

        SVGAttrib& mutable_attrs() {
            return this->attr_;
        }

    private:
        SVGAttrib attr_;
    };

    template<>
    inline AttributeMap::AttrSetter& AttributeMap::AttrSetter::operator<<(const char * value) {
        attr_ += value;
        normalize();
        notify();
        return *this;
    }

    template<>
    inline AttributeMap::AttrSetter& AttributeMap::AttrSetter::operator<<(const std::string value) {
        attr_ += value;
        normalize();
        notify();
        return *this;
    }

    template<>
    inline AttributeMap& AttributeMap::set_attr(const std::string key, const double value) {
        /** Modify the attribute specified by key */
        this->set_attr_value(key, to_string(value));
        return *this;
    }

    inline AttributeMap& AttributeMap::set_attr(const std::string key, const Color& value) {
        this->set_attr_value(key, static_cast<std::string>(value));
        return *this;
    }

    template<>
    inline AttributeMap& AttributeMap::set_attr(const std::string key, const char * value) {
        /** Modify the attribute specified by key */
        this->set_attr_value(key, value);
        return *this;
    }

    template<>
    inline AttributeMap& AttributeMap::set_attr(const std::string key, const std::string value) {
        /** Modify the attribute specified by key */
        this->set_attr_value(key, value);
        return *this;
    }

    /** Shared enum-to-string map for typed CSS helpers. */
    template<typename T>
    class TypedNames {
        static_assert(std::is_enum<T>::value, "Typed CSS keys must be an enum type.");

    public:
        /** Return the canonical CSS name for a typed key. */
        std::string name(T key) const {
            const auto found = this->names_.find(key);
            if (found == this->names_.end()) {
                throw std::invalid_argument("Unknown typed CSS key");
            }
            return found->second;
        }

    protected:
        /** Add one validated typed key/name pair. */
        void add_name(T key, const std::string& normalized_name, const std::string& duplicate_label) {
            if (this->names_.find(key) != this->names_.end()) {
                throw std::invalid_argument("Duplicate typed CSS key");
            }
            if (this->used_names_.find(normalized_name) != this->used_names_.end()) {
                throw std::invalid_argument("Duplicate " + duplicate_label + ": " + normalized_name);
            }

            this->names_[key] = normalized_name;
            this->used_names_[normalized_name] = key;
        }

    private:
        /** Enum-to-name mapping for this helper. */
        std::map<T, std::string> names_;
        /** Reverse map used to reject duplicate output names. */
        std::map<std::string, T> used_names_;
    };

    /** Mapping entry for a typed CSS custom property, optionally with an initial value. */
    template<typename T>
    struct VariableSpec {
        /** Typed variable key. */
        T key;
        /** CSS custom property name, normalized to include a leading "--" by Variables. */
        std::string css_name;
        /** True when this spec also provides an initial value. */
        bool has_value = false;
        /** Initial CSS custom property value. */
        std::string value;

        /** Register a typed key and CSS custom property name without setting a value. */
        VariableSpec(T _key, std::string _css_name) :
                key(_key), css_name(std::move(_css_name)) {}

        /** Register a typed key and CSS custom property name, then set its initial value. */
        VariableSpec(T _key, std::string _css_name, std::string _value) :
                key(_key),
                css_name(std::move(_css_name)),
                has_value(true),
                value(std::move(_value)) {}
    };

    /** Mapping entry for a typed CSS class token. */
    template<typename T>
    struct ClassSpec {
        /** Typed class key. */
        T key;
        /** CSS class token, normalized by Classes to omit any leading ".". */
        std::string css_class;

        /** Register a typed key and CSS class token. */
        ClassSpec(T _key, std::string _css_class) :
                key(_key), css_class(std::move(_css_class)) {}
    };

    /** Typed helper for defining and referencing CSS custom properties without stringly lookups. */
    template<typename T>
    class Variables : public TypedNames<T> {
    public:
        /** Validate the enum-to-name map and apply any initial values to the target style block. */
        Variables(AttributeMap& target, std::initializer_list<VariableSpec<T>> specs) :
                target_(&target) {
            std::vector<std::pair<T, std::string>> initial_values;

            for (const auto& spec : specs) {
                const auto normalized_name = normalize_name(spec.css_name);
                this->add_name(spec.key, normalized_name, "CSS variable name");
                if (spec.has_value) {
                    initial_values.push_back({ spec.key, spec.value });
                }
            }

            for (const auto& initial : initial_values) {
                this->set(initial.first, initial.second);
            }
        }

        /** Return a CSS var() reference for a typed key. */
        std::string var(T key) const {
            return "var(" + this->name(key) + ")";
        }

        /** Set a CSS custom property value in the bound style block. */
        Variables& set(T key, const std::string& value) {
            this->target_->set_attr(this->name(key), value);
            return *this;
        }

        /** Set a CSS custom property value from a string literal. */
        Variables& set(T key, const char* value) {
            return this->set(key, std::string(value));
        }

        /** Replace numbered placeholders like "{0}" with typed var() references. */
        template<typename... Keys>
        std::string format(const std::string& pattern, Keys... keys) const {
            std::vector<std::string> args = { this->var(keys)... };
            std::vector<bool> used(args.size(), false);
            std::string out;

            for (size_t i = 0; i < pattern.size(); ++i) {
                if (pattern[i] == '{') {
                    if (i + 1 < pattern.size() && pattern[i + 1] == '{') {
                        out.push_back('{');
                        ++i;
                        continue;
                    }

                    const auto start = i + 1;
                    auto pos = start;
                    size_t index = 0;
                    while (pos < pattern.size() && std::isdigit(static_cast<unsigned char>(pattern[pos]))) {
                        index = index * 10 + static_cast<size_t>(pattern[pos] - '0');
                        ++pos;
                    }
                    if (pos == start || pos >= pattern.size() || pattern[pos] != '}') {
                        throw std::invalid_argument("Malformed CSS variable format placeholder");
                    }
                    if (index >= args.size()) {
                        throw std::invalid_argument("CSS variable format placeholder index out of range");
                    }

                    out += args[index];
                    used[index] = true;
                    i = pos;
                } else if (pattern[i] == '}') {
                    if (i + 1 < pattern.size() && pattern[i + 1] == '}') {
                        out.push_back('}');
                        ++i;
                        continue;
                    }
                    throw std::invalid_argument("Unmatched CSS variable format brace");
                } else {
                    out.push_back(pattern[i]);
                }
            }

            for (const auto was_used : used) {
                if (!was_used) {
                    throw std::invalid_argument("Unused CSS variable format argument");
                }
            }

            return out;
        }

    private:
        /** Return a valid CSS custom property name, accepting user input with or without "--". */
        static std::string normalize_name(const std::string& css_name) {
            if (css_name.empty()) {
                throw std::invalid_argument("CSS variable name cannot be empty");
            }
            if (css_name.size() >= 2 && css_name[0] == '-' && css_name[1] == '-') {
                return css_name;
            }
            return "--" + css_name;
        }

        /** Style block that receives set() writes. */
        AttributeMap* target_;
    };

    /** Typed helper for building class attributes and class selectors without stringly lookups. */
    template<typename T>
    class Classes : public TypedNames<T> {
    public:
        /** Validate and register typed CSS class tokens. */
        Classes(std::initializer_list<ClassSpec<T>> specs) {
            for (const auto& spec : specs) {
                this->add_name(spec.key, normalize_name(spec.css_class), "CSS class name");
            }
        }

        /** Return a CSS selector for one class or a combined selector for multiple classes. */
        template<typename... Keys>
        std::string selector(Keys... keys) const {
            const auto names = this->names(keys...);
            std::string out;
            for (const auto& name : names) {
                out += "." + name;
            }
            return out;
        }

        /** Return a space-separated class attribute value. */
        template<typename... Keys>
        std::string classes(Keys... keys) const {
            const auto names = this->names(keys...);
            std::string out;
            for (const auto& name : names) {
                if (!out.empty()) out += " ";
                out += name;
            }
            return out;
        }

    private:
        static std::string normalize_name(std::string css_class) {
            if (!css_class.empty() && css_class[0] == '.') {
                css_class.erase(0, 1);
            }
            if (css_class.empty()) {
                throw std::invalid_argument("CSS class name cannot be empty");
            }
            for (const auto ch : css_class) {
                if (std::isspace(static_cast<unsigned char>(ch))) {
                    throw std::invalid_argument("CSS class name cannot contain whitespace");
                }
            }
            return css_class;
        }

        template<typename... Keys>
        std::vector<std::string> names(Keys... keys) const {
            return std::vector<std::string>{ this->name(keys)... };
        }
    };

    /** @class Element
     *  @brief Abstract base class for all SVG elements
     */
    class Element: public AttributeMap {
    public:
        /** @class BoundingBox
         *  @brief Represents the top left and bottom right corners of a bounding rectangle
         */
        class BoundingBox : public QuadCoord {
        public:
            using QuadCoord::QuadCoord;
            BoundingBox() = default;
            BoundingBox(double a, double b, double c, double d) : QuadCoord({ a, b, c, d }) {};

            BoundingBox operator+ (const BoundingBox& other) {
                /** Return a new bounding box which envelopes both original boxes */
                using namespace util;
                BoundingBox new_box;
                new_box.x1 = min_or_not_nan(this->x1, other.x1);
                new_box.x2 = max_or_not_nan(this->x2, other.x2);
                new_box.y1 = min_or_not_nan(this->y1, other.y1);
                new_box.y2 = max_or_not_nan(this->y2, other.y2);
                return new_box;
            }
        };
        using ChildList = std::vector<Element*>;
        using ChildMap = std::map<std::string, ChildList>;

        Element() = default;
        virtual ~Element() = default;
        Element(const Element& other) = delete; // No copy constructor
        Element(Element&& other) noexcept :
                AttributeMap(std::move(other)),
                children(std::move(other.children)),
                parent_(nullptr),
                owner_(nullptr),
                indexed_id_() {
            reparent_children();
        }
        Element& operator=(const Element&) = delete; // No copy assignment
        Element& operator=(Element&& other) noexcept {
            if (this != &other) {
                AttributeMap::operator=(std::move(other));
                children = std::move(other.children);
                parent_ = nullptr;
                owner_ = nullptr;
                indexed_id_.clear();
                reparent_children();
            }
            return *this;
        }

        Element(const char* id) : AttributeMap(
            SVGAttrib({ { "id", id } })) {};
        using AttributeMap::AttributeMap;

        // Implicit string conversion
        operator std::string() { return this->svg_to_string(0); };

        template<typename T, typename... Args>
        T* add_child(Args&&... args) {
            /** Add an SVG element as a child and return a pointer to the element added */
            SVG_TYPE_CHECK;
            auto child = detail::make_child<T>(std::forward<Args>(args)...);
            return static_cast<T*>(this->insert_child(std::move(child), this->children.end()));
        }

        template<typename T>
        Element& operator<<(T&& node) {
            /** Move an SVG element into this container */
            SVG_TYPE_CHECK;
            auto child = detail::make_unique<T>(std::move(node));
            this->insert_child(std::move(child), this->children.end());
            return *this;
        }

        template<typename T>
        std::vector<T*> get_children() {
            /** Return all children of type T */
            SVG_TYPE_CHECK;
            std::vector<T*> ret;
            auto child_elems = this->get_children_helper();
            
            for (auto& child: child_elems)
                if (child->kind() == T::static_kind) ret.push_back(static_cast<T*>(child));

            return ret;
        }

        template<typename T>
        std::vector<T*> get_immediate_children() {
            /** Return all immediate children of type T */
            SVG_TYPE_CHECK;
            std::vector<T*> ret;
            for (auto& child : this->children) {
                if (child->kind() == T::static_kind) ret.push_back(static_cast<T*>(child.get()));
            }

            return ret;
        }

        Element* get_element_by_id(const std::string& id);
        std::vector<Element*> get_elements_by_class(const std::string& clsname);
        const Element* parent() const { return parent_; }
        /** Return the element category used by typed traversal; custom subclasses default to Custom. */
        virtual ElementKind kind() const { return ElementKind::Custom; }
        Element& id(const std::string& value);
        std::string id() const;
        void autoscale(const Margins& margins=DEFAULT_MARGINS);
        void autoscale(const double margin);
        virtual BoundingBox get_bbox();
        ChildMap get_children();

    protected:
        std::vector<std::unique_ptr<Element>> children; /** Smart pointers to child elements */
        using ChildIterator = std::vector<std::unique_ptr<Element>>::iterator;
        std::vector<Element*> get_children_helper();
        void get_bbox(Element::BoundingBox&);
        void get_bbox(Element::BoundingBox&, const detail::AffineTransform& parent_transform);
        virtual std::string svg_to_string(const size_t indent_level); /** SVG string corresponding to this element */
        virtual std::string tag() { return tag_name(this->kind()); } /** The SVG tag of this element */

        void set_attr_value(const std::string& key, const std::string& value) override;
        AttrSetter make_attr_setter(const std::string& key) override;
        SVG* owner_svg();
        const SVG* owner_svg() const;
        void set_owner_svg(SVG* owner);
        void register_subtree_ids();
        void unregister_subtree_ids();
        void register_own_id();
        void unregister_own_id();

        Element* insert_child(std::unique_ptr<Element> child, ChildIterator position) {
            child->parent_ = this;
            child->set_owner_svg(this->owner_svg());
            try {
                child->register_subtree_ids();
            } catch (...) {
                child->unregister_subtree_ids();
                child->set_owner_svg(nullptr);
                child->parent_ = nullptr;
                throw;
            }
            return children.insert(position, std::move(child))->get();
        }

        void reparent_children() {
            for (auto& child : children) {
                child->parent_ = this;
                child->set_owner_svg(this->owner_);
                child->reparent_children();
            }
        }

        double find_numeric(const std::string& key) {
            /** Return the numeric attribute (if it exists) or NAN
             *
             *  @param[in] key Name of the attribute
             */
            if (this->has_attr(key))
                return std::stof(this->get_attr(key));
            return NAN;
        }

    private:
        Element* parent_ = nullptr;
        SVG* owner_ = nullptr;
        std::string indexed_id_;
    };

    template<>
    inline Element::ChildList Element::get_immediate_children() {
        /** Return all immediate children, regardless of type, as Element pointers */
        Element::ChildList ret;
        for (auto& child : this->children) ret.push_back(child.get());
        return ret;
    }

    inline Element* Element::get_element_by_id(const std::string &id) {
        /** Return the SVG element that has a certain id */
        auto child_elems = this->get_children_helper();
        for (auto& current: child_elems)
            if (current->id() == id) return current;
        
        return nullptr;
    }

    inline std::vector<Element*> Element::get_elements_by_class(const std::string &clsname) {
        /** Return all SVG elements with a certain class name */
        std::vector<Element*> ret;
        auto child_elems = this->get_children_helper();
    
        for (auto& current: child_elems) {
            if (current->has_attr("class")
                && current->class_list().contains(clsname))
                ret.push_back(current);
        }
    
        return ret;
    }

    inline Element::BoundingBox Element::get_bbox() {
        /** Compute the bounding box necessary to contain this element */
        return { NAN, NAN, NAN, NAN };
    }

    /** @class Shape
     *  @brief Base class for any SVG elements that have a width and height
     */
    class Shape: public Element {
    public:
        using Element::Element;

        operator Point() {
            /** Implicit conversion to Point */
            return std::make_pair(this->x(), this->y());
        }

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
        virtual double width() {
            /** Return this item's width, either by calculating it or finding the 
             *  width attribute
             */
            return this->find_numeric("width");
        }
        virtual double height() {
            /** Return this item's height, either by calculating it or finding the
             *  height attribute
             */
            return this->find_numeric("height");
        }
    };

    /** Container for reusable SVG definitions such as symbols. */
    class Defs : public Element {
    public:
        static constexpr ElementKind static_kind = ElementKind::Defs;
        using Element::Element;
        ElementKind kind() const override { return static_kind; }
        /** Return an existing symbol with this id, or create one when absent. */
        Symbol* symbol(std::string id);
    };

    /** Reusable element definition that can be instantiated with Use. */
    class Symbol : public Element {
    public:
        static constexpr ElementKind static_kind = ElementKind::Symbol;
        Symbol() = default;
        using Element::Element;

        explicit Symbol(std::string id) {
            this->id(id);
        }

        /** Return this symbol's fragment reference, requiring the symbol to have an id. */
        std::string href() const;
        ElementKind kind() const override { return static_kind; }
        /** Create a use element that references this symbol. */
        Use use(double x, double y) const;
        /** Create a sized use element that references this symbol. */
        Use use(double x, double y, double width, double height) const;
    };

    /** Instance of a reusable SVG element referenced by href. */
    class Use : public Shape {
    public:
        static constexpr ElementKind static_kind = ElementKind::Use;
        Use() = default;
        using Shape::Shape;

        explicit Use(std::string href) {
            set_attr("href", std::move(href));
        }

        Use(std::string href, double x, double y) : Use(std::move(href)) {
            set_attr("x", x);
            set_attr("y", y);
        }

        Use(std::string href, double x, double y, double width, double height) :
                Use(std::move(href), x, y) {
            set_attr("width", width);
            set_attr("height", height);
        }

        /** Set the legacy xlink:href reference for older SVG consumers. */
        Use& xlink_href(std::string href) {
            set_attr("xlink:href", std::move(href));
            return *this;
        }
        ElementKind kind() const override { return static_kind; }
    };

    class SVG : public Shape {
        friend class Element;

        std::map<std::string, Element*> id_index_;
        Defs* defs_ = nullptr;

    public:
        class Style : public Element {
        public:
            static constexpr ElementKind static_kind = ElementKind::Style;
            Style() = default;
            using Element::Element;
            SelectorProperties css; /**< Basic CSS styling */
            std::map<std::string, SelectorProperties> media_queries; /**< CSS media queries */
            std::map<std::string, SelectorProperties> keyframes; /**< CSS animations */
            ElementKind kind() const override { return static_kind; }

        protected:
            std::string svg_to_string(const size_t) override;
        };

        static constexpr ElementKind static_kind = ElementKind::SVG;
        /** Create an SVG root element with the default namespace unless attributes override it. */
        SVG(SVGAttrib _attr =
                { { "xmlns", "http://www.w3.org/2000/svg" } }
        ) : Shape(_attr) {
            set_owner_svg(this);
            rebuild_id_index();
        };

        SVG(SVG&& other) noexcept :
                Shape(std::move(other)),
                id_index_(std::move(other.id_index_)),
                defs_(nullptr),
                css(nullptr) {
            refresh_special_children();
            set_owner_svg(this);
            rebuild_id_index();
        }

        SVG& operator=(SVG&& other) noexcept {
            if (this != &other) {
                Shape::operator=(std::move(other));
                id_index_ = std::move(other.id_index_);
                defs_ = nullptr;
                css = nullptr;
                refresh_special_children();
                set_owner_svg(this);
                rebuild_id_index();
            }
            return *this;
        }

        /** Retrieve a handle corresponding to the given CSS selector */
        AttributeMap& style(const std::string& key) { return this->css->css[key]; }

        /** Set selector styles from Attrs and return the SVG for chaining. */
        SVG& style(const std::string& key, const Attrs& attrs) {
            this->style(key).set_attrs(attrs);
            return *this;
        }

        /** Retrieve a handle corresponding to a selector within a CSS media query */
        AttributeMap& media_style(const std::string& query, const std::string& key) {
            return this->css->media_queries[query][key];
        }

        /** Set media-query selector styles from Attrs and return the SVG for chaining. */
        SVG& media_style(const std::string& query, const std::string& key, const Attrs& attrs) {
            this->media_style(query, key).set_attrs(attrs);
            return *this;
        }

        /** Define typed CSS custom properties in the :root style block. */
        template<typename T>
        Variables<T> set_vars(std::initializer_list<VariableSpec<T>> specs) {
            return this->set_vars<T>(":root", specs);
        }

        /** Define typed CSS custom properties in a selector style block. */
        template<typename T>
        Variables<T> set_vars(const std::string& selector, std::initializer_list<VariableSpec<T>> specs) {
            return Variables<T>(this->style(selector), specs);
        }

        /** Define typed CSS custom properties in a selector inside a media query. */
        template<typename T>
        Variables<T> set_vars(const std::string& query,
                              const std::string& selector,
                              std::initializer_list<VariableSpec<T>> specs) {
            return Variables<T>(this->media_style(query, selector), specs);
        }

        std::map<std::string, AttributeMap>& keyframes(const std::string& key) {
            /** Add or modify an animation keyframe
             *
             *  @param[in] key The name of the animation
             */
            if (!this->css) this->css = this->add_child<Style>();
            return this->css->keyframes[key];
        }

        /** Return the document's singleton defs element, creating it after styles when needed. */
        Defs* defs() {
            if (!this->defs_) this->defs_ = this->add_child<Defs>();
            return this->defs_;
        }

        template<typename T, typename... Args>
        typename std::enable_if<!std::is_same<T, Defs>::value, T*>::type add_child(Args&&... args) {
            return Element::add_child<T>(std::forward<Args>(args)...);
        }

        template<typename T, typename... Args>
        typename std::enable_if<std::is_same<T, Defs>::value, T*>::type add_child(Args&&... args) {
            if (this->defs_) return this->defs_;

            auto child = detail::make_child<T>(std::forward<Args>(args)...);
            auto* inserted = static_cast<T*>(
                this->insert_child(std::move(child), this->defs_insert_position()));
            this->defs_ = inserted;
            return inserted;
        }

        Element* get_element_by_id(const std::string& id) {
            const auto found = this->id_index_.find(id);
            return found == this->id_index_.end() ? nullptr : found->second;
        }

        /** Return an element by id only when its built-in kind matches T::static_kind. */
        template<typename T>
        T* get_element_by_id(const std::string& id) {
            SVG_TYPE_CHECK;
            auto* element = this->get_element_by_id(id);
            if (!element || element->kind() != T::static_kind) return nullptr;
            return static_cast<T*>(element);
        }

        Style* css = this->add_child<Style>(); /**< This item's associated CSS stylesheet */
        ElementKind kind() const override { return static_kind; }

    protected:

    private:
        void refresh_special_children() {
            this->css = nullptr;
            this->defs_ = nullptr;
            for (auto& child : this->children) {
                if (child->kind() == Style::static_kind) {
                    this->css = static_cast<Style*>(child.get());
                } else if (child->kind() == Defs::static_kind) {
                    this->defs_ = static_cast<Defs*>(child.get());
                }
            }
        }

        void register_id(Element& element, const std::string& id) {
            if (id.empty()) return;
            const auto found = this->id_index_.find(id);
            if (found != this->id_index_.end() && found->second != &element) {
                throw std::invalid_argument("Duplicate SVG element id: " + id);
            }
            this->id_index_[id] = &element;
        }

        void unregister_id(Element& element, const std::string& id) {
            if (id.empty()) return;
            const auto found = this->id_index_.find(id);
            if (found != this->id_index_.end() && found->second == &element) {
                this->id_index_.erase(found);
            }
        }

        void rebuild_id_index() {
            this->id_index_.clear();
            this->register_subtree_ids();
        }

        ChildIterator defs_insert_position() {
            if (!this->css) return this->children.begin();

            for (auto it = this->children.begin(); it != this->children.end(); ++it) {
                if (it->get() == this->css) return it + 1;
            }
            return this->children.begin();
        }

    };

    inline SVG* Element::owner_svg() {
        return this->owner_;
    }

    inline const SVG* Element::owner_svg() const {
        return this->owner_;
    }

    inline void Element::set_owner_svg(SVG* owner) {
        this->owner_ = owner;
        for (auto& child : this->children) {
            child->set_owner_svg(owner);
        }
    }

    inline Element& Element::id(const std::string& value) {
        const auto old_indexed_id = this->indexed_id_;
        auto* root = this->owner_svg();

        if (value.empty()) {
            if (root && !old_indexed_id.empty()) {
                root->unregister_id(*this, old_indexed_id);
            }
            this->mutable_attrs().erase("id");
            this->indexed_id_.clear();
            return *this;
        }

        if (root && old_indexed_id != value) {
            root->register_id(*this, value);
        }
        if (root && !old_indexed_id.empty() && old_indexed_id != value) {
            root->unregister_id(*this, old_indexed_id);
        }

        this->mutable_attrs()["id"] = value;
        this->indexed_id_ = root ? value : "";
        return *this;
    }

    inline std::string Element::id() const {
        return this->get_attr("id");
    }

    inline void Element::set_attr_value(const std::string& key, const std::string& value) {
        if (key == "id") {
            this->id(value);
            return;
        }
        AttributeMap::set_attr_value(key, value);
    }

    inline AttributeMap::AttrSetter Element::make_attr_setter(const std::string& key) {
        if (key != "id") {
            return AttributeMap::make_attr_setter(key);
        }

        this->id("");
        this->mutable_attrs()[key] = "";
        return AttrSetter(this->mutable_attrs().at(key), false, [this](const std::string& value) {
            const auto previous = this->indexed_id_;
            try {
                this->id(value);
            } catch (...) {
                if (previous.empty()) {
                    this->mutable_attrs().erase("id");
                } else {
                    this->mutable_attrs()["id"] = previous;
                }
                throw;
            }
        });
    }

    inline void Element::register_subtree_ids() {
        this->register_own_id();
        for (auto& child : this->children) {
            child->register_subtree_ids();
        }
    }

    inline void Element::unregister_subtree_ids() {
        this->unregister_own_id();
        for (auto& child : this->children) {
            child->unregister_subtree_ids();
        }
    }

    inline void Element::register_own_id() {
        const auto current_id = this->id();
        if (current_id.empty()) return;

        if (auto* root = this->owner_svg()) {
            root->register_id(*this, current_id);
            this->indexed_id_ = current_id;
        }
    }

    inline void Element::unregister_own_id() {
        if (this->indexed_id_.empty()) return;

        if (auto* root = this->owner_svg()) {
            root->unregister_id(*this, this->indexed_id_);
        }
        this->indexed_id_.clear();
    }

    inline Symbol* Defs::symbol(std::string id) {
        for (auto* child : this->get_immediate_children<Symbol>()) {
            if (child->id() == id) return child;
        }
        return this->add_child<Symbol>(std::move(id));
    }

    inline std::string Symbol::href() const {
        const auto symbol_id = this->id();
        if (symbol_id.empty()) {
            throw std::logic_error("SVG symbol must have an id before it can be referenced");
        }
        return "#" + symbol_id;
    }

    inline Use Symbol::use(double x, double y) const {
        return Use(this->href(), x, y);
    }

    inline Use Symbol::use(double x, double y, double width, double height) const {
        return Use(this->href(), x, y, width, height);
    }

    class Path : public Shape {
    public:
        static constexpr ElementKind static_kind = ElementKind::Path;
        using Shape::Shape;
        ElementKind kind() const override { return static_kind; }

        template<typename T>
        inline void start(T x, T y) {
            /** Start line at (x, y)
             *  This function overwrites the current path if it exists
             */
            this->set_attr("d", "M " + std::to_string(x) + " " + std::to_string(y));
            this->x_start = x;
            this->y_start = y;
        }

        template<typename T>
        inline void line_to(T x, T y) {
            /** Draw a line to (x, y)
             *  If line has not been initialized by setting a starting point,
             *  then start() will be called with (x, y) as arguments
             */

            if (!this->has_attr("d"))
                start(x, y);
            else
                this->mutable_attrs()["d"] += " L " + std::to_string(x) +
                                             " " + std::to_string(y);
        }

        inline void line_to(std::pair<double, double> coord) {
            this->line_to(coord.first, coord.second);
        }

        inline void to_origin() {
            /** Draw a line back to the origin */
            this->line_to(x_start, y_start);
        }
    private:
        double x_start;
        double y_start;
    };

    /** Text positioned with x/y coordinates. */
    class Text : public Element {
    public:
        static constexpr ElementKind static_kind = ElementKind::Text;
        Text() = default;
        using Element::Element;
        ElementKind kind() const override { return static_kind; }

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
    };

    /** Native SVG title element whose content is XML-escaped when serialized. */
    class Title : public Element {
    public:
        /** Element kind used for kind-based typed lookup. */
        static constexpr ElementKind static_kind = ElementKind::Title;
        Title() = default;
        /** Construct a title from text content, not an id attribute. */
        explicit Title(const char* _content) : content(_content) {}
        /** Construct a title from text content, not an id attribute. */
        explicit Title(std::string _content) : content(std::move(_content)) {}
        ElementKind kind() const override { return static_kind; }

    protected:
        /** Unescaped title text; escaping is applied during serialization. */
        std::string content;
        /** Serialize as an XML-escaped title element. */
        std::string svg_to_string(const size_t) override;
    };

    class Group : public Element {
    public:
        static constexpr ElementKind static_kind = ElementKind::Group;
        using Element::Element;
        ElementKind kind() const override { return static_kind; }
    };
/** Short alias for Group, matching SVG's native @c g tag name. */
using G = Group;

    class Line : public Shape {
    public:
        static constexpr ElementKind static_kind = ElementKind::Line;
        Line() = default;
        using Shape::Shape;
        ElementKind kind() const override { return static_kind; }

        Line(double x1, double x2, double y1, double y2) : Shape({
                { "x1", to_string(x1) },
                { "x2", to_string(x2) },
                { "y1", to_string(y1) },
                { "y2", to_string(y2) }
        }) {};

        Line(Point x, Point y) : Line(x.first, y.first, x.second, y.second) {};

        virtual double x() override { return x1() + (x2() - x1()) / 2; }
        virtual double y() override { return y1() + (y2() - y1()) / 2; }
        double x1() { return this->find_numeric("x1"); }
        double x2() { return this->find_numeric("x2"); }
        double y1() { return this->find_numeric("y1"); }
        double y2() { return this->find_numeric("y2"); }

        double width() override { return std::abs(x2() - x1()); }
        double height() override { return std::abs(y2() - y1()); }
        double length() { return std::sqrt(pow(width(), 2) + pow(height(), 2)); }
        double slope() { return (y2() - y1()) / (x2() - x1()); }
        double angle() { return atan(this->slope()) * RAD_TO_DEG; }

        std::pair<double, double> along(double percent);

    protected:
        Element::BoundingBox get_bbox() override;   
    };

    class Rect : public Shape {
    public:
        static constexpr ElementKind static_kind = ElementKind::Rect;
        Rect() = default;
        using Shape::Shape;
        ElementKind kind() const override { return static_kind; }

        Rect(
            double x, double y, double width, double height) :
            Shape({
                    { "x", to_string(x) },
                    { "y", to_string(y) },
                    { "width", to_string(width) },
                    { "height", to_string(height) }
            }) {};

        Element::BoundingBox get_bbox() override;
    };

    class Circle : public Shape {
    public:
        static constexpr ElementKind static_kind = ElementKind::Circle;
        Circle() = default;
        using Shape::Shape;
        ElementKind kind() const override { return static_kind; }

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
    };

    class Polygon : public Element {
    public:
        static constexpr ElementKind static_kind = ElementKind::Polygon;
        Polygon() = default;
        using Element::Element;
        ElementKind kind() const override { return static_kind; }

        Polygon(const std::vector<Point>& points) {
            // Quick and dirty
            std::string point_str;
            for (auto& pt : points)
                point_str += to_string(pt) + " ";
            this->set_attr("points", point_str);
        };
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
            double length = percent * this->length();
            double discrim = std::sqrt(4 * pow(length, 2) * (1 / (1 + pow(slope(), 2))));

            double x_a = (2 * x1() + discrim) / 2;
            double x_b = (2 * x1() - discrim) / 2;
            x_pos = x_a;

            if ((x_a > x1() && x_a > x2()) || (x_a < x1() && x_a < x2()))
                x_pos = x_b;

            y_pos = slope() * (x_pos - x1()) + y1();
        }
        else { // Edge case:: Completely vertical lines
            x_pos = x1();

            if (y1() > y2()) // Downward pointing
                y_pos = y1() - percent * this->length();
            else
                y_pos = y1() + percent * this->length();
        }

        return std::make_pair(x_pos, y_pos);
    }

    inline std::string Element::svg_to_string(const size_t indent_level) {
        /** Return the string representation of an SVG element
         *
         *  @param[out] indent_level The current level of indentation
         */
         
        auto indent = std::string(indent_level, '\t');
        std::string ret = indent + "<" + tag();

        // Set attributes
        for (auto& pair: attrs())
            ret += " " + pair.first + "=" + "\"" + escape_xml(pair.second) + "\"";

        if (!this->children.empty()) {
            ret += ">\n";

            // Recursively get strings for child elements
            for (auto& child : children) {
                // Avoid adding empty strings
                auto str = child->svg_to_string(indent_level + 1);
                if (str.size()) ret += str +"\n";
            }

            return ret += indent + "</" + tag() + ">";
        }

        return ret += " />";
    }

    namespace detail {
        /** Shared serializer for elements whose children are escaped text content. */
        inline std::string text_content_element_to_string(const Element& element,
                                                          const std::string& tag,
                                                          const std::string& content,
                                                          const size_t indent_level) {
            auto indent = std::string(indent_level, '\t');
            std::string ret = indent + "<" + tag;
            for (auto& pair: element.attrs())
                ret += " " + pair.first + "=" + "\"" + escape_xml(pair.second) + "\"";
            return ret += ">" + escape_xml(content) + "</" + tag + ">";
        }
    }

    inline std::string to_string(const std::map<std::string, AttributeMap>& css, const size_t indent_level) {
        /** Print out a CSS attribute block */
        auto indent = std::string(indent_level, '\t'), ret = std::string();
        for (auto& selector : css) {
            // Loop over each selector's attribute/value pairs
            ret += indent + "\t\t" + selector.first + " {\n";
            for (auto& attr : selector.second.attrs())
                ret += indent + "\t\t\t" + attr.first + ": " + attr.second + ";\n";
            ret += indent + "\t\t" + "}\n";
        }
        return ret;
    }

    inline std::string SVG::Style::svg_to_string(const size_t indent_level) {
        /** Create a CSS stylesheet */
        auto indent = std::string(indent_level, '\t');

        if (!this->css.empty() || !this->media_queries.empty() || !this->keyframes.empty()) {
            std::string ret = indent + "<style type=\"text/css\">\n" +
                indent + "\t<![CDATA[\n";

            // Begin CSS stylesheet
            ret += to_string(this->css, indent_level);

            // Media queries
            for (auto& media : this->media_queries) {
                ret += indent + "\t\t@media " + media.first + " {\n" +
                    to_string(media.second, indent_level + 1) +
                    indent + "\t\t" + "}\n";
            }

            // Animation frames
            for (auto& anim : this->keyframes) {
                ret += indent + "\t\t@keyframes " + anim.first + " {\n" +
                    to_string(anim.second, indent_level + 1) +
                    indent + "\t\t" + "}\n";
            }

            ret += indent + "\t]]>\n";
            return ret + indent + "</style>";
        }

        return "";
    }

    inline std::string Text::svg_to_string(const size_t indent_level) {
        return detail::text_content_element_to_string(*this, tag_name(this->kind()), this->content, indent_level);
    }

    inline std::string Title::svg_to_string(const size_t indent_level) {
        return detail::text_content_element_to_string(*this, tag_name(this->kind()), this->content, indent_level);
    }

    inline void Element::autoscale(const double margin) {
        /** Like other autoscale() but accepts margin as a percentage */
        Element::BoundingBox bbox = this->get_bbox();
        this->get_bbox(bbox);
        double width = bbox.x2 - bbox.x1,
            height = bbox.y2 - bbox.y1;

        this->autoscale({
            width * margin, width * margin,
            height * margin, height * margin 
        });
    }

    inline void Element::autoscale(const Margins& margins) {
        /** Automatically set the width, height, and viewBox attribute of this item
         *  so that it can contain all of its children without clipping
         *
         *  @param[in] margins Extra margins for the sides
         */
        using std::stof;

        Element::BoundingBox bbox = this->get_bbox();
        this->get_bbox(bbox); // Compute the bounding box (recursive)
        double width = bbox.x2 - bbox.x1 + margins.x1 + margins.x2,
            height = bbox.y2 - bbox.y1 + margins.y1 + margins.y2,
            x1 = bbox.x1 - margins.x1, y1 = bbox.y1 - margins.y1;

        this->set_attr("width", width)
             .set_attr("height", height);

        std::stringstream viewbox;
        viewbox << std::fixed << std::setprecision(1)
            << x1 << " " // min-x
            << y1 << " " // min-y
            << width << " "
            << height;
        this->set_attr("viewBox", viewbox.str());
    }

    inline void Element::get_bbox(Element::BoundingBox& box) {
        /** Recursively compute a bounding box */
        this->get_bbox(box, detail::AffineTransform());
    }

    inline void Element::get_bbox(Element::BoundingBox& box, const detail::AffineTransform& parent_transform) {
        /** Recursively compute a transform-aware bounding box */
        auto transform = parent_transform;
        if (this->has_attr("transform")) {
            transform = detail::multiply(transform, detail::parse_supported_transform(this->get_attr("transform")));
        }

        auto this_bbox = this->get_bbox();
        if (!std::isnan(this_bbox.x1) && !std::isnan(this_bbox.x2) &&
                !std::isnan(this_bbox.y1) && !std::isnan(this_bbox.y2)) {
            const auto p1 = transform.apply({ this_bbox.x1, this_bbox.y1 });
            const auto p2 = transform.apply({ this_bbox.x2, this_bbox.y1 });
            const auto p3 = transform.apply({ this_bbox.x2, this_bbox.y2 });
            const auto p4 = transform.apply({ this_bbox.x1, this_bbox.y2 });
            Element::BoundingBox transformed_box(
                std::min(std::min(p1.first, p2.first), std::min(p3.first, p4.first)),
                std::max(std::max(p1.first, p2.first), std::max(p3.first, p4.first)),
                std::min(std::min(p1.second, p2.second), std::min(p3.second, p4.second)),
                std::max(std::max(p1.second, p2.second), std::max(p3.second, p4.second)));
            box = transformed_box + box;
        }

        for (auto& child: this->children) child->get_bbox(box, transform);
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

    inline SVG merge(SVG& left, SVG& right, const Margins& margins) {
        /** Merge two SVG documents together horizontally with a uniform margin */
        SVG ret;

        // Move items
        ret << std::move(left) << std::move(right);

        // Set bounding box of individual pieces
        for (auto& svg_child: ret.get_immediate_children<SVG>())
            svg_child->autoscale(margins);

        // Set x position for child SVG elements, and compute width/height for this
        double x = 0, height = 0;
        for (auto& svg_child: ret.get_immediate_children<SVG>()) {
            svg_child->set_attr("x", x).set_attr("y", 0);
            x += svg_child->width();
            height = std::max(height, svg_child->height());
        }

        ret.set_attr("width", x).set_attr("height", height);
        return ret;
    }

    inline std::vector<Point> bounding_polygon(std::vector<Shape*>& shapes) {
        /* Convert shapes into sets of points, aggregate them, and then calculate
         * convex hull for aggregate set
         */
        std::vector<Point> points;
        for (auto& shp : shapes) {
            auto temp_points = shp->points();
            std::move(temp_points.begin(), temp_points.end(), std::back_inserter(points));
        }

        return util::convex_hull(points);
    }

    inline SVG merge(std::vector<SVG>& frames, const double width, const int max_frame_width) {
        /** Given a vector of SVGs, merge them together
         *  max_frame_width: Maximum width of any individual frame
         */
        SVG root;
        double x = 0, y = 0, total_width = 0, total_height = 0;
        for (auto& frame : frames) {
            // Scale
            frame.autoscale();
            if (frame.width() > max_frame_width) {
                const double scale_factor = max_frame_width/frame.width();
                frame.set_attr("width", max_frame_width);
                frame.set_attr("height", frame.height() * scale_factor); // Scale height proportionally
            }
        }

        // Move
        double current_height = 0;
        for (auto& frame : frames) {
            // Push to next row
            if ((x + frame.width()) > width) {
                total_width = std::max(total_width, x);
                x = 0;
                y += current_height;
                current_height = 0;
            }

            frame.set_attr("x", x).set_attr("y", y);
            x += frame.width();
            current_height = std::max(current_height, frame.height());
            root << std::move(frame);
        }

        total_height = y + current_height;

        // Set viewbox
        root.set_attr("viewBox") << 0 << " " << 0 << " " << total_width << " " << total_height;
        root.set_attr("width", total_width).set_attr("height", total_height);
        return root;
    }

    inline SVG frame_animate(std::vector<SVG>& frames, const double fps) {
        /** Given a vector of SVGs, create a frame-by-frame animation of them
         *
         *  @param[in]  A vector of frames (SVGs)
         *  @param[out] fps Numbers of frames per second
         */
        SVG root;
        const double duration = (double)frames.size() / fps; // [seconds]
        const double frame_step = 1.0 / fps; // duration of each frame [seconds]
        int current_frame = 0;

        root.style("svg.animated").set_attr("animation-iteration-count", "infinite")
            .set_attr("animation-timing-function", "step-end")
            .set_attr("animation-duration", std::to_string(duration) + "s")
            .set_attr("opacity", 0);

        // Move frames into new SVG
        for (auto& frame : frames) {
            std::string frame_id = "frame_" + std::to_string(current_frame);
            frame.set_attr("id", frame_id).set_attr("class", "animated");
            root.style("#" + frame_id).set_attr("animation-name",
                "anim_" + std::to_string(current_frame));
            current_frame++;
            root << std::move(frame);
        }

        // Set animation frames
        for (size_t i = 0, ilen = frames.size(); i < ilen; i++) {
            auto& anim = root.keyframes("anim_" + std::to_string(i));
            double begin_pct = (double)i / frames.size(),
                end_pct = (double)(i + 1) / frames.size();
            anim["0%"].set_attr("opacity", 0);
            anim[std::to_string(begin_pct * 100) + "%"].set_attr("opacity", 1);
            anim[std::to_string(end_pct * 100) + "%"].set_attr("opacity", 0);
        }

        // Scale and center child SVGs
        double width = 0, height = 0;

        for (auto& child : root.get_immediate_children<SVG>()) {
            child->autoscale();
            width = std::max(width, child->width());
            height = std::max(height, child->height());
        }

        root.set_attr("viewBox", "0 0 " + std::to_string(width) + " " + std::to_string(height));

        // Center child SVGs
        for (auto& child : root.get_immediate_children<SVG>())
            child->set_attr("x", (width - child->width())/2).set_attr("y", (height - child->height())/2);

        return root;
    }
}
