#pragma once

#include "bmp.h"

#include <unordered_map>
#include <cassert>
#include <memory>
#include <vector>

struct Point2D {
    Point2D() = default;
    Point2D(const int x_, const int y_) : x(x_), y(y_) {}

    bool operator==(const Point2D& other) const {
        return x == other.x && y == other.y;
    }

    virtual ~Point2D() {}

    int x{0};
    int y{0};
};

using Point = Point2D;

namespace std {
    template<>
    struct hash<Point> {
        size_t operator()(const Point& p) const noexcept {
            std::size_t h1 = std::hash<int>{}(p.x);
            std::size_t h2 = std::hash<int>{}(p.y);
            return h1 ^ (h2 << 1);
        }
    };
}

struct Point3D : Point2D {

    Point get_2d() {
        return {x, y};
    }


    int &operator[](char ch) {
        switch (ch) {
        case 'X':
            return x;
        case 'Y':
            return y;
        case 'Z':
            return z;
        default:
            assert(false);
            break;
        }
    }

    int z{0};
};

struct Line {
    Point start;
    Point end;

    void draw(BMPImage &img, RGB rgb) {
        draw_line(img, rgb);
        draw_points(img);
    }

    void draw_points(BMPImage &img) {
        img.draw_point(start.x, start.y);
        img.draw_point(end.x, end.y);
    }

    virtual void draw_line(BMPImage &img, RGB rgb) {
        img.draw_line(start.x, start.y, end.x, end.y, rgb);
    }

    virtual ~Line() {}
};

struct CircularLine : public Line {

    void draw_line(BMPImage &img, RGB rgb) override{
        img.draw_arc(start.x, start.y, end.x, end.y, center.x, center.y, clockwise, rgb);
    }

    Point center;
    bool clockwise;
};


class Shape {
public:
    using LinePtr = std::unique_ptr<Line>;

    void add_line(LinePtr &&ptr) {
        lines.emplace_back(std::move(ptr));
    }

    void draw(BMPImage &img) {
        RGB rgb;
        rgb.gen();
        for (auto &line : lines) {
            line->draw(img, rgb);
        }
    }

private:
    std::vector<std::unique_ptr<Line>> lines;
};