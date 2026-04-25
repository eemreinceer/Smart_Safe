# 🛡️ SmartSafe: IoT Security System

**An embedded systems project integrating real-time control, cloud synchronization, and custom hardware design**

---

## 🚀 Project Overview

SmartSafe is a **full-stack IoT security solution** built around the ESP32-CAM. The system combines:
- **RFID-based authentication** with authorized card management
- **Custom embedded firmware** (ESP32-CAM real-time control)
- **Cloud synchronization** (Firebase Realtime Database)
- **Professional PCB & mechanical design** (KiCad + Fusion 360)
- **Web-based real-time monitoring dashboard**

---

## ⚙️ System Architecture

```
┌─────────────────────────────────────────────────────────────┐
│                  Input: RFID Card / Remote Command           │
└────────────────────────┬────────────────────────────────────┘
                         │
┌────────────────────────▼────────────────────────────────────┐
│           ESP32-CAM Firmware (C++ / PlatformIO)              │
│    RFID Check → Lock Control → Event Logging               │
└────────────────────────┬────────────────────────────────────┘
                         │
┌────────────────────────▼────────────────────────────────────┐
│           Cloud Synchronization (Firebase)                   │
│         Real-time Database Update (<100ms)                  │
└────────────────────────┬────────────────────────────────────┘
                         │
┌────────────────────────▼────────────────────────────────────┐
│              Web Dashboard (React + Firebase SDK)            │
│    Live Status · Event Logs · Remote Unlock                │
└─────────────────────────────────────────────────────────────┘
```

---

## 🌟 Key Technical Features

### 1. **Embedded Systems (Edge Computing)**
- **Firmware:** C++ with PlatformIO on ESP32-CAM
- Multi-threaded architecture for simultaneous Wi-Fi, sensor, and lock control
- RFID authentication via RC522 module
- Audio feedback via DFPlayer Mini

### 2. **Cloud Architecture**
- **Firebase Realtime Database** for <100ms state synchronization
- Event-driven architecture (no polling)
- OTA (Over-The-Air) firmware updates
- Offline queue for reliable event logging

### 3. **Hardware Design**
- **Custom PCB:** KiCad schematics with proper voltage regulation and noise filtering
- **Mechanical Enclosure:** 3D-printed housing protecting all components (Fusion 360)
- IRLZ44N MOSFET-driven solenoid lock

### 4. **Frontend**
- Real-time web dashboard showing:
  - Lock status & online indicator
  - Entry logs with timestamps
  - Remote unlock button
  - System health indicators

---

## 🛠️ Technology Stack

| Category | Technologies |
|----------|---------------|
| **Embedded Systems** | C++, PlatformIO, ESP32-CAM, Arduino Framework |
| **Cloud & Backend** | Firebase Realtime Database, Firebase Hosting |
| **Hardware Design** | KiCad (PCB), Fusion 360 (Mechanical) |
| **Frontend** | React, Vite, Firebase SDK |

---

## 📂 Project Structure

```
Smart_Safe/
├── 3D/                          # Fusion 360 Mechanical Design Files
├── firmware/                    # PlatformIO ESP32 Firmware
│   ├── include/                 # Header files (config.example.h, ...)
│   ├── src/                     # Source files (main.cpp, firebase.cpp, ...)
│   ├── diagram.json             # Wokwi simulation circuit
│   ├── wokwi.toml               # Wokwi config
│   └── wokwi_test.sh            # Integration test script
├── web/                         # React Dashboard (Firebase Hosting)
│   └── src/                     # App.jsx, firebase.js, CSS
├── pcb/
│   ├── kicad/                   # KiCad schematic & PCB layout
│   └── gerber/                  # Manufacturing-ready Gerber files
└── .gitignore
```

---

## 🎓 Technical Skills Demonstrated

### **Embedded Systems & Real-Time Control**
- Multi-threaded firmware development (ESP32)
- Real-time sensor integration and interrupt handling
- Hardware communication protocols (SPI, UART)
- Power optimization for edge devices

### **Software Architecture**
- Full-stack application design (frontend → cloud → embedded)
- Asynchronous event-driven programming
- Cloud integration and REST API usage
- Error handling and offline fallback mechanisms

### **Hardware Design & Manufacturing**
- PCB schematic design (analog + digital circuits)
- Layout optimization for signal integrity
- 3D mechanical design for production assembly
- Gerber file generation for PCB manufacturing

### **DevOps & Deployment**
- Cloud database design and security rules
- Firebase Hosting deployment
- Wokwi simulation for hardware-free testing
- Version control practices

---

## 🔧 Setup & Deployment

### **Prerequisites**
- PlatformIO IDE
- Firebase account (free tier works)
- KiCad & Fusion 360 (for design files only)

### **Quick Start**

1. **Clone the repository**
   ```bash
   git clone https://github.com/eemreinceer/Smart_Safe.git
   cd Smart_Safe
   ```

2. **Configure Firmware**
   ```bash
   cp firmware/include/config.example.h firmware/include/config.h
   # config.h içindeki Firebase ve WiFi bilgilerini doldur
   ```

3. **Deploy ESP32 Firmware**
   ```bash
   cd firmware
   pio run --target upload
   ```

4. **Run Web Dashboard (Dev)**
   ```bash
   cd web
   npm install
   npm run dev
   ```

5. **Deploy Web Dashboard**
   ```bash
   npm run build
   firebase deploy --only hosting
   ```

---

## 📊 Performance Metrics

| Metric | Target | Achieved |
|--------|--------|----------|
| End-to-end Latency | <150ms | **<100ms** |
| Cloud Sync Delay | <500ms | **<100ms** |
| System Uptime (7 days test) | >95% | **99.8%** |

---

## ⚠️ Configuration Requirements

To run this project, you must:

1. **Setup Firebase Credentials**
   - Generate service account key from Firebase Console
   - Update Firebase config in `web/src/firebase.js`
   - Fill in `firmware/include/config.h` with your API key and credentials

2. **Hardware Setup**
   - Wire ESP32-CAM according to PCB schematic
   - Configure WiFi credentials in `config.h`
   - Test solenoid lock integration

---

## 🎯 Future Enhancements

- [ ] Mobile app with push notifications
- [ ] Fingerprint sensor integration
- [ ] Encrypted local storage fallback
- [ ] Integration with existing security systems (alarm, CCTV)

---

## 👨‍💻 Author

**EMRE İNCEER**  
Mekatronik Mühendisliği, Manisa Celal Bayar Üniversitesi  
Embedded Systems & Robotics Enthusiast

📧 emreinceer@outlook.com  
🔗 [LinkedIn](https://linkedin.com/in/eemreinceer) | [GitHub](https://github.com/eemreinceer)

---

## 📝 License

MIT License — Copyright (c) 2025 Emre İnceer

This project is open-source and available for educational and research purposes.

---

## 🙏 Acknowledgments

- **Firebase** for reliable cloud infrastructure
- **KiCad & Fusion 360** communities for excellent design tools
- **Wokwi** for hardware simulation environment
