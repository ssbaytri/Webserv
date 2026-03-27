#include "../includes/CgiHandler.hpp"
#include "../includes/Request.hpp"
#include <sstream>
#include <cstdlib>
#include <cstring>
#include <iostream>
#include <unistd.h>
#include <fcntl.h>
#include <cstdio>

CgiHandler::CgiHandler(const Request& request, const std::string& scriptPath) : _envp(NULL), _cgiPid(-1)
{
    _inPipeFd[0] = -1;
    _inPipeFd[1] = -1;
    _outPipeFd[0] = -1;
    _outPipeFd[1] = -1;
    _initEnv(request, scriptPath);
}

CgiHandler::~CgiHandler()
{
    clearEnvp();
    if (_inPipeFd[0] != -1) close(_inPipeFd[0]);
    if (_inPipeFd[1] != -1) close(_inPipeFd[1]);
    if (_outPipeFd[0] != -1) close(_outPipeFd[0]);
    if (_outPipeFd[1] != -1) close(_outPipeFd[1]);
}

int CgiHandler::setupIO(const std::string& body)
{
    if (pipe(_inPipeFd) == -1) {
        std::cerr << "CGI Error: Failed to create input pipe" << std::endl;
        return -1;
    }

    if (pipe(_outPipeFd) == -1) {
        std::cerr << "CGI Error: Failed to create output pipe" << std::endl;
        return -1;
    }

    _body = body;

    return 0;
}

int CgiHandler::executeCgi(const std::string& scriptPath, const std::string& cgiExecutor)
{
    _cgiPid = fork();
    if (_cgiPid == -1)
    {
        std::cerr << "CGI Error: Failed to fork process" << std::endl;
        return -1;
    }

    // Child Process
    if (_cgiPid == 0)
    {
        std::string scriptDir = scriptPath.substr(0, scriptPath.rfind('/'));
        if (!scriptDir.empty())
            chdir(scriptDir.c_str());

        // Redirect STDIN from input pipe
        dup2(_inPipeFd[0], STDIN_FILENO);
        close(_inPipeFd[0]);
        close(_inPipeFd[1]);

        // Redirect STDOUT to output pipe
        dup2(_outPipeFd[1], STDOUT_FILENO);
        close(_outPipeFd[0]);
        close(_outPipeFd[1]);

        char *argv[] = 
        {
            const_cast<char*>(cgiExecutor.c_str()),
            const_cast<char*>(scriptPath.c_str()),
            NULL
        };

        execve(cgiExecutor.c_str(), argv, _envp);

        std::cerr << "CGI Error: execve failed" << std::endl;
        exit(1);
    }

    // Parent process

    close(_inPipeFd[0]);
    _inPipeFd[0] = -1;

    close(_outPipeFd[1]);
    _outPipeFd[1] = -1;

    if (!_body.empty())
    {
        ssize_t written = write(_inPipeFd[1], _body.c_str(), _body.length());
        if (written < 0)
            std::cerr << "CGI Error: Failed to write body to stdin pipe" << std::endl;
    }

    close(_inPipeFd[1]);
    _inPipeFd[1] = -1;

    return (0);
}

int CgiHandler::getOutputFd() const { return _outPipeFd[0]; }
pid_t CgiHandler::getPid() const { return _cgiPid; }

void CgiHandler::_initEnv(const Request& request, const std::string& scriptPath)
{
    _envMap["GATEWAY_INTERFACE"] = "CGI/1.1";
    _envMap["SERVER_PROTOCOL"] = "HTTP/1.1";
    _envMap["SERVER_SOFTWARE"] = "webserv/1.0";

    _envMap["REQUEST_METHOD"] = request.getMethod();
    _envMap["QUERY_STRING"] = request.getQueryString();

    if (request.getContentLength() > 0) {
        std::stringstream ss;
        ss << request.getContentLength();
        _envMap["CONTENT_LENGTH"] = ss.str();
    }

    if (!request.getContentType().empty())
        _envMap["CONTENT_TYPE"] = request.getContentType();

    _envMap["SCRIPT_FILENAME"] = scriptPath;
    _envMap["REDIRECT_STATUS"] = "200";

    _envp = new char*[_envMap.size() + 1];
    int i = 0;
    for (std::map<std::string, std::string>::const_iterator it = _envMap.begin();
         it != _envMap.end(); ++it)
    {
        std::string envString = it->first + "=" + it->second;
        _envp[i] = new char[envString.length() + 1];
        std::strcpy(_envp[i], envString.c_str());
        i++;
    }
    _envp[i] = NULL;
}

char** CgiHandler::getEnvp() const
{
    return _envp;
}

void CgiHandler::clearEnvp()
{
    if (_envp) {
        for (int i = 0; _envp[i]; ++i) {
            delete[] _envp[i];
        }
        delete[] _envp;
        _envp = NULL;
    }
}