# RFID/NFC Compatibility Test Results

Okuyucu: MFRC522 / RC522
Kontrolcü: ESP32 DevKit (ESP32-WROOM-32), VSPI, SS=GPIO5, RST=GPIO22
Firmware: `EXPERIMENTS/rfid_nfc_probe`
Test tarihi: _(doldurulacak)_
RC522 VersionReg: _(doldurulacak — seri monitörde `v`)_

> **Bu tablo tahminle doldurulmayacaktır.**
> Her satır yalnızca fiziksel test sonucuna göre yazılır.
> Test edilmemiş satırlar `?` olarak kalır.

## Ana tablo

| Nesne | Detected | UID | UID Length | SAK | PICC Type | UID Stable | SmartSafe V1 |
|---|---|---|---|---|---|---|---|
| RC522 beyaz kart | ? | ? | ? | ? | ? | ? | ? |
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

### RC522 beyaz kart
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
