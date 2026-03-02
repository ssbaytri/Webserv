#ifndef CONFIG_HPP
#define CONFIG_HPP

#include <string>
#include <vector>
#include <map>

// Location block configuration
struct LocationConfig {
    std::string path;                           // "/uploads"
    std::vector<std::string> allowedMethods;    // ["GET", "POST", "DELETE"]
    std::string root;                           // "/var/www/uploads" (optional override)
    bool autoindex;                             // true/false
    std::string redirect;                       // "/new-page" (for redirects)
    std::map<std::string, std::string> cgiPass; // {".php": "/usr/bin/php-cgi"}
    size_t clientMaxBodySize;                   // Optional override
    
    LocationConfig() : autoindex(false), clientMaxBodySize(0) {}
};

// Server block configuration
struct ServerConfig {
    int port;                                   // 8080
    std::string serverName;                     // "localhost"
    std::string root;                           // "/var/www/html"
    std::vector<std::string> index;             // ["index.html", "index.htm"]
    size_t clientMaxBodySize;                   // 10485760 (10MB in bytes)
    std::map<int, std::string> errorPages;      // {404: "/errors/404.html"}
    std::vector<LocationConfig> locations;      // All location blocks
    
    ServerConfig() : port(8080), clientMaxBodySize(1048576) {} // Default 1MB
};

// Main config class
class Config {
private:
    std::vector<ServerConfig> _servers;
    
    // Parsing helpers
    std::string _trim(const std::string& str);
    std::vector<std::string> _tokenize(const std::string& line);
    bool _parseServerBlock(std::ifstream& file, ServerConfig& server);
    bool _parseLocationBlock(std::ifstream& file, LocationConfig& location);
    size_t _parseSize(const std::string& sizeStr);

public:
    Config();
    ~Config();
    
    bool parse(const std::string& filename);
    bool validate() const;
    const std::vector<ServerConfig>& getServers() const;
    void print() const;  // For debugging
};

#endif