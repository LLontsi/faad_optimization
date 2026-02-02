/* src/tls_transfer.h - Correction Finale */
#ifndef TLS_TRANSFER_H
#define TLS_TRANSFER_H

#include <wolfssl/options.h>
#include <wolfssl/ssl.h>
#include <wolfssl/internal.h>

// On garde une taille confortable
#define TLS_BLOB_SIZE 16384 

typedef struct {
    unsigned char blob[TLS_BLOB_SIZE];
    word32 blob_len;
    int cipherSuite;
} tls_session_state_t;

#ifdef ENABLE_TLS_LOGIC
void export_tls_state(WOLFSSL* ssl, tls_session_state_t* state) {
    printf("[TLS DEBUG] Adresse state = %p\n", state);
    printf("[TLS DEBUG] Adresse state->blob = %p\n", state->blob);
    printf("[TLS DEBUG] TLS_BLOB_SIZE = %d\n", TLS_BLOB_SIZE);
    
    // D'abord, demander la taille nécessaire
    word32 needed_size = 0;
    int ret = wolfSSL_session_export_internal(ssl, NULL, &needed_size, WOLFSSL_EXPORT_TLS);
    
    printf("[TLS] Taille nécessaire pour export : %d bytes\n", needed_size);
    
    if (needed_size > TLS_BLOB_SIZE) {
        printf("[TLS] ERREUR: Buffer trop petit ! Besoin de %d, disponible %d\n", 
               needed_size, TLS_BLOB_SIZE);
        state->blob_len = 0;
        return;
    }
    
    // Maintenant faire l'export réel
    state->blob_len = TLS_BLOB_SIZE;
    
    printf("[TLS DEBUG] Avant export: blob_len = %d\n", state->blob_len);
    
    ret = wolfSSL_session_export_internal(ssl, state->blob, &state->blob_len, WOLFSSL_EXPORT_TLS);
    
    printf("[TLS DEBUG] Après export: ret = %d, blob_len = %d\n", ret, state->blob_len);
    
    if (ret == SSL_SUCCESS || ret > 0) {
        printf("[TLS] Export réussi (%d bytes)\n", state->blob_len);
    } else {
        printf("[TLS] Erreur Export: %d\n", ret);
        
        // Afficher les codes d'erreur possibles
        if (ret == BAD_FUNC_ARG) printf("  -> BAD_FUNC_ARG\n");
        if (ret == BUFFER_E) printf("  -> BUFFER_E\n");
        if (ret == MEMORY_E) printf("  -> MEMORY_E\n");
        
        state->blob_len = 0;
    }
}

int import_tls_state(WOLFSSL* ssl, tls_session_state_t* state) {
    if (state->blob_len == 0) {
        printf("[TLS] ERREUR: blob_len = 0, rien à importer\n");
        return -1;
    }

    printf("[TLS] Import de %d bytes...\n", state->blob_len);
    
    // ✅ CORRECTION : Utiliser WOLFSSL_EXPORT_TLS
    int ret = wolfSSL_session_import_internal(ssl, state->blob, state->blob_len, WOLFSSL_EXPORT_TLS);
    
    if (ret == SSL_SUCCESS || ret > 0) {
        printf("[TLS] Import réussi !\n");
        ssl->options.side = WOLFSSL_SERVER_END;
        return SSL_SUCCESS;
    } else {
        printf("[TLS] Erreur Import: code %d\n", ret);
        return ret;
    }
}
#endif
#endif