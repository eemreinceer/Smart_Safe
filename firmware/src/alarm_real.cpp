#include "config.h"

#if !SIMULATION_MODE

#include <Arduino.h>
#include "alarm.h"
#include <DFRobotDFPlayerMini.h>
#include <HardwareSerial.h>

// RX=-1 → bağlı değil, sadece TX kullanılıyor
HardwareSerial dfSerial(1);
DFRobotDFPlayerMini dfplayer;

void initAlarm()
{
    // RX pini -1 → bağlı değil
    dfSerial.begin(9600, SERIAL_8N1, -1, DF_TX);

    // ACK=false → DFPlayer'dan yanıt bekleme (RX hattı olmadığı için)
    if (!dfplayer.begin(dfSerial, false))
        Serial.println("[ALARM] ❌ DFPlayer başlatılamadı");
    else
    {
        dfplayer.volume(25);   // 0-30 arası ses seviyesi
        Serial.println("[ALARM] ✅ DFPlayer hazır");
    }
}

void triggerAlarm()
{
    Serial.println("[ALARM] 🚨 Alarm tetiklendi!");
    dfplayer.play(1);   // SD karttaki 001.mp3 dosyasını çal
}

#endif