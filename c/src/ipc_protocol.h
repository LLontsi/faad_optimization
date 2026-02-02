/* src/ipc_protocol.h */
#ifndef IPC_PROTOCOL_H
#define IPC_PROTOCOL_H

#include <stdint.h>
#include "tls_transfer.h"

#define GATEWAY_RETURN_SOCK "/tmp/return.sock"

#define CMD_NEW_REQUEST 1
#define CMD_MIGRATION   2

#define MAX_SNI_LEN 256

typedef struct {
    uint8_t command;
    char target_name[MAX_SNI_LEN];
    
    uint8_t is_tls;  // ← NOUVEAU: 1=HTTPS, 0=HTTP
    
    tls_session_state_t tls_state; 
    int has_state;
    
    char http_buffer[4096]; 
    int http_buffer_len;
} ipc_header_t;

#endif