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
| Mavi tag (anahtarlık) | YES (6/6) | `A7:93:AA:14` | 4 | 0x08 | MIFARE 1KB | YES (6/6 aynı) | **A** |
| İzmirim Kart | YES (5/5) | `04:6F:43:BA:C4:76:80` | 7 | 0x20 | ISO/IEC 14443-4 | YES (5/5 aynı) | **A** |
| Diğer ulaşım kartı | ? | ? | ? | ? | ? | ? | ? |
| T.C. kimlik kartı | ? | ? | ? | ? | ? | ? | ? |
| Yeni tip ehliyet | ? | ? | ? | ? | ? | ? | ? |
| Banka kartı (temassız) | YES (5/5) | `05:82:15:EB:39:B2:00` | 7 | 0x28 | ISO14443-4 + MIFARE emülasyonu (lib: Unknown) | YES (5/5 aynı) | **A** (teknik) |
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

### Mavi tag (anahtarlık)
```text
ATQA   : 0x0004   SAK : 0x08   PICC : MIFARE 1KB

UID #1 : A7 93 AA 14   (aktivasyon 4/4)
UID #2 : A7 93 AA 14   (aktivasyon 1/4)
UID #3 : A7 93 AA 14   (aktivasyon 2/4)
UID #4 : A7 93 AA 14   (aktivasyon 2/4)
UID #5 : A7 93 AA 14   (aktivasyon 4/4)
UID #6 : A7 93 AA 14   (aktivasyon 3/4)

Detected     : YES (6/6)
UID readable : YES (6/6)
UID stable   : YES  - 6 okumanın tamamında birebir aynı UID
Class        : A
```

Not: bu okumalar beyaz kartla aynı oturumda alındığı için firmware'in o anki
raporu CLASS B verdi. Sebep tag degil, oturumun sifirlanmamis olmasi: ayni
oturumda iki farkli nesne (beyaz kart + mavi tag) goruldugu icin "UID degisti"
sonucu cikti. Mavi tag'e ait 6 okumanin tamami ayni UID'yi verdigi icin dogru
sinif A'dir. Firmware bu tuzagi artik acikca uyariyor.

### İzmirim Kart
```text
ATQA   : 0x0044   SAK : 0x20   PICC : ISO/IEC 14443-4

UID #1 : 04 6F 43 BA C4 76 80   (aktivasyon 2/4)
UID #2 : 04 6F 43 BA C4 76 80   (aktivasyon 2/4)
UID #3 : 04 6F 43 BA C4 76 80   (aktivasyon 4/4)
UID #4 : 04 6F 43 BA C4 76 80   (aktivasyon 2/4)
UID #5 : 04 6F 43 BA C4 76 80   (aktivasyon 4/4)

Detected     : YES (5/5)
UID readable : YES (5/5)
UID stable   : YES  - 5 okumanin tamaminda birebir ayni UID
Class        : A
```

Notlar:
- SAK 0x20 + ATQA 0x0044 + 7 bayt UID: ISO/IEC 14443-4 karti, muhtemelen
  MIFARE DESFire. UID `04` ile basliyor = NXP uretici kodu.
- DESFire kartlar rastgele UID modunda calisacak sekilde yapilandirilabilir.
  Bu kart oyle yapilandirilmamis, sabit UID yayinliyor. Dolayisiyla bu sonuc
  "tum ulasim kartlari A" anlamina gelmez, yalnizca bu kart icin gecerlidir.
- Kart 7 baytlik UID verdigi icin SmartSafe tarafinda UID saklama alani
  4 bayta sabitlenmemeli.

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

### Banka kartı (temassız)
```text
ATQA   : 0x0044   SAK : 0x28

UID #1 : 05 82 15 EB 39 B2 00   (aktivasyon 3/4)
UID #2 : 05 82 15 EB 39 B2 00   (aktivasyon 4/4)
UID #3 : 05 82 15 EB 39 B2 00   (aktivasyon 4/4)
UID #4 : 05 82 15 EB 39 B2 00   (aktivasyon 1/4)
UID #5 : 05 82 15 EB 39 B2 00   (aktivasyon 4/4)

Detected     : YES (5/5)
UID readable : YES (5/5)
UID stable   : YES  - 5 okumanin tamaminda birebir ayni UID
Class        : A (teknik olarak)
```

Notlar:
- **Beklenti tutmadi.** Temassiz odeme kartlarinda yaygin olan rastgele UID
  (0x08 onekli, her okumada degisen) bu kartta YOK. Kart sabit 7 baytlik UID
  yayinliyor. Bu, "banka kartlari rastgele UID kullanir" genellemesinin her
  kart icin gecerli olmadigini gosteriyor.
- SAK 0x28: MFRC522 kutuphanesinin tablosunda yok, `PICC_GetType()` bunu
  "Unknown type" olarak raporluyor. SAK bit alanlarina gore: bit5 (0x20) =
  ISO14443-4 uyumlu, ayrica MIFARE Classic emulasyonu (SmartMX tipi yonga).
  Firmware artik SAK bitlerini ayrica cozumluyor.
- Yalnizca discovery/anti-collision yapildi. Karta ait hicbir hesap verisi,
  EMV alani veya uygulama sorgulanmadi.

**Teknik olarak A, ama SmartSafe icin onerilmez:**
- Odeme karti kaybolur/calinir ve **suresi dolunca yenilenir** (3-5 yil).
  Kart yenilendiginde UID degisir, kullanici kasadan kilitlenir.
- Cuzdan kaybi tek noktada hem odeme araclarini hem kasa anahtarini goturur.
- Kasa acmak icin odeme araci gerektirmek, kullaniciyi kartini taniamdigi
  okuyuculara yaklastirma aliskanligina sokar.
Karar: kabul edilebilir ama **birincil credential olarak secilmemeli**.

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
