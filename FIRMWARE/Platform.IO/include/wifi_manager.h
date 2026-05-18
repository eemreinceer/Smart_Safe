#pragma once

// ─────────────────────────────────────────────
//  WiFi Manager
//  Sorumluluk: bağlantı kurma, yeniden bağlanma,
//  NTP senkronizasyonu ve TX gücü ayarı.
//  Firebase, sadece bu modülün sağladığı bağlantıyı kullanır.
// ─────────────────────────────────────────────

void wifiInit    (int maxRetries = 120); // setup()'ta çağrılır, bloklar
void wifiReconnect();                    // taskWifi içinde çağrılır
bool wifiIsConnected();
