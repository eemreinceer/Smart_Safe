#include <Arduino.h>
#include "systemstate.h"

static SystemState s_currentState = STATE_IDLE;
static portMUX_TYPE s_stateMux = portMUX_INITIALIZER_UNLOCKED;

void setState(SystemState newState)
{
    portENTER_CRITICAL(&s_stateMux);
    const SystemState oldState = s_currentState;
    if (oldState == newState)
    {
        portEXIT_CRITICAL(&s_stateMux);
        return;
    }
    s_currentState = newState;
    portEXIT_CRITICAL(&s_stateMux);

    Serial.printf("[STATE] %s → %s\n",
                  getStateName(oldState),
                  getStateName(newState));
}

SystemState getState()
{
    portENTER_CRITICAL(&s_stateMux);
    const SystemState state = s_currentState;
    portEXIT_CRITICAL(&s_stateMux);
    return state;
}

const char* getStateName(SystemState state)
{
    switch (state)
    {
        case STATE_IDLE:         return "IDLE";
        case STATE_SCAN:         return "SCAN";
        case STATE_AUTHORIZED:   return "AUTHORIZED";
        case STATE_UNAUTHORIZED: return "UNAUTHORIZED";
        case STATE_ALARM:        return "ALARM";
        case STATE_LOCKDOWN:     return "LOCKDOWN";
        default:                 return "UNKNOWN";
    }
}
