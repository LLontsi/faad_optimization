/* test_fd.c - Juste pour vérifier que utils.c marche */
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/wait.h>
#include "src/utils.h"
#include <fcntl.h>  
#include "src/ipc_protocol.h"

int main() {
    // On crée une socketpair (comme un tuyau bidirectionnel) pour simuler Gateway <-> Container
    int sv[2];
    if (socketpair(AF_UNIX, SOCK_STREAM, 0, sv) == -1) {
        perror("socketpair");
        exit(1);
    }

    if (fork() == 0) {
        /* --- ENFANT (Simule le Conteneur) --- */
        close(sv[0]); // Ferme le bout parent
        
        ipc_header_t header;
        int received_fd;
        
        printf("[Container] Attente du FD...\n");
        recv_fd(sv[1], &received_fd, &header, sizeof(header));
        
        printf("[Container] Reçu Commande: %d\n", header.command);
        printf("[Container] Reçu FD: %d\n", received_fd);
        
        // Test : Écrire dans le FD reçu
        dprintf(received_fd, "Hello from Container via transferred FD!\n");
        close(received_fd);
        exit(0);
    } else {
        /* --- PARENT (Simule la Gateway) --- */
        close(sv[1]); // Ferme le bout enfant
        
        // On ouvre un fichier "test.txt" pour simuler une connexion client
        int fake_client_fd = open("test_output.txt", O_RDWR | O_CREAT | O_TRUNC, 0644);
        
        ipc_header_t header = {0};
        header.command = CMD_NEW_REQUEST;
        
        printf("[Gateway] Envoi du FD %d au container...\n", fake_client_fd);
        send_fd(sv[0], fake_client_fd, &header, sizeof(header));
        
        // Important : La gateway ferme sa copie !
        close(fake_client_fd);
        
        wait(NULL); // Attendre l'enfant
        printf("[Gateway] Fini. Vérifie le fichier test_output.txt\n");
    }
    return 0;
}