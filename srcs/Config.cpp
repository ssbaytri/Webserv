#include "../includes/Config.hpp"
#include "../includes/utils.hpp"
#include <fstream>
#include <sstream>
#include <cstdlib>
#include <iostream>

Config::Config() {}
Config::~Config() {}

const std::vector<ServerConfig>& Config::getServers() const {
    return _servers;
}

// Trim whitespace from both ends
std::string Config::_trim(const std::string& str) {
    size_t start = str.find_first_not_of(" \t\r\n");
    if (start == std::string::npos) return "";
    size_t end = str.find_last_not_of(" \t\r\n");
    return str.substr(start, end - start + 1);
}

// Tokenize a line: "listen 8080;" -> ["listen", "8080"]
std::vector<std::string> Config::_tokenize(const std::string& line) {
    std::vector<std::string> tokens;
    std::string token;
    std::istringstream iss(line);
    
    while (iss >> token) {
        // Remove semicolon if present
        if (!token.empty() && token[token.length() - 1] == ';') {
            token = token.substr(0, token.length() - 1);
        }
        if (!token.empty()) {
            tokens.push_back(token);
        }
    }
    
    return tokens;
}

// Parse size strings: "10M" -> 10485760
size_t Config::_parseSize(const std::string& sizeStr) {
    if (sizeStr.empty()) return 0;
    
    size_t value = std::atoi(sizeStr.c_str());
    char unit = sizeStr[sizeStr.length() - 1];
    
    if (unit == 'K' || unit == 'k') {
        return value * 1024;
    } else if (unit == 'M' || unit == 'm') {
        return value * 1024 * 1024;
    } else if (unit == 'G' || unit == 'g') {
        return value * 1024 * 1024 * 1024;
    }
    
    return value;  // No unit, assume bytes
}

// Parse a location block
bool Config::_parseLocationBlock(std::ifstream& file, LocationConfig& location) {
    std::string line;
    
    while (std::getline(file, line)) {
        line = _trim(line);
        
        // Skip empty lines and comments
        if (line.empty() || line[0] == '#') {
            continue;
        }
        
        // End of location block
        if (line == "}") {
            return true;
        }
        
        std::vector<std::string> tokens = _tokenize(line);
        if (tokens.empty()) continue;
        
        // Parse directives
        if (tokens[0] == "allowed_methods") {
            // allowed_methods GET POST DELETE;
            for (size_t i = 1; i < tokens.size(); i++) {
                location.allowedMethods.push_back(tokens[i]);
            }
        }
        else if (tokens[0] == "root" && tokens.size() >= 2) {
            location.root = tokens[1];
        }
        else if (tokens[0] == "autoindex" && tokens.size() >= 2) {
            location.autoindex = (tokens[1] == "on");
        }
        else if (tokens[0] == "return" && tokens.size() >= 3) {
            // return 301 /new-page;
            location.redirect = tokens[2];
        }
        else if (tokens[0] == "cgi_pass" && tokens.size() >= 3) {
            // cgi_pass .php /usr/bin/php-cgi;
            location.cgiPass[tokens[1]] = tokens[2];
        }
        else if (tokens[0] == "client_max_body_size" && tokens.size() >= 2) {
            location.clientMaxBodySize = _parseSize(tokens[1]);
        }
        else if (tokens[0] == "index") {
            for (size_t i = 1; i < tokens.size(); i++) {
                location.index.push_back(tokens[i]);
            }
        }
        else if (tokens[0] == "requires_auth" && tokens.size() >= 2)
        {
            location.requireAuth = (tokens[1] == "on");
        }
    }
    
    return false;  // Unexpected end of file
}

// Parse a server block
bool Config::_parseServerBlock(std::ifstream& file, ServerConfig& server) {
    std::string line;
    
    while (std::getline(file, line)) {
        line = _trim(line);
        
        // Skip empty lines and comments
        if (line.empty() || line[0] == '#') {
            continue;
        }
        
        // End of server block
        if (line == "}") {
            return true;
        }
        
        std::vector<std::string> tokens = _tokenize(line);
        if (tokens.empty()) continue;
        
        // Parse directives
        if (tokens[0] == "listen" && tokens.size() >= 2) {
            server.port = std::atoi(tokens[1].c_str());
        }
        else if (tokens[0] == "server_name" && tokens.size() >= 2) {
            server.serverName = tokens[1];
        }
        else if (tokens[0] == "root" && tokens.size() >= 2) {
            server.root = tokens[1];
        }
        else if (tokens[0] == "index") {
            // index index.html index.htm default.html;
            for (size_t i = 1; i < tokens.size(); i++) {
                server.index.push_back(tokens[i]);
            }
        }
        else if (tokens[0] == "client_max_body_size" && tokens.size() >= 2) {
            server.clientMaxBodySize = _parseSize(tokens[1]);
        }
        else if (tokens[0] == "error_page" && tokens.size() >= 3) {
            // error_page 404 /errors/404.html;
            int code = std::atoi(tokens[1].c_str());
            server.errorPages[code] = tokens[2];
        }
        else if (tokens[0] == "location" && tokens.size() >= 2) {
            // location /uploads {
            LocationConfig location;
            location.path = tokens[1];
            
            // Check if opening brace is on same line or next
            if (line.find('{') == std::string::npos) {
                // Brace on next line
                while (std::getline(file, line)) {
                    line = _trim(line);
                    if (line == "{") break;
                }
            }
            
            if (_parseLocationBlock(file, location)) {
                server.locations.push_back(location);
            } else {
                logError("Failed to parse location block: " + location.path);
                return false;
            }
        }
    }
    
    return false;  // Unexpected end of file
}

