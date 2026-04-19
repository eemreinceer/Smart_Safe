🛡️ SmartSafe: AI-Powered Hybrid IoT Security Ecosystem
SmartSafe is an end-to-end security solution that combines biometric authentication (Face ID), cloud-based real-time monitoring, and custom hardware design. This project integrates software architecture, deep learning, embedded systems, and industrial design into a single cohesive solution.

🚀 Project Overview
This system uses a high-accuracy facial recognition engine to prevent unauthorized access. While it handles the image processing load on a central unit (PC/Server), it performs physical lock control and sensor management on the edge device (ESP32). The entire ecosystem can be monitored and controlled from anywhere in the world via a modern web dashboard.

⚠️ Important Notes
•    Customization: To use the system, you must add your own photo named patron.jpg to the src/python folder.
•    Firebase Configuration: To run this project, you must add your own Firebase credentials to the web/script.js file and the serviceAccountKey.json file in the root directory.

🌟 Key Features
•    AI Face ID: Real-time face verification with over 98% accuracy using the DeepFace (Facenet) architecture.
•    Hybrid Communication: Data synchronization with <100ms latency between the Python backend and ESP32 firmware via the Firebase Realtime Database.
•    Embedded System (Firmware): ESP32 software coded in PlatformIO (C++), capable of Wi-Fi communication and multitasking.
•    Custom Hardware Design: Professional PCB schematics and Gerber files designed using KiCad.
•    Mechanical Design: 3D-printable enclosure models created in Fusion 360 that protect all components.

🛠️ Technology Stack
•    AI & Image Processing: Python, OpenCV, DeepFace, TensorFlow
•    Embedded Systems: C++, PlatformIO, ESP32, ESP32-CAM
•    Cloud & Backend: Firebase Realtime Database, Firebase Hosting
•    Design: KiCad (Electronics), Fusion 360 (Mechanical)




📂 Project Structure
Plaintext
SmartSafe/
├── 📁 3D/                   # Fusion 360 Mechanical Design Files
├── 📁 PCB/                  # KiCad Circuit Design and Gerber Files
├── 📁 src/
│   ├── 📁 python/           # AI Face Recognition and Firebase Services
│   └── 📁 PlatformIO/       # ESP32 C++ Firmware (Sensor and Lock Control)
├── 📁 web/                  # Firebase Hosting Dashboard (Frontend)
├── 📄 requirements.txt      # Python Dependencies
└── 📄 .gitignore            # Security and Cache Filters


graph TD
    subgraph "ALGILAMA KATMANI (Perception)"
        A1[ESP32-CAM / Kamera] -->|Görüntü Akışı| B
        A2[Parmak İzi Sensörü] -->|Biyometrik Veri| G
    end

    subgraph "YAPAY ZEKA VE KARAR (Python Backend)"
        B[OpenCV & DeepFace] -->|Facenet Analizi| C{Doğrulama}
        C -->|Onay| D[Firebase Realtime DB]
        C -->|Red| E[Hata Logu / Fotoğraf]
    end

    subgraph "BULUT VE KONTROL PANELİ (Cloud & UI)"
        D <-->|Çift Yönlü Veri| F[Web Dashboard]
        D <-->|Sinyal Akışı| G[ESP32 Gömülü Yazılım]
    end

    subgraph "GÜÇ VE AKTÜATÖR (Hardware)"
        G -->|GPIO Tetik| H[Röle Modülü]
        H -->|12V DC| I[Solenoid Kilit]
        J[LM2596 Regülatör] -->|Sabit Güç| G
    end

    %% Renklendirme
    style D fill:#FFCA28,stroke:#FFA000,color:#000
    style B fill:#42A5F5,stroke:#1E88E5,color:#fff
    style G fill:#66BB6A,stroke:#43A047,color:#fff
    style I fill:#EF5350,stroke:#E53935,color:#fff


⚙️ How Does the System Work? (Architecture)
1.    Input: The video stream captured by the camera is sent to the Python backend.
2.    AI Analysis: The DeepFace model compares the captured face with the pattern.jpg reference.
3.    Cloud Synchronization: If verification is successful, the `is_locked` value on Firebase is updated.
4.    Physical Action: The ESP32 detects the database change in real-time and triggers the solenoid lock.
5.    User Interface: The lock status is updated on the Web Dashboard, and the entry photo is logged.

🎓 Skills Gained
•    Full-Stack IoT Integration: Seamless coordination of hardware, software, and cloud services.
•    Edge Computing Principles: Distributing heavy computational loads (AI) to appropriate units.
•    Industrial Product Development: Transforming an idea into a physical product through PCB and 3D design phases.
________________________________________
Prepared by: EMRE İNCEER.
