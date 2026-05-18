// ========================================
// SmartSafe AI — Dashboard Script
// Firebase Realtime + Auth + Modern UI
// ========================================

const firebaseConfig = {
  apiKey: "***REMOVED***",
  authDomain: "smartsafe-8f4f9.firebaseapp.com",
  databaseURL: "https://smartsafe-8f4f9-default-rtdb.firebaseio.com",
  projectId: "smartsafe-8f4f9",
  storageBucket: "smartsafe-8f4f9.firebasestorage.app",
  messagingSenderId: "648749418907",
  appId: "1:648749418907:web:a54f48cc2d23d63e07f93e",
  measurementId: "G-1QB5SZ3QMV"
};

firebase.initializeApp(firebaseConfig);
const db = firebase.database();
const auth = firebase.auth();

auth.setPersistence(firebase.auth.Auth.Persistence.SESSION);

// --- DOM Elements ---
const loginScreen = document.getElementById('login-screen');
const appContent = document.getElementById('app-content');
const loginForm = document.getElementById('login-form');
const loginEmail = document.getElementById('login-email');
const loginPass = document.getElementById('login-password');
const btnLogout = document.getElementById('btn-logout');

const lastMethodText = document.getElementById('last-method-text');
const lastTimeText = document.getElementById('last-time-text');
const logsContainer = document.getElementById('logs-container');
const logCount = document.getElementById('log-count');

const lockText = document.getElementById('lock-text');
const lockSub = document.getElementById('lock-sub');
const lockBadge = document.getElementById('lock-badge');
const lockRing = document.getElementById('lock-ring');
const svgLocked = document.getElementById('svg-locked');
const svgUnlocked = document.getElementById('svg-unlocked');

const statusBadge = document.getElementById('status-badge');
const sidebarDot = document.getElementById('sidebar-dot');
const deviceStatusText = document.getElementById('device-status-text');

const liveClock = document.getElementById('live-clock');
const currentDate = document.getElementById('current-date');
const unlockBtn = document.getElementById('unlock-btn');

// --- Confirm Modal Elements ---
const modalOverlay = document.getElementById('modal-overlay');
const modalCancel = document.getElementById('modal-cancel');
const modalConfirm = document.getElementById('modal-confirm');

// --- AUTH ---
auth.onAuthStateChanged((user) => {
    if (user) {
        loginScreen.style.setProperty('display', 'none', 'important');
        appContent.style.setProperty('display', 'flex', 'important');
        startDashboard();
    } else {
        loginScreen.style.setProperty('display', 'flex', 'important');
        appContent.style.setProperty('display', 'none', 'important');
        stopDashboard();
    }
});

loginForm.addEventListener('submit', (e) => {
    e.preventDefault();
    auth.signInWithEmailAndPassword(loginEmail.value, loginPass.value)
        .catch(err => alert("Hata: " + err.message));
});

btnLogout.addEventListener('click', (e) => { e.preventDefault(); auth.signOut(); });

// --- DASHBOARD CORE ---
let statusRef = null;
let logsRef = null;
let lastStatusData = null;
let countdownInterval = null;
let isOnline = false;

function startDashboard() {
    updateClock();
    updateDate();
    setInterval(updateClock, 1000);

    updateUI(null);

    statusRef = db.ref('/safe_001/status');
    statusRef.on('value', (snapshot) => {
        lastStatusData = snapshot.val();
        if (!countdownInterval) {
            updateUI(lastStatusData);
        }
    });

    // SON 10 KAYIT İÇİN KESİN KOMUT
    logsRef = db.ref('/safe_001/logs');
    logsRef.orderByChild('timestamp').limitToLast(10).on('value', (snapshot) => {
        console.log("Log güncellendi, kayıt sayısı:", snapshot.numChildren());
        updateLogs(snapshot);
    });

    setInterval(() => { if (!countdownInterval) updateUI(lastStatusData); }, 5000);
}

