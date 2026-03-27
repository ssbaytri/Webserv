#include "CgiHandler.hpp"
#include <sstream>
#include <cstdlib>
#include <cstring>
#include <iostream>
#include <unistd.h>
#include <fcntl.h>
#include <cstdio>

CgiHandler::CgiHandler(const Request& request, const std::string& scriptPath) 
    : _envp(NULL), _cgiPid(-1) {
    _inPipeFd[0] = -1;
    _inPipeFd[1] = -1;
    _outPipeFd[0] = -1;
    _outPipeFd[1] = -1;
    _initEnv(request, scriptPath);
}

CgiHandler::~CgiHandler() {
    clearEnvp();
    if (_inPipeFd[0] != -1) close(_inPipeFd[0]);
    if (_inPipeFd[1] != -1) close(_inPipeFd[1]);
    if (_outPipeFd[0] != -1) close(_outPipeFd[0]);
    if (_outPipeFd[1] != -1) close(_outPipeFd[1]);
}

int CgiHandler::setupIO(const std::string& body) {
    // 1. Create a pipe for STDIN (Request Body sent to CGI)
    if (pipe(_inPipeFd) == -1) {
        std::cerr << "CGI Error: Failed to create input pipe" << std::endl;
        return -1;
    }

    // Write the body to the input pipe
    if (!body.empty()) {
        // NOTE: Large bodies might block here since normal pipe buffers are 64KB.
        // For a robust implementation, writing to the pipe should happen asynchronously
        // or the pipe size needs to be managed carefully. For now, we write directly.
        ssize_t written = write(_inPipeFd[1], body.c_str(), body.length());
        if (written == -1 || (size_t)written != body.length()) {
            std::cerr << "CGI Error: Failed to write body to input pipe" << std::endl;
            return -1;
        }
    }

    // 2. Create a pipe for STDOUT (CGI Response Output)
    if (pipe(_outPipeFd) == -1) {
        std::cerr << "CGI Error: Failed to create output pipe" << std::endl;
        return -1;
    }

    return 0; // Success
}

int CgiHandler::executeCgi(const std::string& scriptPath, const std::string& cgiExecutor) {
    _cgiPid = fork();
    if (_cgiPid == -1) {
        std::cerr << "CGI Error: Failed to fork process" << std::endl;
        return -1;
    }

    if (_cgiPid == 0) {
        // ---- CHILD PROCESS ----
        
        // Redirect STDIN from input pipe
        dup2(_inPipeFd[0], STDIN_FILENO);
        close(_inPipeFd[0]);
        close(_inPipeFd[1]); // Close write end in child
        
        // Redirect STDOUT to output pipe
        dup2(_outPipeFd[1], STDOUT_FILENO);
        close(_outPipeFd[0]); // Close read end in child
        close(_outPipeFd[1]);

        char* argv[] = {
            const_cast<char*>(cgiExecutor.c_str()),
            const_cast<char*>(scriptPath.c_str()),
            NULL
        };

        // Execute script
        execve(cgiExecutor.c_str(), argv, _envp);
        
        // If execve fails
        std::cerr << "CGI Error: execve failed" << std::endl;
        exit(1);
    } else {
        // ---- PARENT PROCESS ----
        // Close unused pipe ends
        close(_inPipeFd[0]);
        close(_inPipeFd[1]); // We already wrote to it in setupIO
        _inPipeFd[0] = -1;
        _inPipeFd[1] = -1;

        close(_outPipeFd[1]); // Close write end of output pipe
        _outPipeFd[1] = -1;
    }

    return 0; // Success
}

int CgiHandler::getOutputFd() const { return _outPipeFd[0]; }
pid_t CgiHandler::getPid() const { return _cgiPid; }

void CgiHandler::_initEnv(const Request& request, const std::string& scriptPath) {
    // Basic CGI variables
    _envMap["GATEWAY_INTERFACE"] = "CGI/1.1";
    _envMap["SERVER_PROTOCOL"] = "HTTP/1.1";
    _envMap["SERVER_SOFTWARE"] = "webserv/1.0";
    
    // Request specific variables
    _envMap["REQUEST_METHOD"] = request.getMethod();
    _envMap["QUERY_STRING"] = request.getQueryString();

    if (request.getContentLength() > 0) {
        std::stringstream ss;
        ss << request.getContentLength();
        _envMap["CONTENT_LENGTH"] = ss.str();
    }
    
    if (!request.getContentType().empty()) {
        _envMap["CONTENT_TYPE"] = request.getContentType();
    }

    // Mapping to script
    _envMap["SCRIPT_FILENAME"] = scriptPath;
    _envMap["REDIRECT_STATUS"] = "200"; // Required by PHP-CGI specifically
    
    // Allocate the char** array correctly for execve
    _envp = new char*[_envMap.size() + 1];
    int i = 0;
    for (std::map<std::string, std::string>::const_iterator it = _envMap.begin(); it != _envMap.end(); ++it) {
        std::string envString = it->first + "=" + it->second;
        _envp[i] = new char[envString.length() + 1];
        std::strcpy(_envp[i], envString.c_str());
        i++;
    }
    _envp[i] = NULL;
}

char** CgiHandler::getEnvp() const {
    return _envp;
}

void CgiHandler::clearEnvp() {
    if (_envp) {
        for (int i = 0; _envp[i]; ++i) {
            delete[] _envp[i];
        }
        delete[] _envp;
        _envp = NULL;
    }
}