// Main parse function
bool Config::parse(const std::string& filename) {
    std::ifstream file(filename.c_str());
    
    if (!file.is_open()) {
        logError("Failed to open config file: " + filename);
        return false;
    }
    
    std::string line;
    
    while (std::getline(file, line)) {
        line = _trim(line);
        
        // Skip empty lines and comments
        if (line.empty() || line[0] == '#') {
            continue;
        }
        
        std::vector<std::string> tokens = _tokenize(line);
        if (tokens.empty()) continue;
        
        if (tokens[0] == "server") {
            ServerConfig server;
            
            // Check if opening brace is on same line or next
            if (line.find('{') == std::string::npos) {
                // Brace on next line
                while (std::getline(file, line)) {
                    line = _trim(line);
                    if (line == "{") break;
                }
            }
            
            if (_parseServerBlock(file, server)) {
                _servers.push_back(server);
                logMessage("Parsed server on port " + intToString(server.port));
            } else {
                logError("Failed to parse server block");
                file.close();
                return false;
            }
        }
    }
    
    file.close();
    
    if (_servers.empty()) {
        logError("No servers found in config file");
        return false;
    }
    
    logMessage("Successfully parsed " + intToString(_servers.size()) + " server(s)");
    return true;
}

void Config::print() const {
    std::cout << "========== Parsed Configuration ==========" << std::endl;

    for (size_t i = 0; i < _servers.size(); ++i) {
        const ServerConfig& server = _servers[i];

        std::cout << "\n--- Server " << i + 1 << " ---" << std::endl;
        std::cout << "Port: " << server.port << std::endl;
        std::cout << "Server Name: " << server.serverName << std::endl;
        std::cout << "Root: " << server.root << std::endl;
        std::cout << "Client Max Body Size: " 
                  << server.clientMaxBodySize << " bytes" << std::endl;

        // Index files
        std::cout << "Index files: ";
        if (server.index.empty())
            std::cout << "(none)";
        else {
            for (size_t j = 0; j < server.index.size(); ++j)
                std::cout << server.index[j] << " ";
        }
        std::cout << std::endl;

        // Error pages
        std::cout << "Error pages:" << std::endl;
        if (server.errorPages.empty()) {
            std::cout << "  (none)" << std::endl;
        } else {
            for (std::map<int, std::string>::const_iterator it = server.errorPages.begin();
                 it != server.errorPages.end(); ++it) {
                std::cout << "  " << it->first << " => " << it->second << std::endl;
            }
        }

        // Locations
        std::cout << "Locations (" << server.locations.size() << "):" << std::endl;
        for (size_t j = 0; j < server.locations.size(); ++j) {
            const LocationConfig& loc = server.locations[j];

            std::cout << "  Location " << j + 1 << ": " << loc.path << std::endl;

            // Allowed methods
            std::cout << "    Methods: ";
            if (loc.allowedMethods.empty())
                std::cout << "(none)";
            else {
                for (size_t k = 0; k < loc.allowedMethods.size(); ++k)
                    std::cout << loc.allowedMethods[k] << " ";
            }
            std::cout << std::endl;

            // Location root
            if (!loc.root.empty())
                std::cout << "    Root: " << loc.root << std::endl;

            // Index files
            std::cout << "    Index files: ";
            if (loc.index.empty())
                std::cout << "(none)";
            else {
                for (size_t k = 0; k < loc.index.size(); ++k)
                    std::cout << loc.index[k] << " ";
            }
            std::cout << std::endl;

            // Autoindex
            std::cout << "    Autoindex: " 
                      << (loc.autoindex ? "on" : "off") << std::endl;

            // Redirect
            if (!loc.redirect.empty())
                std::cout << "    Redirect: " << loc.redirect << std::endl;

            // CGI
            std::cout << "    CGI:" << std::endl;
            if (loc.cgiPass.empty()) {
                std::cout << "      (none)" << std::endl;
            } else {
                for (std::map<std::string, std::string>::const_iterator it = loc.cgiPass.begin();
                     it != loc.cgiPass.end(); ++it) {
                    std::cout << "      " << it->first 
                              << " => " << it->second << std::endl;
                }
            }

            // Body size override
            if (loc.clientMaxBodySize > 0)
                std::cout << "    Client Max Body Size: "
                          << loc.clientMaxBodySize << " bytes" << std::endl;
        }
    }

    std::cout << "\n========== End Configuration ==========" << std::endl;
}

bool Config::validate() const {
    for (size_t i = 0; i < _servers.size(); i++) {
        const ServerConfig& server = _servers[i];
        
        // Check port is valid
        if (server.port <= 0 || server.port > 65535) {
            logError("Invalid port: " + intToString(server.port));
            return false;
        }
        
        // Check root exists
        if (!isDirectory(server.root)) {
            logError("Root directory does not exist: " + server.root);
            return false;
        }
        
        // Check index files
        if (server.index.empty()) {
            logError("No index files specified for server on port " + intToString(server.port));
            return false;
        }
        
        // Validate locations
        for (size_t j = 0; j < server.locations.size(); j++) {
            const LocationConfig& loc = server.locations[j];
            
            // Check path starts with /
            if (loc.path.empty() || loc.path[0] != '/') {
                logError("Location path must start with /: " + loc.path);
                return false;
            }
            
            // Check methods are valid
            for (size_t k = 0; k < loc.allowedMethods.size(); k++) {
                std::string method = loc.allowedMethods[k];
                if (method != "GET" && method != "POST" && method != "DELETE") {
                    logError("Invalid method: " + method);
                    return false;
                }
            }
        }
    }
    
    return true;
}