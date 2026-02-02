/*
 * Hello Container - Simple fonction hello world
 */

#include "base_container.h"

#define FUNCTION_NAME "hello"
#define SOCKET_PATH "/tmp/hello.sock"
#define CERT_FILE "./certs/hello.crt"
#define KEY_FILE "./certs/hello.key"

void handle_hello_request(const HttpRequest *req, char *response, size_t max_len) {
    char body[1024];
    
    snprintf(body, sizeof(body),
        "=== Hello Function ===\n"
        "Hello from FaaS!\n"
        "Method: %s\n"
        "URL: %s\n",
        req->method_str,
        req->url
    );
    
    build_http_response(response, max_len, 200, body, req->keep_alive);
}

int main() {
    printf("[Hello Container] Starting...\n");
    
    WOLFSSL_CTX *ssl_ctx = init_ssl_context(CERT_FILE, KEY_FILE);
    if (ssl_ctx == NULL) {
        return 1;
    }
    
    int unix_sock = create_unix_server(SOCKET_PATH);
    if (unix_sock < 0) {
        wolfSSL_CTX_free(ssl_ctx);
        return 1;
    }
    
    printf("[Hello] Ready\n");
    
    while (1) {
        int gateway_sock = accept(unix_sock, NULL, NULL);
        if (gateway_sock < 0) continue;
        
        ReceivedFD recv_fd;
        if (receive_fd(gateway_sock, &recv_fd) < 0) {
            close(gateway_sock);
            continue;
        }
        close(gateway_sock);
        
        if (recv_fd.type == CONN_HTTP) {
            handle_http_connection(recv_fd.fd, FUNCTION_NAME, handle_hello_request);
        } else {
            handle_https_connection(recv_fd.fd, ssl_ctx, FUNCTION_NAME,
                                     handle_hello_request, NULL, 0);
        }
        
        close(recv_fd.fd);
    }
    
    return 0;
}
