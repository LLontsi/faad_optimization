/* src/handler_types.h */
#ifndef HANDLER_TYPES_H
#define HANDLER_TYPES_H

typedef enum {
    HANDLER_CONTINUE,
    HANDLER_CLOSE,
    HANDLER_MIGRATE
} HandlerAction;

typedef struct {
    HandlerAction action;
    char migrate_target[256];
    int keep_alive;
} HandlerResult;

#endif