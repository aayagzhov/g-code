#include "parser.h"

#include <string>
#include <iostream>

int main() {
    std::string str;
    Parser parser;
    while ( std::getline(std::cin, str)) {
        if (!parser.add_command(str)) {
            break;
        }
    }
    parser.draw("result.bmp");
    return 0;
}