@echo off
title Akilli Kasa AI Kontrol Merkezi
color 0b
cls

echo ===================================================
echo       AKILLI KASA YAPAY ZEKA SISTEMI BASLATILIYOR
echo ===================================================
echo.
echo [+] Mevcut dizin kontrol ediliyor...
cd /d "%~dp0"

echo [+] Python sanal ortami ve script baslatiliyor...
cd Python
.\.venv\Scripts\python.exe sanal_kasa.py

if %ERRORLEVEL% neq 0 (
    echo.
    echo [!] HATA: Sistem beklenmedik bir sekilde durdu.
    pause
)