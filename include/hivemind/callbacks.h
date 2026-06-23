#ifndef HIVEMIND_CALLBACKS_H
#define HIVEMIND_CALLBACKS_H

#include <stdint.h>

typedef void (*HivemindMessageCallback)(const char* from_name, const char* message);
typedef void (*HivemindStatusCallback)(const char* status);

#endif // HIVEMIND_CALLBACKS_H