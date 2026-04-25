# SmartSafe: IoT Güvenlik Sistemi / IoT Security System

**Gerçek zamanlı kontrol, bulut senkronizasyonu ve özel donanım tasarımını bir araya getiren gömülü sistem projesi**  
**An embedded systems project integrating real-time control, cloud synchronization, and custom hardware design**

---

## Projeye Genel Bakış / Project Overview

SmartSafe, ESP32-CAM tabanlı **tam kapsamlı bir IoT güvenlik çözümüdür**.  
SmartSafe is a **full-stack IoT security solution** built around the ESP32-CAM.

- **RFID tabanlı kimlik doğrulama** / RFID-based authentication with authorized card management
- **Özel gömülü yazılım** (ESP32-CAM gerçek zamanlı kontrol) / Custom embedded firmware (ESP32-CAM real-time control)
- **Bulut senkronizasyonu** (Firebase Realtime Database) / Cloud synchronization (Firebase Realtime Database)
- **Profesyonel PCB & mekanik tasarım** (KiCad + Fusion 360) / Professional PCB & mechanical design
- **Web tabanlı gerçek zamanlı izleme paneli** / Web-based real-time monitoring dashboard

---

## Sistem Mimarisi / System Architecture

```
┌─────────────────────────────────────────────────────────────┐
│           Giriş: RFID Kart / Uzak Komut                     │
│           Input: RFID Card / Remote Command                  │
└────────────────────────┬────────────────────────────────────┘
                         │
┌────────────────────────▼────────────────────────────────────┐
│           ESP32-CAM Yazılımı (C++ / PlatformIO)              │
│           ESP32-CAM Firmware (C++ / PlatformIO)              │
│    RFID Kontrolü → Kilit Kontrolü → Olay Kaydı             │
│    RFID Check → Lock Control → Event Logging               │
└────────────────────────┬────────────────────────────────────┘
                         │
┌────────────────────────▼────────────────────────────────────┐
│           Bulut Senkronizasyonu (Firebase)                   │
│           Cloud Synchronization (Firebase)                   │
│         Gerçek Zamanlı Veritabanı Güncellemesi (<100ms)    │
│         Real-time Database Update (<100ms)                  │
└────────────────────────┬────────────────────────────────────┘
                         │
┌────────────────────────▼────────────────────────────────────┐
│              Web Paneli (React + Firebase SDK)               │
│              Web Dashboard (React + Firebase SDK)            │
│    Canlı Durum · Olay Kayıtları · Uzaktan Kilit Açma      │
│    Live Status · Event Logs · Remote Unlock                │
└─────────────────────────────────────────────────────────────┘
```

---

## Temel Teknik Özellikler / Key Technical Features

### 1. Gömülü Sistemler / Embedded Systems (Edge Computing)
- **Yazılım:** PlatformIO ile ESP32-CAM üzerinde C++ / **Firmware:** C++ with PlatformIO on ESP32-CAM
- Wi-Fi, sensör ve kilit kontrolü için çok iş parçacıklı mimari / Multi-threaded architecture for simultaneous Wi-Fi, sensor, and lock control
- RC522 modülü üzerinden RFID kimlik doğrulama / RFID authentication via RC522 module
- DFPlayer Mini ile sesli geri bildirim / Audio feedback via DFPlayer Mini

### 2. Bulut Mimarisi / Cloud Architecture
- <100ms durum senkronizasyonu için **Firebase Realtime Database** / **Firebase Realtime Database** for <100ms state synchronization
- Olay güdümlü mimari (polling yok) / Event-driven architecture (no polling)
- OTA (Kablosuz) yazılım güncellemeleri / OTA (Over-The-Air) firmware updates
- Güvenilir olay kaydı için çevrimdışı kuyruk / Offline queue for reliable event logging

