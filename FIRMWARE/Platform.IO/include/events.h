#pragma once

// ─── Event Tipleri ───────────────────────────
#define EVENT_AUTHORIZED   1
#define EVENT_UNAUTHORIZED 2

// ─── Olay Yapısı ─────────────────────────────
struct SafeEvent
{
    int  type;
    char id[32];
};
