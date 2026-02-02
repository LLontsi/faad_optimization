/* src/containers/echo.c */
#include "base_container.h"
#include <string.h>

HandlerResult echo_handler(int is_tls, WOLFSSL* ssl, int fd, 
                            const char* path, const char* body, int keep_alive_requested) {
    HandlerResult result;
    
    printf("[Echo] Path='%s', Mode=%s, KeepAlive=%d\n", 
           path, is_tls ? "HTTPS" : "HTTP", keep_alive_requested);
    
    if (strncmp(path, "/echo", 5) != 0 && strcmp(path, "/") != 0) {
        result.action = HANDLER_MIGRATE;
        if (strncmp(path, "/resize", 7) == 0) {
            strcpy(result.migrate_target, "/resize");
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
        "ECHO CONTAINER\n"
        "Requete #%d\n"
        "Mode: %s\n"
        "Keep-Alive: %s\n",
        count, 
        is_tls ? "HTTPS" : "HTTP",
        keep_alive_requested ? "OUI" : "NON");
    
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
    printf("[Echo] Demarrage mode hybride HTTP/HTTPS\n");
    run_container("/tmp/echo.sock", "certs/echo.crt", "certs/echo.key", echo_handler);
    return 0;
}