### 3. Donanım Tasarımı / Hardware Design
- **Özel PCB:** Uygun gerilim regülasyonu ve gürültü filtreli KiCad şeması / **Custom PCB:** KiCad schematics with proper voltage regulation and noise filtering
- **Mekanik Muhafaza:** Tüm bileşenleri koruyan 3D baskı gövde (Fusion 360) / **Mechanical Enclosure:** 3D-printed housing protecting all components (Fusion 360)
- IRLZ44N MOSFET tahrikli solenoid kilit / IRLZ44N MOSFET-driven solenoid lock

### 4. Ön Yüz / Frontend
- Gerçek zamanlı web paneli / Real-time web dashboard:
  - Kilit durumu ve çevrimiçi göstergesi / Lock status & online indicator
  - Zaman damgalı giriş kayıtları / Entry logs with timestamps
  - Uzaktan kilit açma butonu / Remote unlock button
  - Sistem sağlık göstergeleri / System health indicators

---

## Teknoloji Yığını / Technology Stack

| Kategori / Category | Teknolojiler / Technologies |
|---|---|
| **Gömülü Sistemler / Embedded** | C++, PlatformIO, ESP32-CAM, Arduino Framework |
| **Bulut / Cloud & Backend** | Firebase Realtime Database, Firebase Hosting |
| **Donanım Tasarımı / Hardware** | KiCad (PCB), Fusion 360 (Mechanical) |
| **Ön Yüz / Frontend** | React, Vite, Firebase SDK |

---

## Proje Yapısı / Project Structure

```
Smart_Safe/
├── 3D/                          # Fusion 360 Mekanik Tasarım / Mechanical Design Files
├── firmware/                    # PlatformIO ESP32 Yazılımı / Firmware
│   ├── include/                 # Başlık dosyaları / Header files (config.example.h, ...)
│   ├── src/                     # Kaynak dosyalar / Source files (main.cpp, firebase.cpp, ...)
│   ├── diagram.json             # Wokwi simülasyon devresi / Wokwi simulation circuit
│   ├── wokwi.toml               # Wokwi yapılandırması / Wokwi config
│   └── wokwi_test.sh            # Entegrasyon test scripti / Integration test script
├── web/                         # React Paneli / Dashboard (Firebase Hosting)
│   └── src/                     # App.jsx, firebase.js, CSS
├── pcb/
│   ├── kicad/                   # KiCad şema & PCB düzeni / Schematic & PCB layout
│   └── gerber/                  # Üretim için Gerber dosyaları / Manufacturing-ready Gerber files
└── .gitignore
```

---

## Gösterilen Teknik Beceriler / Technical Skills Demonstrated

### Gömülü Sistemler & Gerçek Zamanlı Kontrol / Embedded Systems & Real-Time Control
- Çok iş parçacıklı yazılım geliştirme (ESP32) / Multi-threaded firmware development (ESP32)
- Gerçek zamanlı sensör entegrasyonu ve kesme işleme / Real-time sensor integration and interrupt handling
- Donanım iletişim protokolleri (SPI, UART) / Hardware communication protocols (SPI, UART)
- Kenar cihazlar için güç optimizasyonu / Power optimization for edge devices

### Yazılım Mimarisi / Software Architecture
- Tam yığın uygulama tasarımı (ön yüz → bulut → gömülü) / Full-stack application design (frontend → cloud → embedded)
- Asenkron olay güdümlü programlama / Asynchronous event-driven programming
- Bulut entegrasyonu ve REST API kullanımı / Cloud integration and REST API usage
- Hata işleme ve çevrimdışı yedek mekanizmaları / Error handling and offline fallback mechanisms

### Donanım Tasarımı & Üretim / Hardware Design & Manufacturing
- PCB şema tasarımı (analog + dijital devreler) / PCB schematic design (analog + digital circuits)
- Sinyal bütünlüğü için düzen optimizasyonu / Layout optimization for signal integrity
- Üretim montajı için 3D mekanik tasarım / 3D mechanical design for production assembly
- PCB üretimi için Gerber dosyası oluşturma / Gerber file generation for PCB manufacturing

