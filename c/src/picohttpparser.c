/* src/picohttpparser.c - Version Minifiée pour Prototype */
#include <stddef.h>
#include <string.h>
#include "picohttpparser.h"

// Implémentation simplifiée qui cherche juste la première ligne
// GET /path HTTP/1.1
int phr_parse_request(const char *buf, size_t len, const char **method, size_t *method_len, const char **path, size_t *path_len,
                      int *minor_version, struct phr_header *headers, size_t *num_headers, size_t last_len) {
    
    // Trouver la fin de la méthode
    const char *p = buf;
    const char *end = buf + len;
    
    // Méthode
    while (p < end && *p != ' ') p++;
    if (p == end) return -2; // Incomplet
    *method = buf;
    *method_len = p - buf;
    p++; // Skip space

    // Path
    *path = p;
    while (p < end && *p != ' ') p++;
    if (p == end) return -2;
    *path_len = p - *path;
    
    // On ignore le reste pour ce prototype
    return p - buf; 
}