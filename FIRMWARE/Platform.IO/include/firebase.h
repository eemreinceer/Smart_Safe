#pragma once
#include <Arduino.h>

// ─── Başlatma ───────────────────────────────
void initFirebase();
void initFirebaseStructure();

// ─── Log Sistemi ────────────────────────────
bool sendLog(String status, String id, String photoBase64 = "");

// ─── Cihaz Durumu ───────────────────────────
bool updateDeviceStatus();
bool updateLockState(bool isLocked);
void startLockStatusStream();

// ─── Uzak Komut Kontrolü (tek SSL) ──────────
bool checkRemoteCommands(bool& alarmTrigger, bool& remoteUnlock);
