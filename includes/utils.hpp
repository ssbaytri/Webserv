#ifndef UTILS_HPP
#define UTILS_HPP

#include <string>
#include <sstream>

// String utilities
std::string intToString(int num);
void        logMessage(const std::string& message);
void        logError(const std::string& error);

std::string	toLowerCase(const std::string& str);
std::string trim(const std::string& str);

bool        fileExists(const std::string& path);
bool        isDirectory(const std::string& path);
std::string readFile(const std::string& path);
std::string getMimeType(const std::string& path);

#endif