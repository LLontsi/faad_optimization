/* src/utils.c */
#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <fcntl.h>
#include <sys/socket.h>
#include <sys/un.h>
#include "utils.h"

// Envoi d'un descripteur de fichier + données (header)
int send_fd(int socket, int fd_to_send, void *data, size_t data_len) {
    struct msghdr msg = {0};
    struct iovec iov[1];
    struct cmsghdr *cmsg;
    
    // Buffer pour les données auxiliaires (le FD)
    // On doit allouer assez d'espace pour CMSG_SPACE
    char buf[CMSG_SPACE(sizeof(int))];
    memset(buf, 0, sizeof(buf));

    // 1. Préparer les données "normales" (le header IPC)
    iov[0].iov_base = data;
    iov[0].iov_len = data_len;
    msg.msg_iov = iov;
    msg.msg_iovlen = 1;

    // 2. Préparer les données de contrôle (le FD)
    if (fd_to_send != -1) {
        msg.msg_control = buf;
        msg.msg_controllen = sizeof(buf);

        cmsg = CMSG_FIRSTHDR(&msg);
        cmsg->cmsg_level = SOL_SOCKET;
        cmsg->cmsg_type = SCM_RIGHTS;
        cmsg->cmsg_len = CMSG_LEN(sizeof(int));

        // On copie le FD dans la zone de données du CMSG
        *((int *)CMSG_DATA(cmsg)) = fd_to_send;
    } else {
        msg.msg_control = NULL;
        msg.msg_controllen = 0;
    }

    // 3. Envoyer
    if (sendmsg(socket, &msg, 0) < 0) {
        perror("Failed to send FD");
        return -1;
    }
    return 0;
}

// Réception d'un descripteur de fichier + données
int recv_fd(int socket, int *fd_received, void *buffer, size_t buffer_len) {
    struct msghdr msg = {0};
    struct iovec iov[1];
    struct cmsghdr *cmsg;
    char cmsgbuf[CMSG_SPACE(sizeof(int))];

    // Initialisation
    iov[0].iov_base = buffer;
    iov[0].iov_len = buffer_len;
    msg.msg_iov = iov;
    msg.msg_iovlen = 1;
    msg.msg_control = cmsgbuf;
    msg.msg_controllen = sizeof(cmsgbuf);

    *fd_received = -1; // Valeur par défaut

    // Lecture
    int n = recvmsg(socket, &msg, 0);
    if (n < 0) {
        return -1;
    }

    // Extraction du FD s'il y en a un
    cmsg = CMSG_FIRSTHDR(&msg);
    if (cmsg && cmsg->cmsg_len == CMSG_LEN(sizeof(int))) {
        if (cmsg->cmsg_level == SOL_SOCKET && cmsg->cmsg_type == SCM_RIGHTS) {
            *fd_received = *((int *)CMSG_DATA(cmsg));
        }
    }

    return n; // Retourne le nombre d'octets de données lus
}

int set_nonblocking(int fd) {
    int flags = fcntl(fd, F_GETFL, 0);
    if (flags == -1) return -1;
    return fcntl(fd, F_SETFL, flags | O_NONBLOCK);
}

int create_unix_server(const char *path) {
    int fd;
    struct sockaddr_un addr;

    if ((fd = socket(AF_UNIX, SOCK_STREAM, 0)) == -1) {
        perror("socket error");
        return -1;
    }

    memset(&addr, 0, sizeof(addr));
    addr.sun_family = AF_UNIX;
    strncpy(addr.sun_path, path, sizeof(addr.sun_path) - 1);

    unlink(path); // Supprime le fichier s'il existe déjà

    if (bind(fd, (struct sockaddr*)&addr, sizeof(addr)) == -1) {
        perror("bind error");
        close(fd);
        return -1;
    }

    if (listen(fd, 10) == -1) {
        perror("listen error");
        close(fd);
        return -1;
    }

    return fd;
}

int connect_unix_socket(const char *path) {
    int fd;
    struct sockaddr_un addr;

    if ((fd = socket(AF_UNIX, SOCK_STREAM, 0)) == -1) {
        return -1;
    }

    memset(&addr, 0, sizeof(addr));
    addr.sun_family = AF_UNIX;
    strncpy(addr.sun_path, path, sizeof(addr.sun_path) - 1);

    if (connect(fd, (struct sockaddr*)&addr, sizeof(addr)) == -1) {
        close(fd);
        return -1;
    }

    return fd;
}
/* Ajoute ça à la fin de src/utils.c */
int set_blocking(int fd) {
    int flags = fcntl(fd, F_GETFL, 0);
    if (flags == -1) return -1;
    // On enlève le flag O_NONBLOCK avec un ET binaire et l'inverse du flag
    return fcntl(fd, F_SETFL, flags & ~O_NONBLOCK);
}