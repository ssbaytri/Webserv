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

std::string	toLowerCase(const std::string& str)
{
    std::string result = str;
    for (size_t i = 0; i < result.length(); ++i) {
        result[i] = std::tolower(result[i]);
    }
    return result;
}

std::string trim(const std::string& str)
{
    size_t start = 0;
    size_t end = str.length();
    
    while (start < end && std::isspace(str[start])) {
        start++;
    }
    
    while (end > start && std::isspace(str[end - 1])) {
        end--;
    }
    
    return str.substr(start, end - start);
}


