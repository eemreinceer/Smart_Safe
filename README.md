# SmartSafe — RFID Tabanlı IoT Kasa Prototipi

SmartSafe; ESP32-CAM, RC522 RFID okuyucu, solenoid kilit, Firebase Realtime Database ve web kontrol panelinden oluşan bir **prototip** erişim kontrol sistemidir.

Sistemin tek yerel kimlik doğrulama yöntemi, yapılandırılmış RFID kart UID'leridir. Kamera kaynakları yalnızca yetkisiz erişim olayına fotoğraf eklemek için korunmuştur.

> **Güvenlik sınırı:** RC522 ile kart UID karşılaştırması, klonlanabilir kartlara karşı kriptografik authentication sağlamaz. Bu proje hobby/prototype seviyesindedir; değerli varlıklar için tek güvenlik katmanı olarak kullanılmamalıdır.

## Mimari

```text
Tanımlı RFID UID
       |
       v
ESP32-CAM firmware ----> Solenoid/MOSFET kilit kontrolü
       |                         |
       |                         +--> commanded lock state
       v
Firebase RTDB <-------- device status / append-only event logs
       ^
       |
Yetkili web paneli ----> alarm command request
```

- RFID kararı cihazda verilir; Firebase bağlantısı yerel kart kontrolü için gerekli değildir.
- Kilidi yalnızca cihazdaki allowlist'e tanımlı fiziksel RFID kart açabilir; cloud ve web paneli unlock yetkisine sahip değildir.
- Web paneli status veya audit log üretmez.
- TLS CA veya credential eksikse cloud bağlantısı fail-closed devre dışı kalır.
- OTA password hash yapılandırılmamışsa OTA servisi başlatılmaz.

## Repository yapısı

```text
Smart_Safe/
├── 3D/                         # Fusion 360 kaynakları
├── PCB/                        # KiCad şema, PCB ve Gerber kaynakları
├── FIRMWARE/
│   ├── Platform.IO/            # ESP32-CAM firmware
│   └── WEB/akilli-kasa-dashboard/ # Firebase web paneli ve RTDB rules
└── SECURITY.md
```

## Firmware yapılandırması

Gizli bilgiler repoya yazılmaz. Örnek dosyayı yerel `secrets.h` olarak kopyalayıp değerleri doldurun:

```bash
cd FIRMWARE/Platform.IO
cp include/secrets.example.h include/secrets.h
```

`include/secrets.h`, `.gitignore` kapsamındadır. Wi-Fi bilgileri, ayrı Firebase device account credential'ları, Firebase CA PEM, OTA password hash ve izin verilen RFID UID'leri burada tutulur.

Firmware ve dashboard için aynı Firebase hesabını kullanmayın. Device hesabına `device_id: "safe_001"`; yönetici hesabına `admin: true` custom claim verilmelidir. Custom claim'ler yalnızca güvenilir Admin SDK ortamından atanmalıdır.

## Build

```bash
cd FIRMWARE/Platform.IO
pio run -e esp32cam           # simulation build
pio run -e esp32cam-hardware  # gerçek donanım kod yolu
pio run -e esp32cam-production # cihaza yüklenecek, RFID UID zorunlu image
```

Hardware build'in geçmesi, pinlerin/gerilimlerin doğrulandığını veya fiziksel kilidin güvenli çalıştığını kanıtlamaz.

## Wokwi testi

Token'ı shell ortamından verin; token dosyaya veya komut geçmişine yazılmamalıdır:

```bash
export WOKWI_CLI_TOKEN='...'
cd FIRMWARE/Platform.IO
./wokwi_test.sh
```

Script firmware'i derler; tanımlı ve tanımsız mock RFID akışlarını assertion ile kontrol eder. Cloud veya fiziksel donanım kanıtı değildir.

## Firebase deployment

Rules deploy edilmeden dashboard veya cihazı production Firebase projesine bağlamayın:

```bash
cd FIRMWARE/WEB/akilli-kasa-dashboard
firebase deploy --only database,hosting
```

`database.rules.json` varsayılan olarak tüm erişimi reddeder ve yalnızca custom claim ile ayrılmış device/admin/operator rollerine gerekli minimum yetkiyi verir.

Dashboard için `web/firebase-config.example.js` dosyasını yerel
`web/firebase-config.js` olarak kopyalayın. Gerçek dosya Git'te izlenmez. Firebase web
config public istemci yapılandırması olsa da API key, Google Cloud Console'da yalnızca
gerekli Firebase API'leri ve izinli hosting domain'leriyle sınırlanmalıdır.

## Donanım güvenliği

- ESP32-CAM lojik seviyesi 3.3 V'tur; 5 V sinyal doğrudan GPIO'ya uygulanmamalıdır.
- RC522 3.3 V ile beslenmelidir.
- Solenoid MCU regülatöründen beslenmemelidir; ortak GND ve uygun flyback koruması gereklidir.
- MOSFET, 3.3 V gate geriliminde yük akımı için datasheet ile doğrulanmalıdır.
- Brownout detector aktiftir. Brownout reset'i yazılımla gizlenmemeli, besleme bütünlüğü ölçülmelidir.
- Fiziksel lock-position sensörü olmadığı için `is_locked`, mekanik geri besleme değil actuator command state'idir.

## Doğrulama durumu

Kanıtlanmamış latency, uptime veya fiziksel güvenlik iddiası yapılmaz. Kabul seviyeleri: simulation/mock test, hardware configuration build, Firebase Emulator rules testi, bench ölçümü ve son olarak gerçek kart/actuator/network-loss testi.

## Lisans

MIT License — Copyright (c) 2026 Emre İnceer
