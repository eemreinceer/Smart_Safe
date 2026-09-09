#ifndef STATE_MACHINE_H
#define STATE_MACHINE_H

// ─────────────────────────────────────────────
//  State Tanımları
// ─────────────────────────────────────────────
enum SystemState
{
    STATE_IDLE,
    STATE_SCAN,
    STATE_AUTHORIZED,
    STATE_UNAUTHORIZED,
    STATE_ALARM,
    STATE_LOCKDOWN
};

// ─────────────────────────────────────────────
//  Global State
// ─────────────────────────────────────────────

// ─────────────────────────────────────────────
//  Fonksiyonlar
// ─────────────────────────────────────────────
void        setState    (SystemState newState);
SystemState getState    ();
const char* getStateName(SystemState state);

#endif
