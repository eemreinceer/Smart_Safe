#ifndef FIREBASE_MANAGER_H
#define FIREBASE_MANAGER_H

#include <Arduino.h>

// ─── Başlatma ───
void initFirebase();
void initFirebaseStructure();

// ─── NTP Zaman ───
void   initNTP();
String getTimestamp();

// ─── Log Sistemi ───
bool sendLog(String status, String id);

// ─── Cihaz Durumu ───
bool updateDeviceStatus();
bool updateLockState(bool isLocked);
void startLockStatusStream();

// ─── Alarm Kontrol ───
bool checkAlarmTrigger();
bool checkRemoteUnlock();

// ─── Remote Config ───
bool getRemoteConfig(bool &alarmEnabled, int &lockTimeout);

// ─── Fotoğraf ───
void uploadPhoto(uint8_t* buf, size_t len);

#endif