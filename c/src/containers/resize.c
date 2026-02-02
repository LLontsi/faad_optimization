/* src/containers/resize.c */
#include "base_container.h"
#include <string.h>

HandlerResult resize_handler(int is_tls, WOLFSSL* ssl, int fd, 
                              const char* path, const char* body, int keep_alive_requested) {
    HandlerResult result;
    
    printf("[Resize] Path='%s', Mode=%s\n", path, is_tls ? "HTTPS" : "HTTP");
    
    if (strncmp(path, "/resize", 7) != 0) {
        result.action = HANDLER_MIGRATE;
        if (strncmp(path, "/echo", 5) == 0 || strcmp(path, "/") == 0) {
            strcpy(result.migrate_target, "/echo");
        } else {
            strcpy(result.migrate_target, path);
        }
        return result;
    }
    
    char response[1024];
    char content[512];
    static int count = 0;
    count++;
    
    snprintf(content, sizeof(content), 
        "RESIZE CONTAINER\n"
        "Image #%d redimensionnee\n"
        "Mode: %s\n",
        count, 
        is_tls ? "HTTPS" : "HTTP");
    
    snprintf(response, sizeof(response),
        "HTTP/1.1 200 OK\r\n"
        "Content-Type: text/plain\r\n"
        "Connection: %s\r\n"
        "Content-Length: %ld\r\n"
        "\r\n"
        "%s", 
        keep_alive_requested ? "keep-alive" : "close",
        strlen(content), 
        content);

    io_write(is_tls, ssl, fd, response, strlen(response));
    
    result.action = keep_alive_requested ? HANDLER_CONTINUE : HANDLER_CLOSE;
    result.keep_alive = keep_alive_requested;
    
    return result;
}

int main() {
    printf("[Resize] Demarrage mode hybride HTTP/HTTPS\n");
    run_container("/tmp/resize.sock", "certs/echo.crt", "certs/echo.key", resize_handler);
    return 0;
}