function stopDashboard() {
    if (statusRef) statusRef.off();
    if (logsRef) logsRef.off();
}

// --- UI UPDATES ---
function updateUI(data) {
    const statusText = statusBadge.querySelector('.topbar__status-text');
    if (!data) {
        statusBadge.className = 'topbar__status-pill offline';
        statusText.textContent = 'Cihaz Çevrimdışı';
        setLock(true);
        return;
    }
    
    const now = Math.floor(Date.now() / 1000);
    const lastSeen = data.last_seen || 0;
    const deviceOnline = data.is_online && (now - lastSeen < 60);

    const sidebarStatusText = document.getElementById('sidebar-status-text');
    const sidebarDotEl = document.getElementById('sidebar-dot');

    if (deviceOnline) {
        isOnline = true;
        statusBadge.className = 'topbar__status-pill online';
        statusText.textContent = 'Sistem Çevrimiçi';
        if (sidebarStatusText) sidebarStatusText.textContent = 'Çevrimiçi';
        if (sidebarDotEl) sidebarDotEl.classList.add('online');
        setLock(data.is_locked);
    } else {
        isOnline = false;
        statusBadge.className = 'topbar__status-pill offline';

        statusText.textContent = 'Cihaz Çevrimdışı';
        if (sidebarStatusText) sidebarStatusText.textContent = 'Çevrimdışı';
        if (sidebarDotEl) sidebarDotEl.classList.remove('online');
        setLock(true);
    }
}

function setLock(locked) {
    lockText.textContent = locked ? 'KİLİTLİ' : 'AÇIK';
    lockBadge.textContent = locked ? 'KİLİTLİ' : 'AÇIK';
    
    if (locked) {
        lockSub.textContent = 'Kasa güvenli durumda';
        lockSub.style.color = 'var(--text-muted)';
        lockText.style.color = '';
        lockBadge.style.background = '';
        lockBadge.style.color = '';
        lockRing.className = 'lock-ring locked';
        svgLocked.style.display = 'block';
        svgUnlocked.style.display = 'none';
    } else {
        lockSub.textContent = 'Kasa şu an erişime açık';
        lockSub.style.color = '#10b981';
        lockText.style.color = '#10b981';
        lockBadge.style.background = 'rgba(16, 185, 129, 0.15)';
        lockBadge.style.color = '#10b981';
        lockRing.className = 'lock-ring unlocked';
        svgLocked.style.display = 'none';
        svgUnlocked.style.display = 'block';
    }
}

function updateLogs(snapshot) {
    logsContainer.innerHTML = '';
    const items = [];
    
    snapshot.forEach(child => {
        items.push(child.val());
    });
    
    // En yeni en üstte olacak şekilde sırala
    items.sort((a, b) => b.timestamp - a.timestamp);

    if (items.length === 0) {
        logsContainer.innerHTML = '<div class="logs-empty"><p>Henüz kayıt yok</p></div>';
        logCount.textContent = '0 kayıt';
        return;
    }

    // Üstteki Stat Kartlarını Güncelle
    const latest = items[0];
    const date = new Date(latest.timestamp * 1000);
    let mStr = latest.event === 'AUTHORIZED' ? ('RFID: ' + (latest.method || '?')) :
               latest.event === 'UNAUTHORIZED' ? 'Yetkisiz Giriş' : (latest.method || '?');
    lastMethodText.textContent = mStr;
    lastTimeText.textContent = date.toLocaleString('tr-TR', { day: '2-digit', month: 'short', hour: '2-digit', minute: '2-digit' });

    // Listeyi Doldur
    items.forEach(log => {
        const d = new Date(log.timestamp * 1000);
        let tagClass = 'log-item__method-tag--unknown';
        let mTag = log.method || 'Bilinmiyor';

        if (log.event === 'AUTHORIZED' && log.method === 'REMOTE') {
            tagClass = 'log-item__method-tag--remote'; mTag = 'Uzaktan Erişim';
        } else if (log.event === 'AUTHORIZED') {
            tagClass = 'log-item__method-tag--face'; mTag = 'RFID: ' + log.method;
        } else if (log.event === 'UNAUTHORIZED') {
            tagClass = 'log-item__method-tag--danger'; mTag = 'Yetkisiz Giriş';
        }

        const photo = log.photo_base64 && log.photo_base64.length > 30 ? log.photo_base64 : 'https://via.placeholder.com/72';

        const html = `
            <div class="log-item">
                <img class="log-item__photo" src="${photo}" alt="Foto" onerror="this.src='https://via.placeholder.com/72'">
                <div class="log-item__info">
                    <span class="log-item__time">${d.toLocaleTimeString('tr-TR', {hour:'2-digit', minute:'2-digit'})}</span>
                    <span class="log-item__method-tag ${tagClass}">${mTag}</span>
                </div>
            </div>`;
        logsContainer.insertAdjacentHTML('beforeend', html);
    });
    logCount.textContent = items.length + " kayıt";
}

