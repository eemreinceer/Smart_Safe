#pragma once

#include "events.h"
#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"

// ─────────────────────────────────────────────
//  Security Manager
//  Sorumluluk: yetki doğrulama, başarısız deneme
//  sayacı, lockdown state makinesi.
//  FreeRTOS event queue'su bu modüle aittir.
// ─────────────────────────────────────────────

extern QueueHandle_t eventQueue;

void          securityInit        ();               // queue oluşturur
void          securityHandleEvent (const SafeEvent& ev); // auth/unauth işler
bool          securityIsLockedDown();
unsigned long securityGetLockUntil();
unsigned long securityGetLockRemainingMs();
void          securityCheckExpiry ();               // taskSystem çağırır
