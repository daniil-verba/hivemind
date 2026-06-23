#ifndef HIVEMIND_TYPES_H
#define HIVEMIND_TYPES_H

#include <stdint.h>

typedef uint8_t HivemindNodeId[32];

typedef struct {
    uint16_t port;
    char ip[64];
} HivemindAddress;

#endif // HIVEMIND_TYPES_H