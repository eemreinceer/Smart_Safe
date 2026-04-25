# Akıllı Güvenlik Kasası (Smart Safe)

ESP32-CAM tabanlı, Firebase bağlantılı, yapay zeka destekli akıllı güvenlik kasası projesi.

## Özellikler

- **RFID ile Kimlik Doğrulama** — RC522 modülü ile yetkili kart okuma
- **Yüz Tanıma (AI)** — DeepFace/ArcFace ile Python sunucu üzerinde gerçek zamanlı yüz doğrulama
- **Firebase Entegrasyonu** — Gerçek zamanlı durum izleme ve uzaktan kilit açma
- **Web Dashboard** — React tabanlı, Firebase Hosting'de yayında canlı panel
- **Wokwi Simülasyonu** — Donanımsız test ortamı
- **Ses Geri Bildirimi** — DFPlayer Mini ile sesli uyarılar
- **OTA Güncelleme** — Kablosuz firmware güncellemesi

## Proje Yapısı

```
├── firmware/          # PlatformIO — ESP32 Firmware (C++/Arduino)
│   ├── include/       # Header dosyaları
│   ├── src/           # Kaynak dosyalar
│   ├── diagram.json   # Wokwi simülasyon devresi
│   └── wokwi_test.sh  # Otomatik entegrasyon testleri
├── web/               # React Web Dashboard
│   └── src/
├── python/            # AI Yüz Tanıma Sunucusu (Python)
└── pcb/               # KiCad PCB Tasarımı + Gerber Dosyaları
    ├── kicad/
    └── gerber/
```

## Kurulum

### 1. ESP32 Firmware

```bash
# config.h oluştur
cp firmware/include/config.example.h firmware/include/config.h
# config.h içindeki değerleri kendi bilgilerinizle doldurun
```

PlatformIO ile derle ve yükle:
```bash
cd firmware
pio run --target upload
```

Wokwi simülasyonu için:
```bash
pio run   # firmware.bin üret
wokwi-cli .
```

### 2. Web Dashboard

```bash
cd web
npm install
npm run dev        # Geliştirme
npm run build      # Production build
firebase deploy --only hosting
```

### 3. Python AI Sunucusu

```bash
cd python
pip install -r requirements.txt

# Firebase servis hesabı anahtarını ekle
cp serviceAccountKey.json.example serviceAccountKey.json
# serviceAccountKey.json'ı Firebase Console'dan indirip yerleştir

python sanal_kasa.py
```

## Konfigürasyon

### Firebase Ayarları

1. [Firebase Console](https://console.firebase.google.com)'da yeni proje oluştur
2. Realtime Database etkinleştir
3. Authentication > Email/Password etkinleştir, admin hesabı oluştur
4. `firmware/include/config.example.h` → `config.h` kopyala ve doldur
5. `web/src/firebase.js` içindeki `firebaseConfig`'i güncelle
6. Python için: Project Settings > Service Accounts > Generate new private key

### Wokwi Token

Otomatik testler için:
1. [wokwi.com/dashboard/ci](https://wokwi.com/dashboard/ci) adresinden token al
2. `wokwi_test.sh` içine yaz ya da `WOKWI_CLI_TOKEN` ortam değişkenine ekle

## Devre Şeması

PCB dosyaları KiCad ile açılabilir. Gerber dosyaları üretim için hazırdır.

**Ana Bileşenler:**
| Bileşen | Bağlantı |
|---|---|
| ESP32-CAM (AI Thinker) | Ana mikrodenetleyici |
| RC522 RFID | HSPI: SS=15, SCK=14, MISO=4, MOSI=13 |
| Solenoid Kilit | IRLZ44N MOSFET → GPIO2 |
| DFPlayer Mini | UART TX → GPIO12 |
| LM2596 | 12V → 5V güç regülatörü |

## Mimari

```
[ESP32-CAM] ←→ [Firebase RTDB] ←→ [Web Dashboard]
     ↑                                    ↑
  [RFID]                           [React App]
  [Lock]
  [DFPlayer]
     
[Python AI Server] ←→ [Firebase RTDB]
  (YüzTanıma/DeepFace)
```

## Lisans

MIT License