// --- UZAKTAN AÇMA ---
unlockBtn.addEventListener('click', () => {
    if (!isOnline) {
        showToast('Cihaz çevrimdışı olduğu için kilidi açamazsınız.', 'error');
        return;
    }
    modalOverlay.classList.add('active');
});

modalCancel.addEventListener('click', () => {
    modalOverlay.classList.remove('active');
});

modalOverlay.addEventListener('click', (e) => {
    if (e.target === modalOverlay) {
        modalOverlay.classList.remove('active');
    }
});

modalConfirm.addEventListener('click', () => {
    modalOverlay.classList.remove('active');
    
    if (countdownInterval) clearInterval(countdownInterval);
    
    let timeLeft = 20;
    countdownInterval = true;
    
    setLock(false);
    lockSub.textContent = `Kasa 20 sn içinde kilitlenecek`;
    lockSub.style.color = '#f59e0b';
    
    db.ref('/safe_001/status').update({ is_locked: false });
    db.ref('/safe_001/control/alarm').set('REMOTE_UNLOCK');
    
    // Log kaydını kesin olarak yap
    db.ref('/safe_001/logs').push({
        method: "REMOTE_CONTROL",
        timestamp: Math.floor(Date.now() / 1000),
        photo_base64: ""
    });

    countdownInterval = setInterval(() => {
        timeLeft--;
        if (lockSub) lockSub.textContent = `Kasa ${timeLeft} sn içinde kilitlenecek`;
        if (timeLeft <= 0) {
            clearInterval(countdownInterval);
            countdownInterval = null;
            db.ref('/safe_001/status').update({ is_locked: true });
            setLock(true);
        }
    }, 1000);

    showToast("Erişim açıldı!", "success");
});

function showToast(m, t) {
    const toast = document.createElement('div');
    const bg = t === 'error' ? '#ef4444' : '#10b981';
    toast.style.cssText = `position:fixed; bottom:20px; left:50%; transform:translateX(-50%); padding:12px 24px; background:${bg}; color:white; border-radius:12px; z-index:9999; font-size:14px; font-weight:500; box-shadow:0 4px 20px rgba(0,0,0,0.3);`;
    toast.textContent = m;
    document.body.appendChild(toast);
    setTimeout(() => toast.remove(), 3500);
}

function updateClock() { if(liveClock) liveClock.textContent = new Date().toLocaleTimeString('tr-TR'); }
function updateDate() { if(currentDate) currentDate.textContent = new Date().toLocaleDateString('tr-TR', {weekday:'long', year:'numeric', month:'long', day:'numeric'}); }

logsContainer.addEventListener('click', e => {
    if (e.target.tagName === 'IMG') {
        document.getElementById('zoomed-image').src = e.target.src;
        document.getElementById('image-modal-overlay').classList.add('active');
    }
});
document.getElementById('image-modal-close').onclick = () => document.getElementById('image-modal-overlay').classList.remove('active');