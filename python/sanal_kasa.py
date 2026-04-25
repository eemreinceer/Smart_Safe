import os
import cv2
import firebase_admin
from firebase_admin import credentials, db
import base64
import time
import sys
import threading
import queue

# Windows terminal emoji/unicode desteği ve anlık çıktı (buffering kapatma)
sys.stdout.reconfigure(encoding='utf-8', line_buffering=True)
sys.stderr.reconfigure(encoding='utf-8', line_buffering=True)

print("🚀 Sistem başlatılıyor... Lütfen bekleyin.")

# --- 1. FIREBASE BAĞLANTISI ---
if not firebase_admin._apps:
    # Script'in çalıştığı klasörün tam yolunu otomatik bul
    current_dir = os.path.dirname(os.path.abspath(__file__))
    # JSON dosyasının tam yolunu oluştur
    key_path = os.path.join(current_dir, "serviceAccountKey.json")

    # Oluşturulan bu tam yolu kullan
    cred = credentials.Certificate(key_path)
    firebase_admin.initialize_app(cred, {
        'databaseURL': 'https://smartsafe-8f4f9-default-rtdb.firebaseio.com'
    })

# Paylaşılan Durum Değişkenleri
class SafeState:
    def __init__(self):
        self.is_locked = True
        self.remote_unlock_requested = False
        self.kilit_acilma_zamani = 0
        self.running = True
        self.lock = threading.Lock()

state = SafeState()

# --- GÜVENLİ KAPANIŞ FONKSİYONU ---
def guvenli_kapanis():
    print("\n🛑 Sistem sonlandırılıyor...")
    state.running = False
    try:
        db.reference('/safe_001/status').update({
            'is_online': False,
            'is_locked': True
        })
        print("✅ Firebase güncellendi: Kasa KİLİTLENDİ ve OFFLINE oldu.")
    except Exception as e:
        print(f"Bağlantı hatası, son durum gönderilemedi: {e}")

# --- FIREBASE LISTENERS ---
def start_firebase_listeners():
    def on_status_change(event):
        with state.lock:
            if event.data is not None:
                state.is_locked = event.data

    def on_control_change(event):
        if event.data == True:
            with state.lock:
                state.remote_unlock_requested = True
            print("📲 Uzaktan açma komutu algılandı!")

    db.reference('/safe_001/status/is_locked').listen(on_status_change)
    db.reference('/safe_001/control/remote_unlock').listen(on_control_change)

# AI İşleme Kuyruğu
verification_queue = queue.Queue(maxsize=1)
verification_results = queue.Queue()

def ai_worker():
    print("🧠 AI Modeli yükleniyor (DeepFace/ArcFace)... Bu işlem biraz sürebilir.")
    from deepface import DeepFace
    print("✅ AI Modeli başarıyla yüklendi!")
    PATRON_RESIM = "patron.jpg"
    while state.running:
        try:
            frame_to_verify = verification_queue.get(timeout=1)
            # ArcFace modelini kullanıyoruz çünkü Facenet'ten daha kesindir.
            # detector_backend='skip' kullanıyoruz çünkü Cascade ile yüzü zaten kırptık.
            # Bu, DeepFace'in kırpılmış resim içinde tekrar yüz arayıp hata yapmasını engeller.
            result = DeepFace.verify(img1_path=frame_to_verify, img2_path=PATRON_RESIM, enforce_detection=False, model_name="ArcFace", detector_backend='skip')
            verification_results.put(result)
        except queue.Empty:
            continue
        except Exception as e:
            print(f"AI Hatası: {e}")

# --- KİLİT FONKSİYONLARI ---
def unlock_safe(cap, method="AI_FACE"):
    print(f"🔑 '{method}' ile kilit açma işlemi başlatıldı...")
    with state.lock:
        state.is_locked = False
        state.kilit_acilma_zamani = time.time()
    
    print("📡 Firebase status güncelleniyor...")
    db.reference('/safe_001/status').update({'is_locked': False})
    print("✅ Firebase status güncellendi.")
    
    ret, frame = cap.read()
    if ret:
        print("📸 Fotoğraf çekildi, log gönderiliyor...")
        _, buffer = cv2.imencode('.jpg', frame)
        jpg_as_text = base64.b64encode(buffer).decode('utf-8')
        new_log = {
            'event': 'UNLOCKED',
            'method': method,
            'timestamp': int(time.time()),
            'photo_base64': f"data:image/jpeg;base64,{jpg_as_text}"
        }
        db.reference('/safe_001/logs').push(new_log)
        print("✅ Log gönderildi.")
    else:
        print("⚠️ Fotoğraf çekilemedi, log gönderilemedi.")
    
    # 20 Saniye sonra otomatik kilitle
    import threading
    threading.Timer(20.0, lock_safe).start()
    print(f"🔓 KASA AÇILDI! Yöntem: {method}. 20 sn sonra kilitlenecek.")

