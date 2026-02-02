/* src/containers/base_container.h - HYBRIDE HTTP/HTTPS */
#ifndef BASE_CONTAINER_H
#define BASE_CONTAINER_H

#define ENABLE_TLS_LOGIC 

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/select.h>
#include <sys/socket.h>
#include <errno.h>
#include <wolfssl/options.h>
#include <wolfssl/ssl.h>
#include "../utils.h"
#include "../ipc_protocol.h"
#include "../picohttpparser.h"
#include "../tls_transfer.h"
#include "../handler_types.h"

#define KEEPALIVE_TIMEOUT_SEC 30
#define MAX_KEEPALIVE_REQUESTS 100

/* Wrappers IO hybrides */
static int io_read(int is_tls, WOLFSSL* ssl, int fd, char* buf, int sz) {
    if (is_tls) return wolfSSL_read(ssl, buf, sz);
    return recv(fd, buf, sz, 0);
}

static int io_write(int is_tls, WOLFSSL* ssl, int fd, const char* buf, int sz) {
    if (is_tls) return wolfSSL_write(ssl, buf, sz);
    return send(fd, buf, sz, 0);
}

/* Handler hybride */
typedef HandlerResult (*request_handler_t)(int is_tls, WOLFSSL* ssl, int fd, 
                                            const char* path, const char* body, int keep_alive);

static int is_keepalive_requested(const char* http_buffer, int len) {
    const char* conn_header = strstr(http_buffer, "Connection:");
    if (conn_header) {
        conn_header += 11;
        while (*conn_header == ' ') conn_header++;
        if (strncasecmp(conn_header, "keep-alive", 10) == 0) return 1;
    }
    return 0;
}

static HandlerResult process_request(int is_tls, WOLFSSL* ssl, int fd, 
                                      char* http_buf, int len, request_handler_t handler) {
    const char *method, *path_ptr;
    size_t method_len, path_len;
    int minor_version;
    struct phr_header headers[10];
    size_t num_headers = 10;
    
    phr_parse_request(http_buf, len, &method, &method_len, &path_ptr, &path_len, 
                      &minor_version, headers, &num_headers, 0);
    
    char path[256];
    if (path_len < 256) {
        memcpy(path, path_ptr, path_len);
        path[path_len] = '\0';
    } else {
        strcpy(path, "/");
    }
    
    int keep_alive = is_keepalive_requested(http_buf, len);
    return handler(is_tls, ssl, fd, path, http_buf, keep_alive);
}

static void migrate_connection(int is_tls, WOLFSSL* ssl, int client_fd, 
                                const char* target, char* http_buf, int http_len) {
    printf("\n[Container] Migration vers '%s' (Mode %s)\n", 
           target, is_tls ? "HTTPS" : "HTTP");
    
    ipc_header_t mig_header;
    memset(&mig_header, 0, sizeof(mig_header));
    mig_header.command = CMD_MIGRATION;
    mig_header.is_tls = is_tls;
    strcpy(mig_header.target_name, target);
    
    if (is_tls) {
        export_tls_state(ssl, &mig_header.tls_state);
        if (mig_header.tls_state.blob_len > 0) {
            mig_header.has_state = 1;
        }
    } else {
        mig_header.has_state = 0;
    }
    
    memcpy(mig_header.http_buffer, http_buf, http_len);
    mig_header.http_buffer_len = http_len;
    
    int ret_sock = connect_unix_socket(GATEWAY_RETURN_SOCK);
    if (ret_sock != -1) {
        send_fd(ret_sock, client_fd, &mig_header, sizeof(mig_header));
        close(ret_sock);
        printf("[Container] FD envoye\n");
    }
}

