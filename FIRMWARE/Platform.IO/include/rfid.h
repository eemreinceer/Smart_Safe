#ifndef RFID_H
#define RFID_H

void initRFID();

// Dönüş değerleri:
//  >0   → yetkili kart (kart index numarası)
//  -1   → kart yok / bekleniyor
//  -2   → yetkisiz kart
int readRFID();

#endif