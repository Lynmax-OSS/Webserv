*This project has been created as part of the 42 curriculum by keteo, qrajendr and yosherau.*

# webserv

A HTTP/1.1 web server written in C++98, built from scratch as part of the 42 curriculum.

## Description

webserv is a functional HTTP server that accepts connections from standard web browsers, parses HTTP/1.1 requests, serves static files, handles file uploads and deletions, and executes CGI scripts. The server is non-blocking and event-driven: The server uses a single event loop with poll() to monitor I/O operations across multiple file descriptors. Meaning, many clients can be served concurrently without threads or forked processes per connection.

The goal of the project is to understand HTTP from the inside - how a request travels from the browser, through parsing, routing, and file I/O, and back out as a response.

### Feature overview

- HTTP/1.1 requests with `GET`, `POST`, and `DELETE` methods
- Static file serving with correct MIME types
- File uploads (`POST`) to a configurable directory
- File deletion (`DELETE`)
- Directory listing (`autoindex`) with on/off per location
- Custom error pages (403, 404, 500)
- HTTP redirects (`return` directive in config)
- Chunked transfer encoding for request bodies
- CGI execution based on file extension (Python)
- Keep-alive connections and request pipelining
- Multiple listening ports
- Nginx-style configuration file

## Instructions

### Compilation

```bash
make
```

Compiles all source files with `-Wall -Wextra -Werror -std=c++98` and produces a binary named `webserv`.

### Execution

```bash
./webserv [configuration file]
```

If no configuration file is given, the server attempts to load `configs/default.conf`.

Example:

```bash
./webserv configs/default.conf
```

Then open `http://localhost:8080` in a browser (or whichever port is configured).

### Cleanup

```bash
make clean     # remove object files
make fclean    # remove object files and binary
make re        # rebuild from scratch
```

### Prerequisites

- A Unix-like OS (Linux or macOS)
- A C++ compiler supporting C++98
- Python 3 (only needed for CGI scripts)

### Directory setup

The server expects certain directories to exist:

- `www/` - static site files
- `www/errors/` - custom error pages
- `uploads/` - POST upload destination

If `uploads/` is missing on a fresh clone, create it once:

```bash
mkdir -p uploads
```

## Usage

### Testing with a browser

Open `http://localhost:8080`. The default homepage has three forms for testing GET, POST upload, and DELETE.

### Testing with curl

```bash
# GET a static file
curl http://localhost:8080/index.html

# POST a file
curl -X POST -d "hello" http://localhost:8080/upload/test.txt

# DELETE a file
curl -X DELETE http://localhost:8080/upload/test.txt

# Chunked POST
curl -H "Transfer-Encoding: chunked" -d "hello" http://localhost:8080/upload/ch.txt

# Custom error page
curl http://localhost:8080/does-not-exist
```

### Testing with raw HTTP (telnet / nc)

```bash
printf 'GET / HTTP/1.1\r\nHost: localhost:8080\r\n\r\n' | nc localhost 8080
```

## How it works (brief)

The server is built around a single event loop. These eight objects define the architecture:

| # | Object | Lives in | Role |
|---|--------|----------|------|
| 1 | `ServerConfig` | `main()` stack, in a vector | Settings for one server block |
| 2 | `ServerSocket` | heap, owned by `SocketManager` | Listening socket for one port |
| 3 | `SocketManager` | `main()` stack | Holds all `ServerSocket` objects |
| 4 | `pollfd` | inside `PollManager::_poll_fds` | "Watch this fd for these events" |
| 5 | `PollManager` | `main()` stack | Owns the event loop, `_poll_fds`, `_clients` |
| 6 | `ClientConnection` | inside `_clients` map (by value) | Everything about one client |
| 7 | `HttpParser` | inside `ClientConnection` | State machine that builds an `HttpRequest` |
| 8 | `HttpRequest` | inside `HttpParser` | The finished, parsed request |

Flow of one HTTP request:

1. `poll()` reports which fds are ready.
2. Listening socket ready → `accept()` creates a new client fd.
3. Client socket ready → `read()` bytes into that client's parser.
4. Parser transitions through `REQUEST_LINE` → `HEADERS` → `BODY` (or chunked states) → `COMPLETE`.
5. `processRequest()` calls `handleRequest()` with the parsed request.
6. Response is written into the client's write buffer.
7. `POLLOUT` fires → `write()` sends the response.
8. Parser is reset. Connection stays open if keep-alive, otherwise closed.

The entire server runs inside one `while (true)` loop in `PollManager::run()`. Normal client connections are handled by the main event loop. CGI execution creates a child process using `fork()`.

## Project structure

```
webserv/
├── Makefile
├── configs/
│   └── default.conf
├── include/
│   ├── ConfigHeader/
│   ├── ParserHeader/
│   ├── NetworkHeader/
│   └── NetworkUtilsHeader/
├── src/
│   ├── main.cpp
│   ├── ConfigSrc/          # configuration parsing
│   ├── ParserSrc/          # HTTP request parsing
│   ├── NetworkSrc/         # sockets and poll event loop
│   └── NetworkUtilsSrc/    # response building, CGI
├── www/
│   ├── index.html
│   └── errors/
└── uploads/
```

## Resources

### Classic references

- RFC 7230 - HTTP/1.1: Message Syntax and Routing
- RFC 7231 - HTTP/1.1: Semantics and Content
- RFC 3875 - The Common Gateway Interface (CGI) Version 1.1
- NGINX documentation - https://nginx.org/en/docs/ (used as a reference for config syntax and HTTP behaviour)
- Beej's Guide to Network Programming - https://beej.us/guide/bgnet/
- Linux man pages: `poll(2)`, `socket(2)`, `socket(7)`, `fcntl(2)`, `read(2)`, `write(2)`, `execve(2)`

### AI usage

AI (Claude) was used extensively as a learning and debugging aid during this project:

- **Architecture and code walkthrough** - the request lifecycle (accept → read → parse → handle → write → close) was explained step by step to build a shared mental model.
- **Bug diagnosis** - identify and fix several issues in the HTTP parser and poll event loop, including: header case-insensitivity (RFC 7230), `errno` misuse after `read`/`write` (forbidden by the subject), and a dangling reference after connection close.
- **Code review** - all ai generated codes were tested, and rewritten where necessary before being committed. Every team member can explain and defend the code they submit.
