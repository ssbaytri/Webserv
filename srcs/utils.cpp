#include "../includes/utils.hpp"
#include <iostream>
#include <cctype>
#include <sys/stat.h>
#include <fstream>
#include <unistd.h>

#define COLOR_ORANGE "\033[38;5;208m"
#define COLOR_RED "\033[31m"
#define COLOR_RESET "\033[0m"

std::string intToString(int num) {
    std::stringstream ss;
    ss << num;
    return ss.str();
}

void logMessage(const std::string& message) {
    std::cout << COLOR_ORANGE << "[INFO] " << message << COLOR_RESET << std::endl;
}

void logError(const std::string& error) {
    std::cerr << COLOR_RED << "[ERROR] " << error << COLOR_RESET << std::endl;
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


bool fileExists(const std::string &path)
{
    struct stat buffer;
    return (stat(path.c_str(), &buffer) == 0 && S_ISREG(buffer.st_mode));
}

bool isDirectory(const std::string &path)
{
    struct stat buffer;
    return (stat(path.c_str(), &buffer) == 0 && S_ISDIR(buffer.st_mode));
}

std::string readFile(const std::string &path)
{
    std::ifstream file(path.c_str(), std::ios::binary);
    
    if (!file.is_open()) {
        logError("Failed to open file: " + path);
        return "";
    }
    
    file.seekg(0, std::ios::end);
    size_t size = file.tellg();
    file.seekg(0, std::ios::beg);
    
    std::string content;
    content.resize(size);
    file.read(&content[0], size);
    file.close();
    
    return content;
}

std::string getMimeType(const std::string& path) {
    size_t dotPos = path.find_last_of('.');
    if (dotPos == std::string::npos) {
        return "application/octet-stream";
    }
    
    std::string ext = toLowerCase(path.substr(dotPos + 1));
    
    // Map extensions to MIME types
    if (ext == "html" || ext == "htm") return "text/html";
    if (ext == "css") return "text/css";
    if (ext == "js") return "application/javascript";
    if (ext == "json") return "application/json";
    if (ext == "xml") return "application/xml";
    if (ext == "txt") return "text/plain";
    
    // Images
    if (ext == "jpg" || ext == "jpeg") return "image/jpeg";
    if (ext == "png") return "image/png";
    if (ext == "gif") return "image/gif";
    if (ext == "svg") return "image/svg+xml";
    if (ext == "ico") return "image/x-icon";
    if (ext == "webp") return "image/webp";
    
    // Fonts
    if (ext == "woff") return "font/woff";
    if (ext == "woff2") return "font/woff2";
    if (ext == "ttf") return "font/ttf";
    if (ext == "otf") return "font/otf";
    
    // Archives
    if (ext == "zip") return "application/zip";
    if (ext == "tar") return "application/x-tar";
    if (ext == "gz") return "application/gzip";
    
    // PDF
    if (ext == "pdf") return "application/pdf";
    
    return "application/octet-stream";
}

bool deleteFile(const std::string &path)
{
    if (unlink(path.c_str()) == 0) {
        logMessage("File deleted: " + path);
        return true;
    } else {
        logError("Failed to delete file: " + path);
        return false;
    }
}

bool isPathSafe(const std::string &path)
{
     // 1. Reject paths containing ".."
    if (path.find("..") != std::string::npos) {
        logError("Path contains '..': " + path);
        return false;
    }
    
    // 2. Reject absolute paths (starting with /)
    // We want relative paths only
    if (!path.empty() && path[0] == '/') {
        // This is okay - we handle this by prepending ./www
        // But double-check there's no // which could be tricky
        if (path.length() > 1 && path[1] == '/') {
            logError("Path contains '//': " + path);
            return false;
        }
    }
    
    // 3. Reject paths with null bytes
    if (path.find('\0') != std::string::npos) {
        logError("Path contains null byte");
        return false;
    }
    
    return true;
}
