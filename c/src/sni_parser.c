/* src/sni_parser.c */
#include <string.h>
#include <arpa/inet.h> // Pour ntohs
#include "sni_parser.h"

int parse_sni(const unsigned char *buf, size_t len, char *hostname, size_t hostname_len) {
    // Vérification minimale taille TLS header (5 bytes)
    if (len < 5) return 0;

    // Type Handshake (0x16) et Version TLS (0x03, 0x01/2/3)
    if (buf[0] != 0x16 || buf[1] != 0x03) return -1;

    // Longueur du fragment TLS
    unsigned int frag_len = (buf[3] << 8) | buf[4];
    if (len < 5 + frag_len) return 0; // Paquet incomplet

    // Position courante
    size_t pos = 5;

    // Type Handshake: ClientHello (0x01)
    if (buf[pos] != 0x01) return -1;
    pos++;

    // Longueur ClientHello (3 bytes)
    pos += 3;

    // Version (2 bytes) + Random (32 bytes)
    pos += 2 + 32;

    if (pos >= len) return -1;

    // Session ID length
    unsigned char sess_id_len = buf[pos++];
    pos += sess_id_len;

    if (pos + 2 >= len) return -1;

    // Cipher Suites length
    unsigned short cipher_suites_len = (buf[pos] << 8) | buf[pos + 1];
    pos += 2 + cipher_suites_len;

    if (pos + 1 >= len) return -1;

    // Compression methods length
    unsigned char comp_methods_len = buf[pos++];
    pos += comp_methods_len;

    if (pos + 2 >= len) return -1;

    // Extensions length
    unsigned short ext_tot_len = (buf[pos] << 8) | buf[pos + 1];
    pos += 2;

    size_t ext_end = pos + ext_tot_len;
    if (ext_end > len) return 0; // Incomplet

    // Parcourir les extensions
    while (pos + 4 <= ext_end) {
        unsigned short ext_type = (buf[pos] << 8) | buf[pos + 1];
        unsigned short ext_len = (buf[pos + 2] << 8) | buf[pos + 3];
        pos += 4;

        if (ext_type == 0x0000) { // Extension SNI
            if (pos + 2 > ext_end) return -1;
            
            // List length (2 bytes)
            pos += 2;
            
            // Type (1 byte, 0x00 = Hostname)
            if (buf[pos] != 0x00) return -1;
            pos++;

            // Hostname length (2 bytes)
            unsigned short host_len = (buf[pos] << 8) | buf[pos + 1];
            pos += 2;

            if (pos + host_len > ext_end) return -1;
            if (host_len >= hostname_len) return -1; // Buffer trop petit

            // COPIE DU SNI
            memcpy(hostname, buf + pos, host_len);
            hostname[host_len] = '\0';
            return 1; // Trouvé !
        }

        pos += ext_len;
    }

    return -1; // SNI non trouvé
}