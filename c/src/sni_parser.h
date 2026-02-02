/* src/sni_parser.h */
#ifndef SNI_PARSER_H
#define SNI_PARSER_H

#include <stddef.h>

// Analyse un buffer (le début du ClientHello TLS)
// Retourne :
//  1 si SNI trouvé (stocké dans hostname)
//  0 si paquet incomplet (il faut lire plus)
// -1 si erreur ou pas de SNI
int parse_sni(const unsigned char *buf, size_t len, char *hostname, size_t hostname_len);

#endif