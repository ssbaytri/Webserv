*This project has been created as part of the 42 curriculum by ssbaytri, aychikhi, ayaarab*

# Webserv

## Description

**Webserv** is a fully functional, HTTP/1.1-compliant web server written entirely in C++98. It is inspired by NGINX and implements the core features required to serve static websites, handle file uploads, enforce per-route access control, and execute CGI scripts. The entire server is driven by a single-threaded, non-blocking I/O event loop using multiplexing (`poll`/`select`/`epoll`/`kqueue`), ensuring that it never hangs or crashes under load.

**Goals of the project:**
- Deeply understand the HTTP protocol (request/response cycle, status codes, headers, chunked transfer encoding, and body handling).
- Build a resilient, non-blocking, event-driven server capable of handling multiple concurrent clients without multi-threading.
- Parse and apply a flexible, NGINX-style configuration file.
- Support core HTTP methods: `GET`, `POST`, and `DELETE`.
- Execute CGI scripts (e.g., Python, PHP) for dynamic content generation.

---

## Instructions

### Requirements
- A strictly C++98-compliant compiler (e.g., `c++`, `g++`, or `clang++`).
- A Unix-like operating system (Linux or macOS).
- `make` utility.

### Compilation
To compile the project, simply run `make` at the root of the repository:

```bash
make
```

This produces the `./webserv` executable binary. 

**Available Makefile targets:**
- `make all`: Compiles the project (default).
- `make clean`: Removes the generated object files.
- `make fclean`: Removes the object files and the executable binary.
- `make re`: Performs a full recompilation from scratch.

### Execution
Run the server by providing a configuration file as an argument. If no file is provided, it may fall back to a default configuration.

```bash
./webserv [configuration_file]
```

**Example:**
```bash
./webserv conf/default.conf
```

The server will parse the configuration file, bind to the specified ports, and begin listening for incoming connections. You can then open a web browser or use `curl` to navigate to `http://localhost:<port>`.

---

## Resources

### References
- [RFC 7230 – HTTP/1.1 Message Syntax and Routing](https://datatracker.ietf.org/doc/html/rfc7230)
- [RFC 7231 – HTTP/1.1 Semantics and Content](https://datatracker.ietf.org/doc/html/rfc7231)
- [RFC 3875 – The Common Gateway Interface (CGI) Version 1.1](https://datatracker.ietf.org/doc/html/rfc3875)
- [NGINX Official Documentation](https://nginx.org/en/docs/)
- [Beej's Guide to Network Programming](https://beej.us/guide/bgnet/)
- [MDN Web Docs – HTTP](https://developer.mozilla.org/en-US/docs/Web/HTTP)

### AI Usage
During the development of this project, AI tools (such as GitHub Copilot and ChatGPT) were used to assist with the following specific tasks:
- **Understanding HTTP Internals**: Querying AI for clarifications on dense RFC sections, specifically regarding chunked transfer encoding, multipart boundaries, and exact status code semantics.
- **Debugging Hypotheses**: Generating ideas for why specific edge cases (such as partial reads or malformed headers) were failing, which were then manually verified and patched.
- **Boilerplate Generation**: Creating initial scaffolding for the `poll()` event loop and string manipulation utilities for the configuration parser, which were subsequently reviewed, heavily modified, and integrated into our specific C++98 architecture.
- **Frontend & Assets**: Generating HTML/CSS templates (with glassmorphism styling) to test our server's static file delivery, file uploads, and directory listing functionalities.

*Note: All AI-generated suggestions were strictly reviewed, manually tested, and validated to ensure complete understanding of the underlying concepts before inclusion in the final codebase. No code was included that the team cannot fully explain.*