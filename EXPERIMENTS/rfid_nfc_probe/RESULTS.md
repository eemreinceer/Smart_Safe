# RFID/NFC Compatibility Test Results

Okuyucu: MFRC522 / RC522
Kontrolcü: ESP32 DevKit (ESP32-WROOM-32), VSPI, SS=GPIO5, RST=GPIO22
Firmware: `EXPERIMENTS/rfid_nfc_probe`
Test tarihi: 2026-08-16
RC522 VersionReg: **0xB2 — FM17522E klonu** (NXP MFRC522 değil)
Anten gain: 0x7 (max, 48 dB)

> **Okuyucu notu:** Elimizdeki modül Fudan FM17522E klonu. ISO14443A discovery
> için çalışıyor ancak RF eşleşmesi zayıf: referans MIFARE 1K kartta bile
> okumaların çoğu 2–4 aktivasyon denemesi gerektirdi. Bu, ileride bir nesne
> için **C** sonucu alındığında akılda tutulmalı — C, "bu okuyucuyla
> algılanamadı" demektir, "algılanamaz" demek değil.

> **Bu tablo tahminle doldurulmayacaktır.**
> Her satır yalnızca fiziksel test sonucuna göre yazılır.
> Test edilmemiş satırlar `?` olarak kalır.

## Ana tablo

| Nesne | Detected | UID | UID Length | SAK | PICC Type | UID Stable | SmartSafe V1 |
|---|---|---|---|---|---|---|---|
| RC522 beyaz kart | YES (5/5) | `29:5D:63:11` | 4 | 0x08 | MIFARE 1KB | YES (5/5 aynı) | **A** |
| Mavi tag (anahtarlık) | ? | ? | ? | ? | ? | ? | ? |
| İzmirim Kart | ? | ? | ? | ? | ? | ? | ? |
| Diğer ulaşım kartı | ? | ? | ? | ? | ? | ? | ? |
| T.C. kimlik kartı | ? | ? | ? | ? | ? | ? | ? |
| Yeni tip ehliyet | ? | ? | ? | ? | ? | ? | ? |
| Banka kartı (temassız) | ? | ? | ? | ? | ? | ? | ? |
| Kredi kartı (temassız) | ? | ? | ? | ? | ? | ? | ? |
| iPhone | ? | ? | ? | ? | ? | ? | ? |
| Android | ? | ? | ? | ? | ? | ? | ? |
| Diğer 13.56 MHz tag | ? | ? | ? | ? | ? | ? | ? |

`SmartSafe V1` sütunu: **A / B / C** (README'deki sınıflandırma).

## UID stabilite kayıtları

Her nesne için 5 okuma:

### RC522 beyaz kart  (referans kart)
```text
ATQA   : 0x0004   SAK : 0x08   PICC : MIFARE 1KB

UID #1 : 29 5D 63 11   (aktivasyon 1/4)
UID #2 : 29 5D 63 11   (aktivasyon 4/4)
UID #3 : 29 5D 63 11   (aktivasyon 2/4)
UID #4 : 29 5D 63 11   (aktivasyon 4/4)
UID #5 : 29 5D 63 11   (aktivasyon 2/4)

Detected     : YES (5/5)
UID readable : YES (5/5)
UID stable   : YES  - 5 okumanın tamamında birebir aynı UID
Class        : A
```

Not: ilk turda (retry düzeltmesi öncesi) 5 okumadan biri "UID okunamadı"
vermişti. UID hiçbir okumada değişmediği, yalnızca "var/yok" salınımı olduğu
için bunun kartın özelliği değil, kart alana girerken yarım eşleşmiş haldeyken
poll'a denk gelmesi olduğu değerlendirildi. Aktivasyon retry'ı eklendikten
sonra 5/5 okundu ve retry sayaçları (2/4, 4/4) bu yorumu doğruladı.

### Mavi tag
```text
UID #1 :
UID #2 :
UID #3 :
UID #4 :
UID #5 :
Detected     :
UID readable :
UID stable   :
Class        :
```

### İzmirim Kart
```text
UID #1 :
UID #2 :
UID #3 :
UID #4 :
UID #5 :
Detected     :
UID readable :
UID stable   :
Class        :
```

### T.C. kimlik kartı
```text
UID #1 :
UID #2 :
UID #3 :
UID #4 :
UID #5 :
Detected     :
UID readable :
UID stable   :
Class        :
```

### Yeni tip ehliyet
```text
UID #1 :
UID #2 :
UID #3 :
UID #4 :
UID #5 :
Detected     :
UID readable :
UID stable   :
Class        :
```

### Banka kartı
```text
UID #1 :
UID #2 :
UID #3 :
UID #4 :
UID #5 :
Detected     :
UID readable :
UID stable   :
Class        :
```

### Kredi kartı
```text
UID #1 :
UID #2 :
UID #3 :
UID #4 :
UID #5 :
Detected     :
UID readable :
UID stable   :
Class        :
```

### iPhone
```text
Ekran kilidi durumu :
Express Card ayarı  :
UID #1 :
UID #2 :
UID #3 :
UID #4 :
UID #5 :
Detected     :
UID readable :
UID stable   :
Class        :
```

### Android
```text
NFC ayarı açık mı :
Ekran kilidi      :
UID #1 :
UID #2 :
UID #3 :
UID #4 :
UID #5 :
Detected     :
UID readable :
UID stable   :
Class        :
```

## Her nesne için üç soru

Her test yorumlanırken ayrı ayrı cevaplanacak:

1. RC522 bunu algılıyor mu?
2. UID veya kullanılabilir bir identifier alabiliyor muyuz?
3. Bu identifier SmartSafe authentication için mantıklı mı?

## Sonuç / karar

_(fiziksel testler bittikten sonra doldurulacak)_
