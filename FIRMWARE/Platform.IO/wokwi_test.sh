#!/bin/bash
# Wokwi entegrasyon test scripti

set -eu

: "${WOKWI_CLI_TOKEN:?WOKWI_CLI_TOKEN ortam degiskeni tanimli olmali}"

LOG=$(mktemp)
PIPE=$(mktemp -u)
WOKWI_PID=""

cleanup() {
    exec 3>&- 2>/dev/null || true
    if [ -n "$WOKWI_PID" ]; then kill "$WOKWI_PID" 2>/dev/null || true; fi
    rm -f "$PIPE" "$LOG"
}
trap cleanup EXIT INT TERM

mkfifo "$PIPE"

cd "$(dirname "$0")"

# Pipe'ı açık tut (writer process)
exec 3>"$PIPE"

pio run -e esp32cam

# Wokwi'yi başlat
wokwi-cli --interactive . < "$PIPE" > "$LOG" 2>&1 &
WOKWI_PID=$!
echo "[TEST] Wokwi PID: $WOKWI_PID"

# SMART SAFE READY bekle
echo "[TEST] ESP32 açılışı bekleniyor..."
for i in $(seq 1 60); do
    if grep -q "SMART SAFE READY" "$LOG" 2>/dev/null; then
        echo "[TEST] ✅ ESP32 hazır!"
        break
    fi
    sleep 2
done

if ! grep -q "SMART SAFE READY" "$LOG"; then
    echo "[TEST] ESP32 hazir olmadi" >&2
    exit 1
fi

# === TEST 1: YETKİLİ RFID ===
echo ""
echo "[TEST] === TEST 1: Yetkili RFID ==="
echo "RFID:A1B2C3D4" >&3
sleep 8

grep -q "Yetkili giris: KART-1" "$LOG" || {
    echo "[TEST] Yetkili RFID sonucu gorulmedi" >&2
    exit 1
}

echo "[TEST] --- Log (TEST 1 sonrası) ---"
tail -20 "$LOG"

# === TEST 2: YETKİSİZ RFID ===
echo ""
echo "[TEST] === TEST 2: Yetkisiz RFID ==="
echo "RFID:DEADBEEF" >&3
sleep 8

grep -q "YETKISIZ ERISIM" "$LOG" || {
    echo "[TEST] Yetkisiz RFID sonucu gorulmedi" >&2
    exit 1
}

echo "[TEST] --- Log (TEST 2 sonrası) ---"
tail -20 "$LOG"

echo ""
echo "[TEST] Testler başarılı."
