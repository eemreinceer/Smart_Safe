#include <Arduino.h>
#include "systemstate.h"

volatile SystemState currentState = STATE_IDLE;

void setState(SystemState newState)
{
    if (currentState == newState)
        return;

    Serial.printf("[STATE] %s → %s\n",
                  getStateName(currentState),
                  getStateName(newState));

    currentState = newState;
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