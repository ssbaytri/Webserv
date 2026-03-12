#ifndef UTILS_HPP
#define UTILS_HPP

#include <string>
#include <vector>
#include <sstream>
#include <ctime>
#include <string>
#include <iomanip>

struct FileInfo {
    std::string name;
    bool isDirectory;
    size_t size;
    time_t modified;
};

std::string intToString(int num);
void        logMessage(const std::string& message);
void        logError(const std::string& error);
std::string	toLowerCase(const std::string& str);
std::string trim(const std::string& str);
std::string formatSize(std::size_t bytes);
std::string formatTime(std::time_t t);
std::string getFileExtension(const std::string& path);

bool        				fileExists(const std::string& path);
bool        				isDirectory(const std::string& path);
std::string 				readFile(const std::string& path);
std::string 				getMimeType(const std::string& path);
bool        				deleteFile(const std::string& path);
bool        				isPathSafe(const std::string& path);
bool						writeFile(const std::string& filePath, const std::string& content);
std::vector<FileInfo>	    listDirectory(const std::string& path);

#endif