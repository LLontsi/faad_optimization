/* src/utils.h */
#ifndef UTILS_H
#define UTILS_H

#include <sys/types.h>
#include <sys/socket.h>

// Fonction critique : Envoie un FD et une structure de données (header)
// Retourne 0 en cas de succès, -1 en cas d'erreur
int send_fd(int socket, int fd_to_send, void *data, size_t data_len);

// Fonction critique : Reçoit un FD et lit les données dans buffer
// Retourne la taille lue, ou -1 en cas d'erreur
int recv_fd(int socket, int *fd_received, void *buffer, size_t buffer_len);

// Met un socket en mode non-bloquant (utile pour epoll)
int set_nonblocking(int fd);

// Crée un socket Unix écoutant sur un chemin donné
int create_unix_server(const char *path);

// Se connecte à un socket Unix
int connect_unix_socket(const char *path);
int set_blocking(int fd);
#endif