#ifndef HTTP_SERVER_HPP
#define HTTP_SERVER_HPP

#ifndef _WIN32_WINNT
#define _WIN32_WINNT 0x0601
#endif

#include <winsock2.h>
#include <ws2tcpip.h>
#include <windows.h>

#include <string>
#include <vector>
#include <map>
#include <functional>
#include <sstream>
#include <iostream>
#include <fstream>
#include <algorithm>
#include <cctype>

#include "../json.hpp"

using json = nlohmann::json;

// -------------------------------------------------------------
// Request Object
// -------------------------------------------------------------
struct HttpRequest {
    std::string method;
    std::string path;
    std::string raw_query;
    std::map<std::string, std::string> query_params;
    std::map<std::string, std::string> headers;
    std::string body;
    std::string client_ip;

    std::string query(const std::string& key, const std::string& default_val = "") const {
        auto it = query_params.find(key);
        return it != query_params.end() ? it->second : default_val;
    }

    std::string header(const std::string& key) const {
        std::string lower_key = key;
        std::transform(lower_key.begin(), lower_key.end(), lower_key.begin(), ::tolower);
        auto it = headers.find(lower_key);
        return it != headers.end() ? it->second : "";
    }

    std::string bearer_token() const {
        std::string auth = header("authorization");
        if (auth.rfind("Bearer ", 0) == 0) {
            return auth.substr(7);
        }
        return "";
    }

    json json_body() const {
        try {
            if (!body.empty()) {
                return json::parse(body);
            }
        } catch (...) {
            // Invalid JSON
        }
        return json::object();
    }
};

// -------------------------------------------------------------
// Response Object
// -------------------------------------------------------------
struct HttpResponse {
    int status_code = 200;
    std::string status_text = "OK";
    std::map<std::string, std::string> headers;
    std::string body;

    void set_header(const std::string& key, const std::string& value) {
        headers[key] = value;
    }

    void json(const nlohmann::json& j, int code = 200) {
        status_code = code;
        status_text = (code == 200 ? "OK" : (code == 201 ? "Created" : (code == 400 ? "Bad Request" : (code == 401 ? "Unauthorized" : (code == 404 ? "Not Found" : "Error")))));
        set_header("Content-Type", "application/json; charset=utf-8");
        body = j.dump(2);
    }

    void text(const std::string& txt, int code = 200) {
        status_code = code;
        set_header("Content-Type", "text/plain; charset=utf-8");
        body = txt;
    }

    void error(int code, const std::string& msg) {
        nlohmann::json j;
        j["success"] = false;
        j["error"] = msg;
        json(j, code);
    }
};

// Handler function type
using HttpHandler = std::function<void(const HttpRequest&, HttpResponse&)>;

// -------------------------------------------------------------
// HTTP Server Implementation (Winsock2)
// -------------------------------------------------------------
class HttpServer {
public:
    HttpServer() : m_listen_socket(INVALID_SOCKET), m_running(false) {
        WSADATA wsaData;
        int res = WSAStartup(MAKEWORD(2, 2), &wsaData);
        if (res != 0) {
            std::cerr << "[Server Error] WSAStartup failed: " << res << std::endl;
        }
    }

    ~HttpServer() {
        stop();
        WSACleanup();
    }

    void get(const std::string& path, HttpHandler handler) {
        m_routes["GET"][path] = handler;
    }

    void post(const std::string& path, HttpHandler handler) {
        m_routes["POST"][path] = handler;
    }

    void put(const std::string& path, HttpHandler handler) {
        m_routes["PUT"][path] = handler;
    }

    void del(const std::string& path, HttpHandler handler) {
        m_routes["DELETE"][path] = handler;
    }

    void set_static_dir(const std::string& dir) {
        m_static_dir = dir;
    }

