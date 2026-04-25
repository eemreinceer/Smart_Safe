// Firebase Ayarları
const firebaseConfig = {
  apiKey: "YOUR_API_KEY",
  authDomain: "YOUR_PROJECT_ID.firebaseapp.com",
  databaseURL: "https://YOUR_PROJECT_ID-default-rtdb.firebaseio.com",
  projectId: "YOUR_PROJECT_ID",
  storageBucket: "YOUR_PROJECT_ID.appspot.com",
  messagingSenderId: "YOUR_SENDER_ID",
  appId: "YOUR_APP_ID"
};

// Firebase'i Başlat
firebase.initializeApp(firebaseConfig);
const db = firebase.database();

// Elementler
const lockText = document.getElementById('lock-text');
const lockIcon = document.getElementById('lock-icon');
const statusBadge = document.getElementById('status-badge');
const logsContainer = document.getElementById('logs-container');
const unlockBtn = document.getElementById('unlock-btn');

db.ref('/safe_001/status').on('value', (snapshot) => {
    const data = snapshot.val();
    if (!data) return;

    // Online/Offline Durumu
    if(data.is_online) {
        statusBadge.innerText = "SİSTEM AKTİF";
        statusBadge.className = "badge online";
    } else {
        statusBadge.innerText = "CİHAZ ÇEVRİMDIŞI";
        statusBadge.className = "badge offline";
    }

    // Kilit Durumu
    if(data.is_locked) {
        lockText.innerText = "KİLİTLİ";
        lockIcon.innerText = "🔒";
        document.body.classList.remove('unlocked');
    } else {
        lockText.innerText = "AÇIK";
        lockIcon.innerText = "🔓";
        document.body.classList.add('unlocked');
    }
});

// Uzaktan Kilit Açma
unlockBtn.addEventListener('click', () => {
    
    db.ref('/safe_001/control').update({ remote_unlock: true });
    alert("Açma komutu gönderildi!");
});

// Logları Getir
db.ref('/safe_001/logs').limitToLast(5).on('value', (snapshot) => {
    logsContainer.innerHTML = "";
    snapshot.forEach((child) => {
        const log = child.val();
        const date = new Date(log.timestamp * 1000).toLocaleString();
        const html = `
            <div class="log-item">
                <b>⏰ ${date}</b><br>
                👤 Yöntem: ${log.method}<br>
                <img src="${log.photo_base64}" style="width:100%; border-radius:8px; margin-top:5px;">
            </div>
        `;
        logsContainer.insertAdjacentHTML('afterbegin', html);
    });
});