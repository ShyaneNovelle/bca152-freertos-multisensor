#pragma once

#include <stdbool.h>

typedef enum {
    SYSTEM_ACTIVE = 0,
    SYSTEM_INACTIVE
} SystemState;

SystemState evaluateSystemState(SystemState current, bool motion, bool timeout);