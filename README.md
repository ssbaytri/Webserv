# Webserv

*This project has been created as part of the 42 curriculum by ssbaytri*

---

## Description

**Webserv** is an HTTP/1.1-compliant web server written in C++ 98.  
It is inspired by NGINX and implements the core features required to serve static websites, handle file uploads, enforce per-route access control, and execute CGI scripts — all driven by a single non-blocking I/O loop powered by `poll()`.

**Goals:**
- Understand the HTTP protocol (request/response cycle, status codes, headers, body handling)
- Build a non-blocking, event-driven server that never hangs or crashes
- Parse and apply a flexible NGINX-style configuration file
- Support GET, POST, and DELETE HTTP methods
- Execute CGI scripts (Python, PHP) for dynamic content

---

## Instructions

### Requirements

- A C++ 98–compliant compiler (e.g. `g++` or `clang++`)
- Linux or macOS

### Compilation

```bash
make
```

This produces the `./webserv` binary.  
Other Makefile targets:

| Target  | Action                          |
|---------|---------------------------------|
| `all`   | Compile the project (default)   |
| `clean` | Remove object files             |
| `fclean`| Remove object files + binary    |
| `re`    | Full recompilation from scratch |

### Running

```bash
./webserv [configuration_file]
```

**Example:**

```bash
./webserv conf/file.conf
```

The server will start listening on every port defined in the configuration file.  
Open your browser and navigate to `http://localhost:8080` (or whatever port you configured).

### Configuration File Format

The configuration file follows an NGINX-inspired syntax:

```nginx
server {
    listen       8080;
    server_name  localhost;
    root         ./www;
    index        index.html;
    client_max_body_size 5M;

    error_page 404 /errors/404.html;

    location / {
        allowed_methods GET;
        autoindex off;
    }

    location /uploads {
        allowed_methods GET POST DELETE;
        autoindex on;
        client_max_body_size 500M;
    }

    location /cgi_bin {
        allowed_methods GET POST;
        root ./www/cgi_bin;
        cgi_pass .py  /usr/bin/python3;
        cgi_pass .php /usr/bin/php-cgi;
    }

    location /old-page {
        return 301 /new-page;
    }
}
```

Supported directives:

| Directive               | Level              | Description                                      |
|-------------------------|--------------------|--------------------------------------------------|
| `listen`                | server             | Port to listen on                                |
| `server_name`           | server             | Virtual host name                                |
| `root`                  | server / location  | Document root directory                          |
| `index`                 | server / location  | Default index file(s)                            |
| `client_max_body_size`  | server / location  | Maximum request body size (e.g. `10M`, `500K`)   |
| `error_page`            | server             | Custom error page per status code                |
| `allowed_methods`       | location           | Whitelist of allowed HTTP methods                |
| `autoindex`             | location           | Enable/disable directory listing (`on`/`off`)    |
| `return`                | location           | HTTP redirect (`return 301 /new-url`)            |
| `cgi_pass`              | location           | Map file extension to CGI executor               |

---

## Resources

### References

