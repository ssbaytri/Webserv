#include "../includes/utils.hpp"
#include <iostream>

std::string intToString(int num) {
    std::stringstream ss;
    ss << num;
    return ss.str();
}

void logMessage(const std::string& message) {
    std::cout << "[INFO] " << message << std::endl;
}

void logError(const std::string& error) {
    std::cerr << "[ERROR] " << error << std::endl;
}