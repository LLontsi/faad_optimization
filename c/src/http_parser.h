/*
 * HTTP Parser - Parse méthode, URL, headers HTTP
 */

#ifndef HTTP_PARSER_H
#define HTTP_PARSER_H

#define _GNU_SOURCE
#include <string.h>
#include <stdio.h>
#include <ctype.h>
#include <stdlib.h>

#define MAX_METHOD_LEN 16
#define MAX_URL_LEN 256

typedef enum {
    HTTP_GET,
    HTTP_POST,
    HTTP_PUT,
    HTTP_DELETE,
    HTTP_PATCH,
    HTTP_HEAD,
    HTTP_OPTIONS,
    HTTP_UNKNOWN
} HttpMethod;

typedef struct {
    HttpMethod method;
    char url[MAX_URL_LEN];
    char method_str[MAX_METHOD_LEN];
    int content_length;
    int keep_alive;
} HttpRequest;

static HttpMethod parse_method(const char *method_str) {
    if (strcmp(method_str, "GET") == 0) return HTTP_GET;
    if (strcmp(method_str, "POST") == 0) return HTTP_POST;
    if (strcmp(method_str, "PUT") == 0) return HTTP_PUT;
    if (strcmp(method_str, "DELETE") == 0) return HTTP_DELETE;
    if (strcmp(method_str, "PATCH") == 0) return HTTP_PATCH;
    if (strcmp(method_str, "HEAD") == 0) return HTTP_HEAD;
    if (strcmp(method_str, "OPTIONS") == 0) return HTTP_OPTIONS;
    return HTTP_UNKNOWN;
}

static int parse_http_request(const char *buffer, size_t len, HttpRequest *req) {
    if (len == 0 || buffer == NULL || req == NULL) return -1;
    
    memset(req, 0, sizeof(HttpRequest));
    
    const char *line_end = strstr(buffer, "\r\n");
    if (!line_end) return -1;
    
    char first_line[512];
    size_t line_len = line_end - buffer;
    if (line_len >= sizeof(first_line)) return -1;
    
    memcpy(first_line, buffer, line_len);
    first_line[line_len] = '\0';
    
    char *space1 = strchr(first_line, ' ');
    if (!space1) return -1;
    
    size_t method_len = space1 - first_line;
    if (method_len >= MAX_METHOD_LEN) return -1;
    
    memcpy(req->method_str, first_line, method_len);
    req->method_str[method_len] = '\0';
    req->method = parse_method(req->method_str);
    
    char *space2 = strchr(space1 + 1, ' ');
    if (!space2) return -1;
    
    size_t url_len = space2 - (space1 + 1);
    if (url_len >= MAX_URL_LEN) return -1;
    
    memcpy(req->url, space1 + 1, url_len);
    req->url[url_len] = '\0';
    
    const char *header_start = line_end + 2;
    const char *headers_end = strstr(header_start, "\r\n\r\n");
    if (!headers_end) return -1;
    
    const char *cl_header = strcasestr(header_start, "Content-Length:");
    if (cl_header && cl_header < headers_end) {
        cl_header += 15;
        while (*cl_header == ' ') cl_header++;
        req->content_length = atoi(cl_header);
    }
    
    const char *conn_header = strcasestr(header_start, "Connection:");
    if (conn_header && conn_header < headers_end) {
        conn_header += 11;
        while (*conn_header == ' ') conn_header++;
        if (strncasecmp(conn_header, "keep-alive", 10) == 0) {
            req->keep_alive = 1;
        }
    }
    
    return 0;
}

static int extract_function_name(const char *url, char *function_name, size_t max_len) {
    if (!url || !function_name || max_len == 0) return -1;
    
    const char *func_start = NULL;
    
    if (strncmp(url, "/function/", 10) == 0) {
        func_start = url + 10;
    } else if (url[0] == '/') {
        func_start = url + 1;
    } else {
        return -1;
    }
    
    size_t i = 0;
    while (func_start[i] != '\0' && 
           func_start[i] != '/' && 
           func_start[i] != '?' &&
           i < max_len - 1) {
        function_name[i] = func_start[i];
        i++;
    }
    function_name[i] = '\0';
    
    return (i > 0) ? 0 : -1;
}

static void build_http_response(char *buffer, size_t max_len, 
                                 int status_code, const char *body, 
                                 int keep_alive) {
    const char *status_text;
    switch (status_code) {
        case 200: status_text = "OK"; break;
        case 400: status_text = "Bad Request"; break;
        case 404: status_text = "Not Found"; break;
        case 500: status_text = "Internal Server Error"; break;
        default: status_text = "Unknown"; break;
    }
    
    size_t body_len = body ? strlen(body) : 0;
    
    snprintf(buffer, max_len,
        "HTTP/1.1 %d %s\r\n"
        "Content-Type: text/plain\r\n"
        "Content-Length: %zu\r\n"
        "Connection: %s\r\n"
        "\r\n"
        "%s",
        status_code, status_text,
        body_len,
        keep_alive ? "keep-alive" : "close",
        body ? body : "");
}

#endif /* HTTP_PARSER_H */