- [RFC 7230 – HTTP/1.1 Message Syntax and Routing](https://datatracker.ietf.org/doc/html/rfc7230)
- [RFC 7231 – HTTP/1.1 Semantics and Content](https://datatracker.ietf.org/doc/html/rfc7231)
- [NGINX Documentation](https://nginx.org/en/docs/)
- [Beej's Guide to Network Programming](https://beej.us/guide/bgnet/)
- [MDN Web Docs – HTTP](https://developer.mozilla.org/en-US/docs/Web/HTTP)
- [CGI/1.1 Specification (RFC 3875)](https://datatracker.ietf.org/doc/html/rfc3875)

### AI Usage

AI tools (GitHub Copilot / ChatGPT) were used for the following tasks during this project:

- **Understanding HTTP internals**: asking for clarifications on RFC sections (chunked encoding, multipart boundaries, status code semantics).
- **Debugging ideas**: generating hypotheses for why specific edge cases (partial reads, malformed headers) were failing, which we then verified manually.
- **Boilerplate structure**: initial scaffolding for the `poll()` event loop and the config parser, then carefully reviewed and rewritten to match our design.
- **Documentation**: drafting sections of this README, reviewed and edited to be accurate.

All AI-generated content was reviewed, tested, and validated before inclusion. No code was used that we could not fully explain during peer evaluation.

---

## Project Status

> A detailed breakdown of what is **implemented** and what remains **to do**, evaluated against the mandatory requirements of the subject.

### ✅ Done

| Feature | Details |
|---|---|
| **Makefile** | All required rules (`all`, `clean`, `fclean`, `re`); compiles with `-Wall -Wextra -Werror -std=c++98` |
| **Configuration file parser** | Parses `server` and `location` blocks; supports all required directives |
| **Config validation** | Validates port range, root directory existence, method names |
| **Multiple ports / servers** | Server listens on all ports defined in config simultaneously |
| **Non-blocking I/O with `poll()`** | Single `poll()` loop drives all accept / read / write operations |
| **Non-blocking sockets** | All sockets set to `O_NONBLOCK` via `fcntl()` |
| **GET method** | Serves static files with correct MIME types; resolves index files |
| **POST method** | Handles `multipart/form-data` file uploads; respects `client_max_body_size` |
| **DELETE method** | Deletes files; returns 204 on success, 404 if not found |
| **HTTP redirects** | `return 301 <url>` in location blocks works correctly |
| **Directory listing (autoindex)** | Styled HTML listing with file names, sizes, and dates |
| **Method access control** | Per-location `allowed_methods` enforcement (405 on violation) |
| **Default error pages** | Built-in HTML pages for 400, 403, 404, 405, 413, 500, 501; custom pages via config |
| **Request parser** | Parses request line, headers, and body (including multipart) |
| **Response builder** | Builds valid HTTP/1.1 responses with status line, headers, body |
| **Client state machine** | Tracks READING_REQUEST → SENDING_RESPONSE → DONE lifecycle |
| **Static website** | `./www` contains a working multi-page static site |
| **Location root override** | Locations can override the server-level root |
| **CGI routing** | GET and POST requests to CGI locations are routed to `_handleCGI()` |
| **Path safety** | Rejects paths containing `..`, null bytes, or double slashes |

---

### ❌ Not Yet Done / TODO

| Feature | Status | Notes |
|---|---|---|
| **CGI execution** | ❌ Stub only | `_handleCGI()` exists but contains only a `TODO` comment. Needs `fork()` + `execve()`, environment variable setup (`QUERY_STRING`, `CONTENT_TYPE`, `PATH_INFO`, etc.), and pipe-based I/O |
| **Chunked Transfer-Encoding** | ✅ Done | Server now detects and decodes chunked request bodies in `Client::isRequestComplete()` and `Request::_parseChunkedBody()`. Bodies are un-chunked before being passed to request handlers. |
| **keep-alive / persistent connections** | ❌ Missing | Server closes every connection after a response. HTTP/1.1 keep-alive is not handled |
| **Client timeout enforcement** | ❌ Defined, never called | `Client::isTimedOut()` exists but is never invoked in the main `run()` loop |
| **Custom 404/500 error pages** | ⚠️ Partial | config supports them, but `www/errors/` only has `405.html` and `501.html`; `404.html` and `500.html` are missing |
| **Signal handling (SIGINT / SIGTERM)** | ❌ Missing | No graceful shutdown on Ctrl-C; resources may not be freed cleanly |
| **README.md** | ✅ Done (this file) | Was a single line before; now complete with all subject-required sections |
| **Bonus: cookies & session management** | ❌ Not started | Requires implementing `Set-Cookie` / `Cookie` header handling |
| **Bonus: multiple CGI types** | ❌ Blocked by base CGI | Depends on CGI execution being implemented first |
| **Test scripts** | ❌ Missing | Subject recommends writing test scripts in Python / Go / C to validate all features |

---

### Summary

The **core infrastructure** of the server is solid: non-blocking I/O, config parsing, HTTP request/response handling, static file serving, file uploads, and directory listings all work.  

The **most critical missing piece** is the CGI execution engine — without it the server cannot execute dynamic scripts, which is a mandatory requirement.  

After CGI, the next priorities are: chunked-encoding support, connection timeouts, keep-alive, graceful signal handling, and completing the error-page assets.