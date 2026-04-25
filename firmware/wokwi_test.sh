#!/bin/bash
# Wokwi entegrasyon test scripti
# Token almak için: https://wokwi.com/dashboard/ci

export WOKWI_CLI_TOKEN="YOUR_WOKWI_CLI_TOKEN_HERE"
LOG=/tmp/wokwi.log
PIPE=/tmp/wokwi_in

rm -f "$PIPE" "$LOG"
mkfifo "$PIPE"

cd "$(dirname "$0")"

# Pipe'ı açık tut (writer process)
exec 3>"$PIPE"

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

# === TEST 1: YETKİLİ RFID ===
echo ""
echo "[TEST] === TEST 1: Yetkili RFID ==="
echo "RFID:00000000" >&3
sleep 8

echo "[TEST] --- Log (TEST 1 sonrası) ---"
tail -20 "$LOG"

# === TEST 2: YETKİSİZ RFID ===
echo ""
echo "[TEST] === TEST 2: Yetkisiz RFID ==="
echo "RFID:DEADBEEF" >&3
sleep 8

echo "[TEST] --- Log (TEST 2 sonrası) ---"
tail -20 "$LOG"

# === TEST 3: UZAKTAN KİLİT AÇMA ===
echo ""
echo "[TEST] === TEST 3: Uzaktan kilit açma (Firebase üzerinden) ==="
curl -s -X PUT \
  "${DATABASE_URL}/safe_001/control/alarm.json" \
  -d '"REMOTE_UNLOCK"' \
  -H "Content-Type: application/json"
echo ""
echo "[TEST] Firebase'e REMOTE_UNLOCK yazıldı, 12 saniye bekleniyor..."
sleep 12

echo "[TEST] --- Son 30 satır log ---"
tail -30 "$LOG"

exec 3>&-
kill $WOKWI_PID 2>/dev/null
echo ""
echo "[TEST] Test tamamlandı."