    bool listen(int port = 8080, const std::string& host = "127.0.0.1") {
        m_listen_socket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
        if (m_listen_socket == INVALID_SOCKET) {
            std::cerr << "[Server Error] Socket creation failed: " << WSAGetLastError() << std::endl;
            return false;
        }

        // Allow address reuse
        int opt = 1;
        setsockopt(m_listen_socket, SOL_SOCKET, SO_REUSEADDR, (const char*)&opt, sizeof(opt));

        sockaddr_in server_addr;
        server_addr.sin_family = AF_INET;
        server_addr.sin_port = htons(port);
        server_addr.sin_addr.s_addr = inet_addr(host.c_str());

        if (bind(m_listen_socket, (sockaddr*)&server_addr, sizeof(server_addr)) == SOCKET_ERROR) {
            std::cerr << "[Server Error] Bind failed on " << host << ":" << port << " - Error: " << WSAGetLastError() << std::endl;
            closesocket(m_listen_socket);
            return false;
        }

        if (::listen(m_listen_socket, SOMAXCONN) == SOCKET_ERROR) {
            std::cerr << "[Server Error] Listen failed: " << WSAGetLastError() << std::endl;
            closesocket(m_listen_socket);
            return false;
        }

        m_running = true;
        std::cout << "============================================================" << std::endl;
        std::cout << "  SkillSwap C++ HTTP Server is RUNNING!                    " << std::endl;
        std::cout << "  Listening at: http://" << host << ":" << port << "/" << std::endl;
        std::cout << "  API Base URL: http://" << host << ":" << port << "/api" << std::endl;
        std::cout << "============================================================" << std::endl;

        while (m_running) {
            sockaddr_in client_addr;
            int client_len = sizeof(client_addr);
            SOCKET client_socket = accept(m_listen_socket, (sockaddr*)&client_addr, &client_len);

            if (client_socket == INVALID_SOCKET) {
                if (!m_running) break;
                continue;
            }

            // Spawn client worker thread
            ClientWorkerArgs* args = new ClientWorkerArgs{ this, client_socket, inet_ntoa(client_addr.sin_addr) };
            HANDLE thread_handle = CreateThread(nullptr, 0, ClientWorkerThread, args, 0, nullptr);
            if (thread_handle) {
                CloseHandle(thread_handle); // Detach thread
            } else {
                delete args;
                closesocket(client_socket);
            }
        }

        return true;
    }

    void stop() {
        m_running = false;
        if (m_listen_socket != INVALID_SOCKET) {
            closesocket(m_listen_socket);
            m_listen_socket = INVALID_SOCKET;
        }
    }

private:
    SOCKET m_listen_socket;
    bool m_running;
    std::string m_static_dir = ".";
    std::map<std::string, std::map<std::string, HttpHandler>> m_routes;

    struct ClientWorkerArgs {
        HttpServer* server;
        SOCKET socket;
        std::string client_ip;
    };

    static DWORD WINAPI ClientWorkerThread(LPVOID lpParam) {
        ClientWorkerArgs* args = static_cast<ClientWorkerArgs*>(lpParam);
        args->server->handle_client(args->socket, args->client_ip);
        delete args;
        return 0;
    }

