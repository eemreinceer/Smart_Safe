#pragma once

// ─── Event Tipleri ───────────────────────────
#define EVENT_AUTHORIZED   1
#define EVENT_UNAUTHORIZED 2
#define EVENT_REMOTE_ALARM 3

// ─── Olay Yapısı ─────────────────────────────
struct SafeEvent
{
    int  type;
    char id[32];
};
