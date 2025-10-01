#pragma once

#include "shape.h"
#include "bmp.h"

#include <iostream>
#include <deque>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <sstream>
#include <functional>
#include <memory>

class Parser {
public:
    using CommandParams = std::unordered_map<char, int>;

    Parser() = default;

    bool add_command(std::string str) {
        size_t begin = str.find_first_not_of(' ');
        if (begin == std::string::npos) return true;
        size_t end = str.find(';', begin);
        str = str.substr(begin,
                end == std::string::npos ? str.size() - begin : end - begin);
        if (str.empty()) return true;

        size_t space = str.find(' ');
        std::string command = str.substr(0, space);
        if (!commands.count(command)) {
            std::cerr << "[ERROR] Unknown command \"" << command << "\"\n";
            return false;
        }

        CommandParams ps;
        std::stringstream ss(space == std::string::npos ? "" : str.substr(space));
        std::string token;
        while (ss >> token) {
            char key = token[0];
            std::string value = token.substr(1);

            if (!options.at(command).count(key)) {
                std::cerr << "[ERROR] Unknown option \"" << key << "\" for command \"" << command << "\"\n";
                return false;
            }
            if (!is_nums(value)) {
                std::cerr << "[ERROR] Invalid value \"" << value << "\" for option \"" << key
                        << "\" of command \"" << command << "\"\n";
                return false;
            }
            ps[key] = std::stoi(value);
        }
        commands[command](ps);
        return command != "M30";
    }

    void draw(const std::string& filename) {
        BMPImage img(300, 300);
        for (auto &shape : shapes) {
            shape.draw(img);
        }
        img.save_bmp(filename);
    }

private:
    bool is_nums(const std::string &str) {
        if (str.empty()) return false;
        size_t start = 0;
        if (str[0] == '-' || str[0] == '+') {
            if (str.size() == 1) return false;
            start = 1;
        }
        for (size_t i = start; i < str.size(); i++) {
            if (!std::isdigit((unsigned char)str[i])) {
                return false;
            }
        }
        return true;
    }

    void g00(const CommandParams &ps) {
        for (auto p : {'X', 'Y', 'Z'}) {
            if (ps.count(p)) {
                position[p] = ps.at(p);
            }
        }
        std::cout << "[INFO] Set new position: X" << position['X'] <<
                                            " Y" << position['Y'] <<
                                            " Z" << position['Z'] << std::endl;
    }

    void add_line(std::unique_ptr<Line> &&line) {
        auto start = line->start;
        auto end = line->end;
        if (position['Z'] < 0 && active) { // shape already exist
            if (points.count(start)) {
                auto shape_ptr = points.at(start);
                shape_ptr->add_line(std::move(line));
                points.erase(start);
                if (points.count(end)) {
                    points.erase(end);
                    return;
                }
                points.emplace(end, shape_ptr);
            } else { // new_shape
                shapes.push_back(Shape{});
                auto shape_ptr = &shapes.back();
                shape_ptr->add_line(std::move(line));
                points.emplace(start, shape_ptr);
                points.emplace(end, shape_ptr);
            }
        }
        position['X'] = end.x;
        position['Y'] = end.y;
    }

    void g01(const CommandParams &ps) {
        if (ps.count('Z')) {
            position['Z'] = ps.at('Z');
        }
        if (ps.count('X') && ps.count('Y')) {
            Point end{ps.at('X'), ps.at('Y')};
            Point start = position.get_2d();
            auto line = std::make_unique<Line>();
            line->start = start;
            line->end = end;
            add_line(std::move(line));
        }
        std::cout << "[INFO] Moving tool to position: X" << position['X'] <<
                                                    " Y" << position['Y'] <<
                                                    " Z" << position['Z'] << std::endl;
    }

    void g02(const CommandParams &ps) {
        Point end{ps.at('X'), ps.at('Y')};
        Point start = position.get_2d();
        int i = ps.at('I');
        int j = ps.at('J');
        auto line = std::make_unique<CircularLine>();
        line->start = start;
        line->end = end;
        line->center.x = start.x + i;
        line->center.y = start.y + j;
        line->clockwise = true;
        add_line(std::move(line));
    }

    void g03(const CommandParams &ps) {
        Point end{ps.at('X'), ps.at('Y')};
        Point start = position.get_2d();
        int i = ps.at('I');
        int j = ps.at('J');
        auto line = std::make_unique<CircularLine>();
        line->start = start;
        line->end = end;
        line->center.x = start.x + i;
        line->center.y = start.y + j;
        line->clockwise = false;
        add_line(std::move(line));
    }

    void g28(const CommandParams &ps) {
        if (ps.size()) {
            g00(ps);
        }
        CommandParams new_ps {{'X', 0}, {'Y', 0}};
        g00(new_ps);
    }

    void m03(const CommandParams &ps) {
        active = true;
        std::cout << "[INFO] Spindle ON";
        if (ps.count('S')) {
            std::cout << ", speed: S" << ps.at('S');
        } else {
            std::cout << ", speed: default";
        }
        std::cout << std::endl;
    }

    void m05(const CommandParams &ps) {
        active = false;
        std::cout << "[INFO] Spindle OFF" << std::endl;
    }

    void m30(const CommandParams &ps) {
        active = false;
        std::cout << "[INFO] End of the programm" << std::endl;
    }

    void empty(const CommandParams &ps) {
    }

    Point3D position; // posion
    bool active{false};

    std::unordered_map<Point, Shape*> points;
    std::deque<Shape> shapes;

    std::unordered_map<std::string, std::unordered_set<char>> options {
        {"G21", {}},
        {"G90", {}},
        {"G00", {'X', 'Y', 'Z'}},
        {"G01", {'X', 'Y', 'Z', 'F'}},
        {"G02", {'X', 'Y', 'Z', 'I', 'J', 'F'}},
        {"G03", {'X', 'Y', 'Z', 'I', 'J', 'F'}},
        {"G28", {'X', 'Y'}},
        {"M03", {'S'}},
        {"M05", {}},
        {"M30", {}}
    };

    std::unordered_map<std::string, std::function<void(const CommandParams &ps)>> commands {
        // работа в абсолютных координатах, единиц ыизмерения мм, плоскость XY,
        // сверлит только при Z < 0;
        {"G21", [this](const CommandParams &ps){ empty(ps); }},   // Быстрое позиционирование
        {"G90", [this](const CommandParams &ps){ empty(ps); }},   // Быстрое позиционирование
        {"G00", [this](const CommandParams &ps){ g00(ps); }},   // Быстрое позиционирование
        {"G01", [this](const CommandParams &ps){ g01(ps); }},   // Линейная интерполяция
        {"G02", [this](const CommandParams &ps){ g02(ps); }},   // Круговая интерполяция (движение по часовой сьрелке)
        {"G03", [this](const CommandParams &ps){ g03(ps); }},   // Круговая интерполяция (движение против часовой стрелки)
        {"G28", [this](const CommandParams &ps){ g28(ps); }},   // Возврат в нулевую точку
        {"M03", [this](const CommandParams &ps){ m03(ps); }},   // Включить шпиндель
        {"M05", [this](const CommandParams &ps){ m05(ps); }},   // Выключить шпиндель
        {"M30", [this](const CommandParams &ps){ m30(ps); }}    // Выключение программы
    };
};