    void handle_client(SOCKET client_socket, const std::string& client_ip) {
        std::string raw_request;
        char buffer[8192];
        int bytes_received = 0;

        // Read until end of headers (\r\n\r\n)
        while ((bytes_received = recv(client_socket, buffer, sizeof(buffer) - 1, 0)) > 0) {
            buffer[bytes_received] = '\0';
            raw_request.append(buffer, bytes_received);
            if (raw_request.find("\r\n\r\n") != std::string::npos) {
                break;
            }
        }

        if (raw_request.empty()) {
            closesocket(client_socket);
            return;
        }

        HttpRequest req;
        req.client_ip = client_ip;
        parse_request(raw_request, req);

        // Check if client expects 100-continue (e.g. PowerShell/.NET HttpWebRequest)
        std::string expect = req.header("expect");
        if (expect.find("100-continue") != std::string::npos) {
            const char* cont_resp = "HTTP/1.1 100 Continue\r\n\r\n";
            send(client_socket, cont_resp, static_cast<int>(strlen(cont_resp)), 0);
        }

        // Check if there is remaining body to read according to Content-Length
        std::string cl_str = req.header("content-length");
        if (!cl_str.empty()) {
            size_t content_length = 0;
            try { content_length = std::stoul(cl_str); } catch (...) {}
            size_t header_end = raw_request.find("\r\n\r\n");
            size_t body_start = (header_end != std::string::npos) ? header_end + 4 : raw_request.size();
            size_t current_body_size = (raw_request.size() > body_start) ? raw_request.size() - body_start : 0;

            req.body = raw_request.substr(body_start);

            while (current_body_size < content_length) {
                size_t to_read = (sizeof(buffer) - 1 < content_length - current_body_size) ? sizeof(buffer) - 1 : content_length - current_body_size;
                int n = recv(client_socket, buffer, static_cast<int>(to_read), 0);
                if (n <= 0) break;
                buffer[n] = '\0';
                req.body.append(buffer, n);
                current_body_size += n;
            }
        }

        HttpResponse res;

        // Add standard CORS headers to every response
        res.set_header("Access-Control-Allow-Origin", "*");
        res.set_header("Access-Control-Allow-Methods", "GET, POST, PUT, DELETE, OPTIONS");
        res.set_header("Access-Control-Allow-Headers", "Content-Type, Authorization, X-Requested-With");

        // Handle CORS preflight OPTIONS
        if (req.method == "OPTIONS") {
            res.status_code = 204;
            res.status_text = "No Content";
            send_response(client_socket, res);
            closesocket(client_socket);
            return;
        }

        // Match route
        bool handled = false;
        auto method_it = m_routes.find(req.method);
        if (method_it != m_routes.end()) {
            // 1. Direct exact match
            auto handler_it = method_it->second.find(req.path);
            if (handler_it != method_it->second.end()) {
                handler_it->second(req, res);
                handled = true;
            } else {
                // 2. Parametric match (e.g. /api/skills/teach/:id or /api/skills/:id)
                for (auto& route_pair : method_it->second) {
                    std::string route_pattern = route_pair.first;
                    if (route_pattern.find(':') != std::string::npos) {
                        std::map<std::string, std::string> params;
                        if (match_route(route_pattern, req.path, params)) {
                            // Put params into query_params for easy access
                            for (auto& p : params) req.query_params[p.first] = p.second;
                            route_pair.second(req, res);
                            handled = true;
                            break;
                        }
                    }
                }
            }
        }

        // If not an API route and method is GET, try static file
        if (!handled && req.method == "GET") {
            if (serve_static_file(req.path, res)) {
                handled = true;
            }
        }

        if (!handled) {
            res.error(404, "Endpoint not found: " + req.method + " " + req.path);
        }

        send_response(client_socket, res);
        closesocket(client_socket);
    }

    void parse_request(const std::string& raw, HttpRequest& req) {
        std::istringstream stream(raw);
        std::string request_line;
        if (!std::getline(stream, request_line)) return;

        // Clean \r
        if (!request_line.empty() && request_line.back() == '\r') request_line.pop_back();

        std::istringstream line_stream(request_line);
        std::string full_url, http_ver;
        line_stream >> req.method >> full_url >> http_ver;

        // Split path and query
        size_t query_pos = full_url.find('?');
        if (query_pos != std::string::npos) {
            req.path = full_url.substr(0, query_pos);
            req.raw_query = full_url.substr(query_pos + 1);
            parse_query(req.raw_query, req.query_params);
        } else {
            req.path = full_url;
        }

        // Parse headers
        std::string header_line;
        while (std::getline(stream, header_line) && header_line != "\r" && !header_line.empty()) {
            if (!header_line.empty() && header_line.back() == '\r') header_line.pop_back();
            size_t colon = header_line.find(':');
            if (colon != std::string::npos) {
                std::string key = header_line.substr(0, colon);
                std::string val = header_line.substr(colon + 1);
                // Trim leading whitespace
                while (!val.empty() && std::isspace(val.front())) val.erase(val.begin());
                std::transform(key.begin(), key.end(), key.begin(), ::tolower);
                req.headers[key] = val;
            }
        }

        // Parse body
        size_t body_pos = raw.find("\r\n\r\n");
        if (body_pos != std::string::npos) {
            req.body = raw.substr(body_pos + 4);
        }
    }

