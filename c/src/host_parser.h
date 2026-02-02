/* src/host_parser.h - Parse le header Host: pour HTTP */
#ifndef HOST_PARSER_H
#define HOST_PARSER_H

#include <string.h>
#include <ctype.h>

/* Extrait "echo.local" depuis "Host: echo.local" */
int parse_host_header(const char *buf, int len, char *hostname, int max_len) {
    const char *ptr = strstr(buf, "Host:");
    if (!ptr) ptr = strstr(buf, "host:");
    
    if (ptr) {
        ptr += 5;
        while (*ptr == ' ' || *ptr == '\t') ptr++;
        
        int i = 0;
        while (i < max_len - 1 && ptr[i] != '\r' && ptr[i] != '\n' && 
               ptr[i] != '\0' && ptr[i] != ':') {
            hostname[i] = ptr[i];
            i++;
        }
        hostname[i] = '\0';
        
        return 1;
    }
    return 0;
}

#endif