def lock_safe():
    # Bu fonksiyon dışarıdan çağrıldığında kilitler
    db.reference('/safe_001/status').update({'is_locked': True})
    print("🔒 Kasa kilitlendi (Firebase güncellendi).")

def log_unauthorized_access(cap):
    print("⚠️ YETKİSİZ GİRİŞ TESPİT EDİLDİ! Fotoğraf gönderiliyor...")
    ret, frame = cap.read()
    if ret:
        _, buffer = cv2.imencode('.jpg', frame)
        jpg_as_text = base64.b64encode(buffer).decode('utf-8')
        new_log = {
            'event': 'UNAUTHORIZED',
            'method': 'AI_FACE_FAILED',
            'timestamp': int(time.time()),
            'photo_base64': f"data:image/jpeg;base64,{jpg_as_text}"
        }
        db.reference('/safe_001/logs').push(new_log)
        print("✅ Yetkisiz giriş logu gönderildi.")
    else:
        print("⚠️ Fotoğraf çekilemedi.")

def cleanup_old_logs():
    """24 saatten eski logları temizler"""
    while True:
        try:
            print("🧹 Eski loglar kontrol ediliyor...")
            cutoff = time.time() - 86400 # 24 saat öncesi
            logs_ref = db.reference('/safe_001/logs')
            # 24 saatten eski kayıtları bul
            old_logs = logs_ref.order_by_child('timestamp').end_at(cutoff).get()
            
            if old_logs:
                count = 0
                for key in old_logs:
                    logs_ref.child(key).delete()
                    count += 1
                print(f"🗑️ {count} adet eski log silindi.")
            else:
                print("✨ Silinecek eski log bulunamadı.")
        except Exception as e:
            print(f"❌ Temizlik hatası: {e}")
        
        time.sleep(3600) # Her saat başı kontrol et