    void parse_query(const std::string& query_str, std::map<std::string, std::string>& out_params) {
        std::istringstream ss(query_str);
        std::string pair;
        while (std::getline(ss, pair, '&')) {
            size_t eq = pair.find('=');
            if (eq != std::string::npos) {
                std::string key = url_decode(pair.substr(0, eq));
                std::string val = url_decode(pair.substr(eq + 1));
                out_params[key] = val;
            } else if (!pair.empty()) {
                out_params[url_decode(pair)] = "";
            }
        }
    }

    std::string url_decode(const std::string& in) {
        std::string out;
        for (size_t i = 0; i < in.length(); ++i) {
            if (in[i] == '%') {
                if (i + 2 < in.length()) {
                    int val;
                    std::istringstream is(in.substr(i + 1, 2));
                    if (is >> std::hex >> val) {
                        out += static_cast<char>(val);
                        i += 2;
                    } else {
                        out += in[i];
                    }
                }
            } else if (in[i] == '+') {
                out += ' ';
            } else {
                out += in[i];
            }
        }
        return out;
    }

    bool match_route(const std::string& pattern, const std::string& actual, std::map<std::string, std::string>& params) {
        std::vector<std::string> pat_parts = split(pattern, '/');
        std::vector<std::string> act_parts = split(actual, '/');

        if (pat_parts.size() != act_parts.size()) return false;

        for (size_t i = 0; i < pat_parts.size(); ++i) {
            if (!pat_parts[i].empty() && pat_parts[i][0] == ':') {
                std::string param_name = pat_parts[i].substr(1);
                params[param_name] = act_parts[i];
            } else if (pat_parts[i] != act_parts[i]) {
                return false;
            }
        }
        return true;
    }

    std::vector<std::string> split(const std::string& s, char delim) {
        std::vector<std::string> tokens;
        std::string token;
        std::istringstream tokenStream(s);
        while (std::getline(tokenStream, token, delim)) {
            if (!token.empty()) tokens.push_back(token);
        }
        return tokens;
    }

    bool serve_static_file(const std::string& req_path, HttpResponse& res) {
        std::string file_path = req_path;
        if (file_path == "/" || file_path.empty()) file_path = "/index.html";

        // Remove leading slash
        if (file_path[0] == '/') file_path = file_path.substr(1);

        // Security check: prevent directory traversal
        if (file_path.find("..") != std::string::npos) return false;

        std::string full_path = m_static_dir + "/" + file_path;
        std::ifstream file(full_path, std::ios::binary);
        if (!file.is_open()) return false;

        std::ostringstream ss;
        ss << file.rdbuf();
        res.body = ss.str();
        res.status_code = 200;
        res.status_text = "OK";
        res.set_header("Content-Type", get_mime_type(full_path));
        return true;
    }

    std::string get_mime_type(const std::string& path) {
        size_t dot = path.find_last_of('.');
        if (dot == std::string::npos) return "text/plain";
        std::string ext = path.substr(dot);
        std::transform(ext.begin(), ext.end(), ext.begin(), ::tolower);

        if (ext == ".html" || ext == ".htm") return "text/html; charset=utf-8";
        if (ext == ".css") return "text/css; charset=utf-8";
        if (ext == ".js") return "application/javascript; charset=utf-8";
        if (ext == ".json") return "application/json; charset=utf-8";
        if (ext == ".png") return "image/png";
        if (ext == ".jpg" || ext == ".jpeg") return "image/jpeg";
        if (ext == ".svg") return "image/svg+xml";
        if (ext == ".ico") return "image/x-icon";
        return "application/octet-stream";
    }

    void send_response(SOCKET client_socket, const HttpResponse& res) {
        std::ostringstream ss;
        ss << "HTTP/1.1 " << res.status_code << " " << res.status_text << "\r\n";
        for (const auto& header : res.headers) {
            ss << header.first << ": " << header.second << "\r\n";
        }
        ss << "Content-Length: " << res.body.size() << "\r\n";
        ss << "Connection: close\r\n";
        ss << "\r\n";

        std::string header_str = ss.str();
        send(client_socket, header_str.c_str(), static_cast<int>(header_str.size()), 0);
        if (!res.body.empty()) {
            send(client_socket, res.body.c_str(), static_cast<int>(res.body.size()), 0);
        }
    }
};

#endif // HTTP_SERVER_HPP
