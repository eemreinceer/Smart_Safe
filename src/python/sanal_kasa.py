import cv2
from deepface import DeepFace
import firebase_admin
from firebase_admin import credentials, db
import base64
import time
import sys
import os  

# --- 0. DİNAMİK DOSYA YOLLARI AYARI ---
BASE_DIR = os.path.dirname(os.path.abspath(__file__))


JSON_PATH = os.path.join(BASE_DIR, "..", "..", "serviceAccountKey.json")


PATRON_RESIM = os.path.join(BASE_DIR, "patron.jpg")

# --- 1. FIREBASE BAĞLANTISI ---
if not firebase_admin._apps:
    try:
        cred = credentials.Certificate(JSON_PATH)
        firebase_admin.initialize_app(cred, {
            'databaseURL': 'BURAYA_DATABASE_URL_ADRESINI_YAZ' 
        })
    except Exception as e:
        print(f"❌ HATA: serviceAccountKey.json dosyası bulunamadı! Yol: {JSON_PATH}")
        sys.exit()

# --- GÜVENLİ KAPANIŞ FONKSİYONU ---
def guvenli_kapanis():
    print("\n🛑 Sistem sonlandırılıyor...")
    try:
        db.reference('/safe_001/status').update({
            'is_online': False,
            'is_locked': True
        })
        print("✅ Firebase güncellendi: Kasa KİLİTLENDİ ve OFFLINE oldu.")
    except Exception as e:
        print("Bağlantı hatası, son durum gönderilemedi.")

# ==========
# ANA SİSTEM 
# ==========
try:
    # --- SISTEM ONLINE BILDIRIMI ---
    db.reference('/safe_001/status').update({'is_online': True}) 
    print("🌐 Web Dashboard'a sinyal gönderildi: CİHAZ ONLINE!")

    # --- 2. AI HAZIRLIĞI ---
    face_cascade = cv2.CascadeClassifier(cv2.data.haarcascades + 'haarcascade_frontalface_default.xml')
    kilit_acilma_zamani = 0

    def unlock_safe(method="AI_FACE"):
        global kilit_acilma_zamani
        db.reference('/safe_001/status').update({'is_locked': False})
        kilit_acilma_zamani = time.time()
        
        ret, frame = cap.read()
        if ret:
            _, buffer = cv2.imencode('.jpg', frame)
            jpg_as_text = base64.b64encode(buffer).decode('utf-8')
            new_log = {
                'event': 'UNLOCKED',
                'method': method,
                'timestamp': int(time.time()),
                'photo_base64': f"data:image/jpeg;base64,{jpg_as_text}"
            }
            db.reference('/safe_001/logs').push(new_log)
        print(f"🔓 KASA AÇILDI! Yöntem: {method}. 20 sn sonra kilitlenecek.")

    def lock_safe():
        db.reference('/safe_001/status').update({'is_locked': True})
        print("🔒 20 SANIYE DOLDU: Kasa otomatik kilitlendi!")

    # --- 3. KAMERA BAŞLATMA ---
    cap = cv2.VideoCapture(0, cv2.CAP_DSHOW)
    cap.set(cv2.CAP_PROP_FOURCC, cv2.VideoWriter_fourcc(*'MJPG'))
    cap.set(cv2.CAP_PROP_FRAME_WIDTH, 1280)
    cap.set(cv2.CAP_PROP_FRAME_HEIGHT, 720)
    cap.set(cv2.CAP_PROP_FPS, 30)

    time.sleep(2)

    print("🚀 AI Sistemi Başlıyor... HD Kamera Aktif.")
    print("⚠️ ÇIKMAK İÇİN KAMERA EKRANINDAYKEN 'Q' TUŞUNA BASIN!")

    last_check_time = 0

    while True:
        ret, frame = cap.read()
        if not ret: break

        gray = cv2.cvtColor(frame, cv2.COLOR_BGR2GRAY)
        faces = face_cascade.detectMultiScale(gray, 1.1, 5, minSize=(60, 60))

        # Firebase'den kilit durumunu oku
        current_status = db.reference('/safe_001/status/is_locked').get()

        # 20 SN OTOMATİK KİLİT KONTROLÜ
        if current_status == False and kilit_acilma_zamani > 0:
            if time.time() - kilit_acilma_zamani >= 20:
                lock_safe()
                kilit_acilma_zamani = 0

        if len(faces) > 0:
            (x, y, w, h) = faces[0]
            cv2.rectangle(frame, (x, y), (x+w, y+h), (255, 0, 0), 2)

            if current_status == True:
                if time.time() - last_check_time > 1.0:
                    try:
                        cropped_face = frame[y:y+h, x:x+w]
                      
                        result = DeepFace.verify(img1_path=cropped_face, img2_path=PATRON_RESIM, enforce_detection=False, model_name="Facenet")
                        
                        if result["verified"]:
                            print("✅ KİMLİK DOĞRULANDI: Merhaba Patron!")
                            unlock_safe(method="FACE_ID")
                        else:
                            print("🚫 YABANCI: Erişim Reddedildi.")
                        
                        last_check_time = time.time()
                    except Exception as e:
                        pass 

        # Uzaktan kontrolü dinle
        remote_command = db.reference('/safe_001/control/remote_unlock').get()
        if remote_command == True:
            print("📲 Uzaktan açma komutu alındı!")
            unlock_safe(method="REMOTE_CONTROL")
            db.reference('/safe_001/control/remote_unlock').set(False)

        cv2.imshow('Akilli Kasa AI - SmartSafe', frame)
        
        if cv2.waitKey(1) & 0xFF == ord('q'): 
            break

except KeyboardInterrupt:
    print("\n⚠️ Program durduruldu!")
finally:
    if 'cap' in locals():
        cap.release()
    cv2.destroyAllWindows()
    guvenli_kapanis() 
    time.sleep(1)