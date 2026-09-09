// ========================================
// SmartSafe — Dashboard Script
// Firebase Realtime + Auth + Modern UI
// ========================================

const firebaseConfig = window.SMARTSAFE_FIREBASE_CONFIG;
if (!firebaseConfig || !firebaseConfig.apiKey || !firebaseConfig.databaseURL) {
    throw new Error('firebase-config.js eksik veya gecersiz');
}

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
let isOnline = false;
let clockInterval = null;
let refreshInterval = null;

function startDashboard() {
    updateClock();
    updateDate();
    if (!clockInterval) clockInterval = setInterval(updateClock, 1000);

    updateUI(null);

    statusRef = db.ref('/safe_001/status');
    statusRef.on('value', (snapshot) => {
        lastStatusData = snapshot.val();
        updateUI(lastStatusData);
    });

    // SON 10 KAYIT İÇİN KESİN KOMUT
    logsRef = db.ref('/safe_001/logs');
    logsRef.orderByChild('timestamp').limitToLast(10).on('value', (snapshot) => {
        console.log("Log güncellendi, kayıt sayısı:", snapshot.numChildren());
        updateLogs(snapshot);
    });

    if (!refreshInterval) {
        refreshInterval = setInterval(() => updateUI(lastStatusData), 5000);
    }
}

function stopDashboard() {
    if (statusRef) statusRef.off();
    if (logsRef) logsRef.off();
    statusRef = null;
    logsRef = null;
    if (clockInterval) clearInterval(clockInterval);
    if (refreshInterval) clearInterval(refreshInterval);
    clockInterval = null;
    refreshInterval = null;
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
    logsContainer.replaceChildren();
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
            tagClass = 'log-item__method-tag--rfid'; mTag = 'RFID: ' + log.method;
        } else if (log.event === 'UNAUTHORIZED') {
            tagClass = 'log-item__method-tag--danger'; mTag = 'Yetkisiz Giriş';
        }

        const photo = typeof log.photo_base64 === 'string' &&
            /^data:image\/jpeg;base64,[A-Za-z0-9+/=]+$/.test(log.photo_base64) &&
            log.photo_base64.length <= 150000
            ? log.photo_base64
            : 'data:image/svg+xml,%3Csvg xmlns="http://www.w3.org/2000/svg" width="72" height="72"%3E%3Crect width="72" height="72" fill="%23202738"/%3E%3C/svg%3E';

        const item = document.createElement('div');
        item.className = 'log-item';

        const image = document.createElement('img');
        image.className = 'log-item__photo';
        image.src = photo;
        image.alt = 'Erişim olayı fotoğrafı';

        const info = document.createElement('div');
        info.className = 'log-item__info';
        const time = document.createElement('span');
        time.className = 'log-item__time';
        time.textContent = d.toLocaleTimeString('tr-TR', {hour:'2-digit', minute:'2-digit'});
        const method = document.createElement('span');
        method.className = `log-item__method-tag ${tagClass}`;
        method.textContent = String(mTag).slice(0, 64);

        info.append(time, method);
        item.append(image, info);
        logsContainer.appendChild(item);
    });
    logCount.textContent = items.length + " kayıt";
}

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
