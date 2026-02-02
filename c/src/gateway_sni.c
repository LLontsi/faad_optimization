/* src/gateway_sni.c - DUAL HTTP/HTTPS */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <sys/epoll.h>
#include <netinet/in.h>
#include <fcntl.h>
#include "utils.h"
#include "sni_parser.h"
#include "host_parser.h"
#include "ipc_protocol.h"

#define MAX_EVENTS 20
#define PORT_HTTPS 8443
#define PORT_HTTP  8080
#define BUF_SIZE 4096
#define SOCKET_DIR "/tmp"

void resolve_target(const char *target, char *path) {
    char name[128];
    const char *start = target;
    if (target[0] == '/') start++;
    
    const char *dot = strchr(start, '.');
    int len = dot ? (dot - start) : strlen(start);
    if (len > 127) len = 127;
    
    strncpy(name, start, len);
    name[len] = '\0';
    
    sprintf(path, "%s/%s.sock", SOCKET_DIR, name);
}

int create_tcp_server(int port) {
    int fd = socket(AF_INET, SOCK_STREAM, 0);
    int opt = 1;
    setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));
    
    struct sockaddr_in addr;
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = INADDR_ANY;
    addr.sin_port = htons(port);
    
    bind(fd, (struct sockaddr*)&addr, sizeof(addr));
    listen(fd, 10);
    return fd;
}

void route_connection(int client_fd, const char *target, ipc_header_t *header_data, 
                      void *payload, int payload_len) {
    char container_path[256];
    resolve_target(target, container_path);
    
    printf("[Gateway] Route vers %s (Target=%s, TLS=%d)\n", 
           container_path, target, header_data->is_tls);

    int container_conn = connect_unix_socket(container_path);
    if (container_conn == -1) {
        fprintf(stderr, "[Gateway] Container %s introuvable\n", container_path);
        close(client_fd);
        return;
    }

    size_t total_len = sizeof(ipc_header_t) + payload_len;
    unsigned char *combined_buffer = malloc(total_len);
    
    memcpy(combined_buffer, header_data, sizeof(ipc_header_t));
    if (payload && payload_len > 0) {
        memcpy(combined_buffer + sizeof(ipc_header_t), payload, payload_len);
    }

    if (send_fd(container_conn, client_fd, combined_buffer, total_len) < 0) {
        perror("[Gateway] send_fd");
    } else {
        printf("[Gateway] Transfer OK\n");
    }

    free(combined_buffer);
    close(container_conn);
    close(client_fd);
}

int main() {
    int https_fd = create_tcp_server(PORT_HTTPS);
    int http_fd  = create_tcp_server(PORT_HTTP);
    int return_fd = create_unix_server(GATEWAY_RETURN_SOCK);
    chmod(GATEWAY_RETURN_SOCK, 0777);

    int epoll_fd = epoll_create1(0);
    struct epoll_event ev, events[MAX_EVENTS];

    ev.events = EPOLLIN;
    ev.data.fd = https_fd;
    epoll_ctl(epoll_fd, EPOLL_CTL_ADD, https_fd, &ev);
    
    ev.data.fd = http_fd;
    epoll_ctl(epoll_fd, EPOLL_CTL_ADD, http_fd, &ev);
    
    ev.data.fd = return_fd;
    epoll_ctl(epoll_fd, EPOLL_CTL_ADD, return_fd, &ev);

    printf("[Gateway] Ready - HTTPS:%d, HTTP:%d, Return:%s\n", 
           PORT_HTTPS, PORT_HTTP, GATEWAY_RETURN_SOCK);

    while(1) {
        int nfds = epoll_wait(epoll_fd, events, MAX_EVENTS, -1);
        
        for (int i = 0; i < nfds; i++) {
            int fd = events[i].data.fd;

            if (fd == https_fd || fd == http_fd) {
                int client = accept(fd, NULL, NULL);
                unsigned char buf[BUF_SIZE];
                
                int n = recv(client, buf, BUF_SIZE, MSG_PEEK);
                if (n <= 0) {
                    close(client);
                    continue;
                }

                char target[256] = {0};
                int is_tls = (fd == https_fd);
                int found = 0;

                if (is_tls) {
                    if (parse_sni(buf, n, target, sizeof(target)) == 1) {
                        found = 1;
                        printf("[Gateway] HTTPS SNI: %s\n", target);
                    }
                } else {
                    if (parse_host_header((char*)buf, n, target, sizeof(target))) {
                        found = 1;
                        printf("[Gateway] HTTP Host: %s\n", target);
                    }
                }

                if (found) {
                    ipc_header_t h = {0};
                    h.command = CMD_NEW_REQUEST;
                    h.is_tls = is_tls;
                    strncpy(h.target_name, target, sizeof(h.target_name));
                    
                    route_connection(client, target, &h, NULL, 0);
                } else {
                    printf("[Gateway] No Host/SNI found\n");
                    close(client);
                }
            }
            else if (fd == return_fd) {
                int container_conn = accept(return_fd, NULL, NULL);
                int client_fd;
                unsigned char buffer[32768];
                
                int n = recv_fd(container_conn, &client_fd, buffer, sizeof(buffer));
                
                if (n > 0) {
                    ipc_header_t *h = (ipc_header_t*)buffer;
                    
                    if (h->command == CMD_MIGRATION) {
                        printf("[Gateway] MIGRATION (TLS=%d) vers: %s\n", 
                               h->is_tls, h->target_name);
                        
                        void *payload = buffer + sizeof(ipc_header_t);
                        int payload_len = n - sizeof(ipc_header_t);
                        
                        route_connection(client_fd, h->target_name, h, payload, payload_len);
                    }
                }
                close(container_conn);
            }
        }
    }
    return 0;
}