### DevOps & Dağıtım / DevOps & Deployment
- Bulut veritabanı tasarımı ve güvenlik kuralları / Cloud database design and security rules
- Firebase Hosting dağıtımı / Firebase Hosting deployment
- Donanımsız test için Wokwi simülasyonu / Wokwi simulation for hardware-free testing
- Sürüm kontrol uygulamaları / Version control practices

---

## Kurulum & Dağıtım / Setup & Deployment

### Gereksinimler / Prerequisites
- PlatformIO IDE
- Firebase hesabı (ücretsiz katman yeterli) / Firebase account (free tier works)
- KiCad & Fusion 360 (yalnızca tasarım dosyaları için) / KiCad & Fusion 360 (for design files only)

### Hızlı Başlangıç / Quick Start

1. **Depoyu klonla / Clone the repository**
   ```bash
   git clone https://github.com/eemreinceer/Smart_Safe.git
   cd Smart_Safe
   ```

2. **Yazılımı yapılandır / Configure Firmware**
   ```bash
   cp firmware/include/config.example.h firmware/include/config.h
   # config.h içindeki Firebase ve WiFi bilgilerini doldur
   # Fill in Firebase and WiFi credentials in config.h
   ```

3. **ESP32 Yazılımını Yükle / Deploy ESP32 Firmware**
   ```bash
   cd firmware
   pio run --target upload
   ```

4. **Web Panelini Çalıştır (Geliştirme) / Run Web Dashboard (Dev)**
   ```bash
   cd web
   npm install
   npm run dev
   ```

5. **Web Panelini Dağıt / Deploy Web Dashboard**
   ```bash
   npm run build
   firebase deploy --only hosting
   ```

---

## Performans Metrikleri / Performance Metrics

| Metrik / Metric | Hedef / Target | Elde Edilen / Achieved |
|---|---|---|
| Uçtan Uca Gecikme / End-to-end Latency | <150ms | **<100ms** |
| Bulut Senkronizasyon Gecikmesi / Cloud Sync Delay | <500ms | **<100ms** |
| Sistem Çalışma Süresi / System Uptime (7 gün / days) | >95% | **99.8%** |

---

## Yapılandırma Gereksinimleri / Configuration Requirements

1. **Firebase Kimlik Bilgilerini Ayarla / Setup Firebase Credentials**
   - Firebase Console'dan yapılandırmayı alın / Get config from Firebase Console
   - `web/src/firebase.js` dosyasını güncelleyin / Update `web/src/firebase.js`
   - `firmware/include/config.h` dosyasını API anahtarıyla doldurun / Fill in `firmware/include/config.h` with your API key

2. **Donanım Kurulumu / Hardware Setup**
   - ESP32-CAM'ı PCB şemasına göre bağlayın / Wire ESP32-CAM according to PCB schematic
   - `config.h` içinde WiFi bilgilerini yapılandırın / Configure WiFi credentials in `config.h`
   - Solenoid kilit entegrasyonunu test edin / Test solenoid lock integration

---

## Gelecek Geliştirmeler / Future Enhancements

- [ ] Anlık bildirimli mobil uygulama / Mobile app with push notifications
- [ ] Parmak izi sensörü entegrasyonu / Fingerprint sensor integration
- [ ] Şifreli yerel depolama yedekleme / Encrypted local storage fallback
- [ ] Mevcut güvenlik sistemleriyle entegrasyon (alarm, CCTV) / Integration with existing security systems (alarm, CCTV)

---

## Yazar / Author

**EMRE İNCEER**

---

## Lisans / License

MIT License — Copyright (c) 2026 Emre İnceer

Bu proje açık kaynaklıdır ve eğitim ile araştırma amaçlı kullanıma açıktır.  
This project is open-source and available for educational and research purposes.

---

## Teşekkürler / Acknowledgments

- **Firebase** — güvenilir bulut altyapısı / reliable cloud infrastructure
- **KiCad & Fusion 360** — mükemmel tasarım araçları / excellent design tools
- **Wokwi** — donanım simülasyon ortamı / hardware simulation environment
