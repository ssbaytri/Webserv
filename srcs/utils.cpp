#include "../includes/utils.hpp"
#include <iostream>
#include <cctype>
#include <sys/stat.h>
#include <fstream>
#include <unistd.h>
#include <dirent.h>

// --- ANSI Escape Codes ---
#define COLOR_RESET   "\033[0m"
#define COLOR_BOLD    "\033[1m"
#define COLOR_DIM     "\033[2m"
#define COLOR_RED     "\033[31m"
#define COLOR_GREEN   "\033[32m"
#define COLOR_YELLOW  "\033[33m"
#define COLOR_BLUE    "\033[34m"
#define COLOR_MAGENTA "\033[35m"
#define COLOR_CYAN    "\033[36m"

static std::string getTimestamp()
{
    char buffer[20];
    std::time_t now = std::time(NULL);
    std::tm* timeinfo = std::localtime(&now);
    std::strftime(buffer, sizeof(buffer), "%Y-%m-%d %H:%M:%S", timeinfo);
    return std::string(buffer);
}

std::string intToString(int num) {
    std::stringstream ss;
    ss << num;
    return ss.str();
}

void logMessage(const std::string& message)
{
    std::string timestamp = getTimestamp();
    std::string icon = "ℹ️";
    std::string prefixColor = COLOR_BLUE;

    // --- Smart Heuristics ---
    // Automatically detect what the server is doing based on the string content!
    
    if (message.find("GET ") != std::string::npos || 
        message.find("POST ") != std::string::npos || 
        message.find("DELETE ") != std::string::npos) {
        icon = "📥"; // Incoming request
        prefixColor = COLOR_CYAN;
    } 
    else if (message.find("HTTP/1.1 2") != std::string::npos || 
             message.find("HTTP/1.1 3") != std::string::npos) {
        icon = "📤"; // Outgoing success response
        prefixColor = COLOR_GREEN;
    }
    else if (message.find("HTTP/1.1 4") != std::string::npos || 
             message.find("HTTP/1.1 5") != std::string::npos) {
        icon = "📤"; // Outgoing error response
        prefixColor = COLOR_YELLOW;
    }
    else if (message.find("connection") != std::string::npos || 
             message.find("accept") != std::string::npos ||
             message.find("client") != std::string::npos) {
        icon = "🔗"; // Connection events
        prefixColor = COLOR_MAGENTA;
    }
    else if (message.find("listen") != std::string::npos || 
             message.find("start") != std::string::npos ||
             message.find("Server") != std::string::npos) {
        icon = "🚀"; // Boot up events
        prefixColor = COLOR_GREEN;
    }

    // Print the stylish log
    std::cout << COLOR_DIM << "[" << timestamp << "] " << COLOR_RESET
              << prefixColor << COLOR_BOLD << icon << " [INFO] " << COLOR_RESET 
              << message << std::endl;
}

void logError(const std::string& error)
{
    std::string timestamp = getTimestamp();
    
    // Print the stylish error
    std::cerr << COLOR_DIM << "[" << timestamp << "] " << COLOR_RESET
              << COLOR_RED << COLOR_BOLD << "✖️ [ERROR] " << COLOR_RESET 
              << COLOR_RED << error << COLOR_RESET << std::endl;
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

bool writeFile(const std::string& filePath, const std::string& content)
{
    std::ofstream file(filePath.c_str(), std::ios::binary);
    if (!file.is_open()) {
        logError("Failed to open file for writing: " + filePath);
        return false;
    }
    
    file.write(content.c_str(), content.size());
    file.close();
    
    return true;
}

std::vector<FileInfo> listDirectory(const std::string& path) {
    std::vector<FileInfo> files;
    DIR* dir = opendir(path.c_str());
    
    if (!dir) {
        return files;
    }
    
    struct dirent* entry;
    while ((entry = readdir(dir)) != NULL) {
        std::string name = entry->d_name;
        
        if (name == "." || name == "..") {
            continue;
        }
        
        FileInfo info;
        info.name = name;
        
        std::string fullPath = path + "/" + name;
        struct stat st;
        
        if (stat(fullPath.c_str(), &st) == 0) {
            info.isDirectory = S_ISDIR(st.st_mode);
            info.size = st.st_size;
            info.modified = st.st_mtime;
        } else {
            info.isDirectory = false;
            info.size = 0;
            info.modified = 0;
        }
        
        if (info.isDirectory) {
            info.name += "/";
        }
        
        files.push_back(info);
    }
    
    closedir(dir);
    return files;
}

std::string formatSize(size_t bytes) {
    const char* units[] = {"B", "KB", "MB", "GB"};
    int unit = 0;
    double size = static_cast<double>(bytes);
    
    while (size >= 1024.0 && unit < 3) {
        size /= 1024.0;
        unit++;
    }
    
    std::stringstream ss;
    ss.precision(1);
    ss << std::fixed << size << " " << units[unit];
    return ss.str();
}

std::string formatTime(time_t time) {
    char buffer[80];
    struct tm* timeinfo = localtime(&time);
    strftime(buffer, sizeof(buffer), "%Y-%m-%d %H:%M", timeinfo);
    return std::string(buffer);
}

std::string getFileExtension(const std::string& path)
{
    size_t dotPos = path.find_last_of('.');
    
    if (dotPos == std::string::npos) {
        return "";
    }
    
    return path.substr(dotPos);
}
