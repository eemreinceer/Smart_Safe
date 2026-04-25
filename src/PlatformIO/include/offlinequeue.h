#ifndef OFFLINE_QUEUE_H
#define OFFLINE_QUEUE_H

#include <Arduino.h>

void initOfflineQueue();
void storeOfflineLog(String status, String id);
void trySyncOfflineLogs();

#endif