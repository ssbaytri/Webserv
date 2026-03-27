#ifndef CGIHANDLER_HPP
#define CGIHANDLER_HPP

#include <string>
#include <map>
#include <sys/types.h>
#include "Request.hpp"

class CgiHandler {
private:
    std::map<std::string, std::string> _envMap;
    char** _envp;

    int _inPipeFd[2];
    int _outPipeFd[2];
    pid_t _cgiPid;

    void _initEnv(const Request& request, const std::string& scriptPath);

public:
    CgiHandler(const Request& request, const std::string& scriptPath);
    ~CgiHandler();

    char** getEnvp() const;
    void   clearEnvp();

    // CGI Execution
    int    setupIO(const std::string& body);
    int    executeCgi(const std::string& scriptPath, const std::string& cgiExecutor);
    int    getOutputFd() const;
    pid_t  getPid() const;
};

#endif