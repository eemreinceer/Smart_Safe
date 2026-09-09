#pragma once
#include <Arduino.h>

// ─── Başlatma ───────────────────────────────
void initFirebase();
bool firebaseIsReady();
bool firebaseIsConfigured();
void initFirebaseStructure();

// ─── Log Sistemi ────────────────────────────
bool sendLog(String status, String id, String photoBase64 = "", uint32_t eventTimestamp = 0);

// ─── Cihaz Durumu ───────────────────────────
bool updateDeviceStatus();
bool updateLockState(bool isLocked);
void startLockStatusStream();

// ─── Uzak Komut Kontrolü (tek SSL) ──────────
bool checkRemoteCommands(bool& alarmTrigger);