static void handle_keepalive_loop(int is_tls, WOLFSSL* ssl, int client_fd, 
                                   const char* socket_path, request_handler_t handler) {
    int request_count = 0;
    
    printf("\n[Container] KEEPALIVE LOOP START (%s)\n", is_tls ? "HTTPS" : "HTTP");
    
    while (request_count < MAX_KEEPALIVE_REQUESTS) {
        request_count++;
        
        struct timeval timeout;
        timeout.tv_sec = KEEPALIVE_TIMEOUT_SEC;
        timeout.tv_usec = 0;
        
        fd_set read_fds;
        FD_ZERO(&read_fds);
        FD_SET(client_fd, &read_fds);
        
        int ready = select(client_fd + 1, &read_fds, NULL, NULL, &timeout);
        if (ready <= 0) {
            printf("[Container] Timeout ou erreur\n");
            break;
        }
        
        char http_buf[4096];
        int received = io_read(is_tls, ssl, client_fd, http_buf, sizeof(http_buf) - 1);
        
        if (received <= 0) {
            printf("[Container] Connexion fermee\n");
            break;
        }
        
        http_buf[received] = '\0';
        printf("[Container] Requete #%d: %d bytes\n", request_count, received);
        
        HandlerResult result = process_request(is_tls, ssl, client_fd, 
                                                http_buf, received, handler);
        
        if (result.action == HANDLER_CLOSE) {
            printf("[Container] Handler demande fermeture\n");
            break;
        } else if (result.action == HANDLER_MIGRATE) {
            printf("[Container] Migration demandee\n");
            migrate_connection(is_tls, ssl, client_fd, result.migrate_target, 
                              http_buf, received);
            return;
        }
    }
    
    printf("[Container] KEEPALIVE LOOP END\n");
}

void run_container(const char* socket_path, const char* cert_file, 
                   const char* key_file, request_handler_t handler) {
    wolfSSL_Init();
    WOLFSSL_CTX* ctx = wolfSSL_CTX_new(wolfTLSv1_2_server_method());
    wolfSSL_CTX_use_certificate_file(ctx, cert_file, SSL_FILETYPE_PEM);
    wolfSSL_CTX_use_PrivateKey_file(ctx, key_file, SSL_FILETYPE_PEM);

    int ipc_fd = create_unix_server(socket_path);
    printf("[Container] Pret sur %s (Mode Hybride HTTP/HTTPS)\n", socket_path);

    while(1) {
        int gateway_conn = accept(ipc_fd, NULL, NULL);
        if (gateway_conn == -1) continue;

        int client_fd;
        unsigned char buffer[32768]; 
        
        int n = recv_fd(gateway_conn, &client_fd, buffer, sizeof(buffer));
        if (n > 0) {
            ipc_header_t *header = (ipc_header_t*)buffer;
            int is_tls = header->is_tls;
            WOLFSSL* ssl = NULL;
            
            set_blocking(client_fd);
            printf("\n[Container] Client Mode: %s\n", is_tls ? "HTTPS" : "HTTP");

            int setup_ok = 0;
            
            if (is_tls) {
                ssl = wolfSSL_new(ctx);
                wolfSSL_set_fd(ssl, client_fd);
                
                if (header->command == CMD_MIGRATION && header->has_state) {
                    printf("[Container] Import TLS\n");
                    if (import_tls_state(ssl, &header->tls_state) == SSL_SUCCESS) {
                        setup_ok = 1;
                    }
                } else {
                    printf("[Container] Handshake TLS\n");
                    if (wolfSSL_accept(ssl) == SSL_SUCCESS) {
                        setup_ok = 1;
                    }
                }
            } else {
                setup_ok = 1;
            }

            if (setup_ok) {
                if (header->command == CMD_MIGRATION && header->http_buffer_len > 0) {
                    printf("[Container] Buffer herite: %d bytes\n", header->http_buffer_len);
                    HandlerResult res = process_request(is_tls, ssl, client_fd, 
                                                         header->http_buffer, 
                                                         header->http_buffer_len, handler);
                    
                    if (res.action == HANDLER_MIGRATE) {
                        migrate_connection(is_tls, ssl, client_fd, res.migrate_target,
                                          header->http_buffer, header->http_buffer_len);
                        close(gateway_conn);
                        continue;
                    } else if (res.action == HANDLER_CONTINUE) {
                        handle_keepalive_loop(is_tls, ssl, client_fd, socket_path, handler);
                    }
                } else {
                    handle_keepalive_loop(is_tls, ssl, client_fd, socket_path, handler);
                }
            }
            
            if (is_tls && ssl) wolfSSL_free(ssl);
            close(client_fd);
        }
        close(gateway_conn);
    }
    
    wolfSSL_CTX_free(ctx);
    wolfSSL_Cleanup();
}

#endif