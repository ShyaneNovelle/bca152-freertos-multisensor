#include "system_state.h"

SystemState evaluateSystemState(SystemState current, bool motion, bool timeout) {
    if (motion) {
        return SYSTEM_ACTIVE;
    }
    if (current == SYSTEM_ACTIVE && timeout) {
        return SYSTEM_INACTIVE;
    }
    return current;
}