# ==========
# ANA SİSTEM
# ==========
def main():
    try:
        print("🔧 Firebase bağlantısı kuruluyor...")
        # --- SISTEM ONLINE BILDIRIMI ---
        status_ref = db.reference('/safe_001/status')
        status_ref.update({'is_online': True})
        
        # SİSTEM ZORLA KAPATILDIĞINDA (Crash/Kill) durumu Web tarafındaki Heartbeat (last_seen) 
        # ile kontrol edildiği için on_disconnect kullanımına gerek kalmadı (Python Admin SDK'da desteklenmez).
        
        print("🌐 Web Dashboard'a sinyal gönderildi: CİHAZ ONLINE!")

        print("📡 Firebase dinleyicileri başlatılıyor...")
        start_firebase_listeners()
        
        print("🧠 AI Worker (DeepFace) başlatılıyor...")
        # AI Worker Başlat
        threading.Thread(target=ai_worker, daemon=True).start()

        print("🔍 Cascade Classifier yükleniyor...")
        cascade_path = cv2.data.haarcascades + 'haarcascade_frontalface_default.xml'
        print(f"📂 Cascade path: {cascade_path}")
        face_cascade = cv2.CascadeClassifier(cascade_path)
        if face_cascade.empty():
            print("❌ HATA: Cascade Classifier yüklenemedi!")
        else:
            print("✅ Cascade Classifier yüklendi.")
        
        print("📷 Kamera (ESP32-CAM) hazırlanıyor...")
        
        # ESP32-CAM IP'sini Firebase'den al
        esp32_ip = "192.168.1.100" # Varsayılan/Fallback
        try:
            ip_snapshot = db.reference('/safe_001/status/ip').get()
            if ip_snapshot:
                esp32_ip = ip_snapshot
                print(f"📡 ESP32-CAM IP'si algılandı: {esp32_ip}")
        except:
            print("⚠️ ESP32-CAM IP'si Firebase'den alınamadı, varsayılan kullanılıyor.")

        # MJPEG Stream URL'si
        stream_url = f"http://{esp32_ip}:81/stream"
        print(f"🔗 Akış adresi: {stream_url}")

        # --- KAMERA BAŞLATMA ---
        cap = cv2.VideoCapture(stream_url)
        
        if not cap.isOpened():
            print(f"❌ ESP32-CAM akışı açılamadı ({stream_url}).")
            print("🔄 Yerel kamera (Index 0) deneniyor...")
            cap = cv2.VideoCapture(0)
            
        if cap.isOpened():
            print("🚀 AI Sistemi Başlıyor... Kamera aktif.")
        else:
            print("🛑 KRİTİK HATA: Hiçbir kamera başlatılamadı!")
            return

        last_ai_time = 0
        ardisik_dogrulama = 0
        GEREKEN_DOGRULAMA = 1
        MAX_MESAFE = 0.80

        last_heartbeat = 0
        ardisik_hata = 0
        GEREKEN_HATA = 5 

        # 24 Saatlik Temizlik Görevlisini Başlat
        import threading
        threading.Thread(target=cleanup_old_logs, daemon=True).start()
        print("🚀 Sistem Hazır. (AI + Heartbeat + Cleanup aktif)")

        while state.running:
            if time.time() - last_heartbeat > 5:
                print(f"💓 Sistem aktif... (Kilit: {'KAPALI' if state.is_locked else 'AÇIK'})")
                # Heartbeat'i Firebase'e gönder
                try:
                    db.reference('/safe_001/status').update({
                        'last_seen': int(time.time()),
                        'is_online': True
                    })
                except Exception as e:
                    print(f"Heartbeat hatası: {e}")
                last_heartbeat = time.time()
            
            ret, frame = cap.read()
            if not ret: break

            display_frame = frame.copy()
            gray = cv2.cvtColor(frame, cv2.COLOR_BGR2GRAY)
            faces = face_cascade.detectMultiScale(gray, 1.1, 5, minSize=(100, 100))

            # Kilit Zaman Aşımı Kontrolü
            with state.lock:
                if not state.is_locked and state.kilit_acilma_zamani > 0:
                    if time.time() - state.kilit_acilma_zamani >= 20:
                        print("🔒 20 SANIYE DOLDU: Otomatik kilitleniyor...")
                        state.is_locked = True
                        state.kilit_acilma_zamani = 0
                        # Kilitleme işlemini Firebase'e loop dışında veya ayrı thread'de yapalım ki takılmasın
                        threading.Thread(target=lock_safe, daemon=True).start()

                # Uzaktan Açma Kontrolü
                do_unlock = False
                if state.remote_unlock_requested:
                    print("⚡ Loop içinde uzaktan açma isteği algılandı...")
                    state.remote_unlock_requested = False
                    do_unlock = True
            
            if do_unlock:
                unlock_safe(cap, method="REMOTE_CONTROL")
                db.reference('/safe_001/control/remote_unlock').set(False)
                print("✅ Uzaktan açma işlemi tamamlandı ve resetlendi.")

            # Yüz Tespiti ve AI Doğrulama
            if len(faces) > 1:
                for (fx, fy, fw, fh) in faces:
                    cv2.rectangle(display_frame, (fx, fy), (fx+fw, fy+fh), (0, 0, 255), 2)
                ardisik_dogrulama = 0
            elif len(faces) == 1:
                (x, y, w, h) = faces[0]
                cv2.rectangle(display_frame, (x, y), (x+w, y+h), (255, 0, 0), 2)

                if state.is_locked and time.time() - last_ai_time > 0.5:
                    if verification_queue.empty():
                        cropped_face = frame[y:y+h, x:x+w]
                        verification_queue.put(cropped_face)
                        last_ai_time = time.time()

            try:
                result = verification_results.get_nowait()
                mesafe = result.get("distance", 1.0)
                if result["verified"] and mesafe < MAX_MESAFE:
                    ardisik_dogrulama += 1
                    ardisik_hata = 0 # Hata sayacını sıfırla
                    print(f"✅ DOĞRULAMA BAŞARILI! (Mesafe: {mesafe:.4f})")
                    if ardisik_dogrulama >= GEREKEN_DOGRULAMA:
                        unlock_safe(cap, method="FACE_ID")
                        ardisik_dogrulama = 0
                else:
                    ardisik_dogrulama = 0
                    if state.is_locked: # Sadece kilitliyken hataları say
                        ardisik_hata += 1
                        print(f"❌ Tanınmayan Yüz! Mesafe: {mesafe:.4f} (Eşik: {MAX_MESAFE})")
                        print(f"⚠️ Hata Sayacı: {ardisik_hata}/{GEREKEN_HATA}")
                        if ardisik_hata >= GEREKEN_HATA:
                            log_unauthorized_access(cap)
                            ardisik_hata = 0 # Log attıktan sonra sıfırla
            except queue.Empty:
                pass

            cv2.putText(display_frame, f"Sistem: {'KILITLI' if state.is_locked else 'ACIK'}", (10, 30), cv2.FONT_HERSHEY_SIMPLEX, 1, (0, 255, 0) if not state.is_locked else (0, 0, 255), 2)
            cv2.imshow('Akilli Kasa AI - Performance Mode', display_frame)
            
            if cv2.waitKey(1) & 0xFF == ord('q'): 
                print("👋 'q' basıldı, çıkılıyor...")
                break

    except KeyboardInterrupt:
        pass
    finally:
        guvenli_kapanis()
        if 'cap' in locals():
            cap.release()
        cv2.destroyAllWindows()

if __name__ == "__main__":
    main()
