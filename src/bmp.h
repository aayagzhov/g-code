#pragma once

#include <iostream>
#include <vector>
#include <cstdint>
#include <fstream>
#include <algorithm>
#include <cmath>

struct RGB {
    void hsv2rgb(double h, double s, double v) {
        double c = v * s;
        double x = c * (1 - std::fabs(fmod(h / 60.0, 2) - 1));
        double m = v - c;
        double r_, g_, b_;

        if (h < 60)      { r_ = c; g_ = x; b_ = 0; }
        else if (h < 120){ r_ = x; g_ = c; b_ = 0; }
        else if (h < 180){ r_ = 0; g_ = c; b_ = x; }
        else if (h < 240){ r_ = 0; g_ = x; b_ = c; }
        else if (h < 300){ r_ = x; g_ = 0; b_ = c; }
        else             { r_ = c; g_ = 0; b_ = x; }

        r = static_cast<uint8_t>((r_ + m) * 255);
        g = static_cast<uint8_t>((g_ + m) * 255);
        b = static_cast<uint8_t>((b_ + m) * 255);
    }

    void gen() {
        static int counter = 0;
        double h = fmod(counter * 137.508, 360.0);
        counter++;

        double s = 0.9;
        double v = 0.7;

        hsv2rgb(h, s, v);
    }

    uint8_t r;
    uint8_t g;
    uint8_t b;
};

class BMPImage {
public:
    using Image = std::vector<uint8_t>;

    BMPImage(int hight, int weight) :
        h(hight),
        w(weight),
        img(hight * weight * 3, 255) { }


    void draw_point(int x, int y, RGB rgb = RGB{255, 0, 0}){
        for (int dy = -1; dy <= 1; ++dy) {
            for (int dx = -1; dx <= 1; ++dx) {
                set_pixel(x + dx, y + dy, rgb);
            }
        }
    };

    void draw_line(int x0, int y0, int x1, int y1, RGB rgb) {
        bool steep = std::abs(y1 - y0) > std::abs(x1 - x0);
        if (steep) {
            std::swap(x0, y0);
            std::swap(x1, y1);
        }
        if (x0 > x1) {
            std::swap(x0, x1);
            std::swap(y0, y1);
        }
        int dx = x1 - x0;
        int dy = std::abs(y1 - y0);
        int error = dx / 2;
        int ystep = (y0 < y1) ? 1 : -1;
        int y = y0;
        for (int x = x0; x <= x1; ++x) {
            if (steep) {
                set_pixel(y, x, rgb);
            } else {
                set_pixel(x, y, rgb);
            }
            error -= dy;
            if (error < 0) {
                y += ystep;
                error += dx;
            }
        }
    }

    bool save_bmp(const std::string& filename) {
        int rowSizeInFile = ((w * 3 + 3) / 4) * 4;
        int dataSize = rowSizeInFile * h;
        int fileSize = 14 + 40 + dataSize;

        std::ofstream fout(filename, std::ios::binary);
        if (!fout) return false;

        // --- BITMAPFILEHEADER (14 bytes)
        fout.put('B'); fout.put('M'); // bfType
        auto write32 = [&](uint32_t v) {
            fout.put((char)(v & 0xFF));
            fout.put((char)((v>>8)&0xFF));
            fout.put((char)((v>>16)&0xFF));
            fout.put((char)((v>>24)&0xFF));
        };

        auto write16 = [&](uint16_t v) {
            fout.put((char)(v & 0xFF));
            fout.put((char)((v>>8)&0xFF));
        };

        write32(fileSize);     // bfSize
        write16(0);            // bfReserved1
        write16(0);            // bfReserved2
        write32(14 + 40);      // bfOffBits

        // --- BITMAPINFOHEADER (40 bytes)
        write32(40);           // biSize
        write32(w);            // biWidth
        write32(h);            // biHeight (positive => bottom-up). We'll write rows bottom-up.
        write16(1);            // biPlanes
        write16(24);           // biBitCount
        write32(0);            // biCompression (BI_RGB)
        write32(dataSize);     // biSizeImage
        write32(2835);         // biXPelsPerMeter (~72 DPI)
        write32(2835);         // biYPelsPerMeter
        write32(0);            // biClrUsed
        write32(0);            // biClrImportant

        // Pixel data: BMP expects rows bottom-up. Our buffer is top-down (row 0 = top).
        // So write rows from bottom to top, with padding to 4 bytes per row.
        std::vector<uint8_t> padding(rowSizeInFile - w * 3, 0);

        for (int row = h - 1; row >= 0; --row) {
            const uint8_t* rowPtr = &img[row * w * 3];
            fout.write((const char*)rowPtr, w * 3);
            if (!padding.empty()) fout.write((const char*)padding.data(), padding.size());
        }

        fout.close();
        return true;
    }

    // direction: true/false
    void draw_arc(int x0, int y0, int x1, int y1, int cx, int cy, bool ccw, RGB rgb) {
        double startAngle = atan2(y0 - cy, x0 - cx);
        double endAngle   = atan2(y1 - cy, x1 - cx);
        double radius     = std::sqrt((x0 - cx)*(x0 - cx) + (y0 - cy)*(y0 - cy));

        if (ccw) {
            if (endAngle < startAngle) endAngle += 2 * M_PI;
        } else {
            if (endAngle > startAngle) endAngle -= 2 * M_PI;
        }

        int segments = 200; // чем больше, тем плавнее
        double step = (endAngle - startAngle) / segments;

        for (int i = 0; i <= segments; i++) {
            double a = startAngle + step * i;
            int x = (int)std::round(cx + radius * std::cos(a));
            int y = (int)std::round(cy + radius * std::sin(a));
            set_pixel(x, y, rgb);
        }
    }


private:

    void set_pixel(int x, int y, RGB rgb) {
        if (x < 0 || x >= w || y < 0 || y >= h) return;
        int idx = (y * w + x) * 3;
        img[idx + 0] = rgb.b;
        img[idx + 1] = rgb.g;
        img[idx + 2] = rgb.r;
    }


    int h{0};
    int w{0};
    Image img;
};
