#pragma once
#include <Arduino.h>

typedef struct {
    uint8_t state;
    uint8_t type;
    uint16_t content_size;
    char *content;
} attack_status_t;


typedef enum {
    ATTACK_TYPE_PASSIVE,
    ATTACK_TYPE_HANDSHAKE,
    ATTACK_TYPE_PMKID,
    ATTACK_TYPE_DOS
} attack_type_t;


typedef enum {
    READY,      ///< no attack is in progress and results from previous attack run are available.
    RUNNING,    ///< attack is in progress, attack_status_t.content may not be consistent.
    FINISHED,   ///< last attack finsihed and results are available.
    TIMEOUT     ///< last attack timed out. This option will be moved as sub category of FINISHED state.
} attack_state_t;