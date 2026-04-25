import { useState, useEffect } from 'react';
import { ref, onValue, set } from 'firebase/database';
import { db } from './firebase';
import { Lock, Unlock, Activity, ShieldCheck, History, Cpu, Zap, Radio } from 'lucide-react';

function App() {
  const [isLocked, setIsLocked] = useState(true);
  const [isOnline, setIsOnline] = useState(false);
  const [espIp, setEspIp] = useState("");
  const [logs, setLogs] = useState([]);

  useEffect(() => {
    // Global data variable for interval check
    let currentData = null;

    const statusRef = ref(db, '/safe_001/status');
    const unsubStatus = onValue(statusRef, (snapshot) => {
      const data = snapshot.val();
      currentData = data;
      if (data) {
        const now = Math.floor(Date.now() / 1000);
        const lastSeen = data.last_seen || 0;
        const isActuallyOnline = data.is_online && (now - lastSeen < 60);
        
        setIsOnline(isActuallyOnline);
        setIsLocked(isActuallyOnline ? data.is_locked : true);
        if (data.ip) setEspIp(data.ip);
      }
    });

    const interval = setInterval(() => {
      if (currentData) {
        const now = Math.floor(Date.now() / 1000);
        const lastSeen = currentData.last_seen || 0;
        const isActuallyOnline = currentData.is_online && (now - lastSeen < 60);
        
        setIsOnline(isActuallyOnline);
        if (!isActuallyOnline) setIsLocked(true);
      }
    }, 5000);

    const logsRef = ref(db, '/safe_001/logs');
    const unsubLogs = onValue(logsRef, (snapshot) => {
      const data = snapshot.val();
      if (data) {
        const logsArray = Object.keys(data).map(key => ({
          id: key,
          ...data[key]
        })).reverse();
        setLogs(logsArray.slice(0, 10)); // Son 10 log
      }
    });

    return () => {
      unsubStatus();
      unsubLogs();
      clearInterval(interval);
    };
  }, []);

  const handleUnlock = () => {
    set(ref(db, '/safe_001/control/alarm'), 'REMOTE_UNLOCK');
  };

  return (
    <div className="dashboard-container" style={{ padding: '40px 20px', maxWidth: '1200px', margin: '0 auto' }}>
      
      {/* HEADER SECTION */}
      <header style={{ marginBottom: '40px', display: 'flex', justifyContent: 'space-between', alignItems: 'center' }}>
        <div>
          <h1 style={{ fontSize: '32px', fontWeight: '700', letterSpacing: '-0.5px', display: 'flex', alignItems: 'center', gap: '12px' }}>
            <ShieldCheck size={38} className="glow-blue" style={{ color: 'var(--accent-blue)' }} />
            SMART<span style={{ color: 'var(--accent-blue)' }}>SAFE</span> AI
          </h1>
          <p style={{ color: 'var(--text-secondary)', marginTop: '4px' }}>Advanced Biometric Security Dashboard</p>
        </div>
        
        <div className="glass-card" style={{ padding: '10px 20px', display: 'flex', alignItems: 'center', gap: '12px' }}>
          <div style={{ position: 'relative' }}>
            <Radio size={20} color={isOnline ? 'var(--accent-green)' : 'var(--text-secondary)'} />
            {isOnline && <div className="pulse-active" style={{ position: 'absolute', top: 0, left: 0, width: '100%', height: '100%', borderRadius: '50%', background: 'var(--accent-green)', opacity: 0.4 }}></div>}
          </div>
          <span style={{ fontWeight: '600', color: isOnline ? 'var(--accent-green)' : 'var(--text-secondary)' }}>
            {isOnline ? 'SYSTEM LIVE' : 'OFFLINE'}
          </span>
        </div>
      </header>

      {/* LIVE CAMERA FEED */}
      {isOnline && espIp && (
        <section style={{ marginBottom: '40px', animation: 'fadeIn 0.8s ease-out' }}>
          <div className="glass-card" style={{ padding: '24px', overflow: 'hidden' }}>
            <div style={{ display: 'flex', justifyContent: 'space-between', alignItems: 'center', marginBottom: '16px' }}>
              <h3 style={{ fontSize: '18px', fontWeight: '600', display: 'flex', alignItems: 'center', gap: '10px' }}>
                <Activity size={20} color="var(--accent-green)" /> LIVE SECURE FEED
              </h3>
              <div style={{ display: 'flex', alignItems: 'center', gap: '8px', background: 'rgba(239, 68, 68, 0.1)', padding: '4px 12px', borderRadius: '20px', border: '1px solid rgba(239, 68, 68, 0.2)' }}>
                <div className="pulse-active" style={{ width: '8px', height: '8px', borderRadius: '50%', background: 'var(--accent-red)' }}></div>
                <span style={{ fontSize: '12px', fontWeight: '700', color: 'var(--accent-red)' }}>REC</span>
              </div>
            </div>
            <div style={{ position: 'relative', borderRadius: '16px', overflow: 'hidden', background: '#000', aspectRatio: '16/9' }}>
              <img 
                src={`http://${espIp}:81/stream`} 
                alt="ESP32-CAM Stream" 
                style={{ width: '100%', height: '100%', objectFit: 'cover' }}
                onError={(e) => {
                  e.target.style.display = 'none';
                  e.target.nextSibling.style.display = 'flex';
                }}
              />
              <div style={{ display: 'none', position: 'absolute', top: 0, left: 0, width: '100%', height: '100%', alignItems: 'center', justifyContent: 'center', flexDirection: 'column', gap: '15px', color: 'var(--text-secondary)' }}>
                <Radio size={48} opacity={0.3} />
                <p>Waiting for Camera Stream Connection...</p>
                <p style={{ fontSize: '12px' }}>{`http://${espIp}:81/stream`}</p>
              </div>
            </div>
          </div>
        </section>
      )}

      <div style={{ display: 'grid', gridTemplateColumns: 'repeat(auto-fit, minmax(350px, 1fr))', gap: '24px', marginBottom: '40px' }}>
        
        {/* MAIN LOCK STATUS */}
        <div className={`glass-card ${isLocked ? 'glow-red' : 'glow-green'}`} style={{ padding: '40px', textAlign: 'center', display: 'flex', flexDirection: 'column', alignItems: 'center', justifyContent: 'center' }}>
          <div style={{ marginBottom: '24px', padding: '30px', borderRadius: '50%', background: isLocked ? 'rgba(239, 68, 68, 0.1)' : 'rgba(16, 185, 129, 0.1)', border: `1px solid ${isLocked ? 'rgba(239, 68, 68, 0.2)' : 'rgba(16, 185, 129, 0.2)'}` }}>
            {isLocked ? <Lock size={80} color="var(--accent-red)" /> : <Unlock size={80} color="var(--accent-green)" />}
          </div>
          
          <h2 style={{ fontSize: '14px', textTransform: 'uppercase', letterSpacing: '2px', color: 'var(--text-secondary)', marginBottom: '8px' }}>Security Status</h2>
          <div style={{ fontSize: '48px', fontWeight: '800', color: isLocked ? 'var(--accent-red)' : 'var(--accent-green)', marginBottom: '32px' }}>
            {isLocked ? 'LOCKED' : 'UNLOCKED'}
          </div>

          <button 
            onClick={handleUnlock}
            disabled={!isLocked || !isOnline}
            style={{ 
              width: '100%', padding: '18px', borderRadius: '16px', border: 'none',
              background: isLocked && isOnline ? 'linear-gradient(135deg, #3b82f6 0%, #2563eb 100%)' : 'rgba(255,255,255,0.05)',
              color: isLocked && isOnline ? 'white' : 'var(--text-secondary)',
              fontSize: '18px', fontWeight: '700', cursor: isLocked && isOnline ? 'pointer' : 'not-allowed',
              boxShadow: isLocked && isOnline ? '0 10px 20px rgba(37, 99, 235, 0.3)' : 'none'
            }}
          >
            {isLocked ? (isOnline ? 'AUTHORIZE REMOTE ACCESS' : 'SYSTEM OFFLINE') : 'ACCESS GRANTED'}
          </button>
        </div>

        {/* SYSTEM STATS */}
        <div className="glass-card" style={{ padding: '32px', display: 'flex', flexDirection: 'column', gap: '20px' }}>
          <h3 style={{ fontSize: '20px', fontWeight: '600', marginBottom: '10px', display: 'flex', alignItems: 'center', gap: '10px' }}>
            <Cpu size={24} color="var(--accent-blue)" /> System Diagnostics
          </h3>
          
          <div style={{ display: 'grid', gridTemplateColumns: '1fr 1fr', gap: '16px' }}>
            <div style={{ padding: '20px', borderRadius: '16px', background: 'rgba(255,255,255,0.03)', border: '1px solid var(--glass-border)' }}>
              <div style={{ color: 'var(--text-secondary)', fontSize: '12px', marginBottom: '4px' }}>AI Model</div>
              <div style={{ fontWeight: '600' }}>ArcFace Premium</div>
            </div>
            <div style={{ padding: '20px', borderRadius: '16px', background: 'rgba(255,255,255,0.03)', border: '1px solid var(--glass-border)' }}>
              <div style={{ color: 'var(--text-secondary)', fontSize: '12px', marginBottom: '4px' }}>Response Time</div>
              <div style={{ fontWeight: '600' }}>~450ms</div>
            </div>
            <div style={{ padding: '20px', borderRadius: '16px', background: 'rgba(255,255,255,0.03)', border: '1px solid var(--glass-border)' }}>
              <div style={{ color: 'var(--text-secondary)', fontSize: '12px', marginBottom: '4px' }}>Camera</div>
              <div style={{ fontWeight: '600' }}>HD 30 FPS</div>
            </div>
            <div style={{ padding: '20px', borderRadius: '16px', background: 'rgba(255,255,255,0.03)', border: '1px solid var(--glass-border)' }}>
              <div style={{ color: 'var(--text-secondary)', fontSize: '12px', marginBottom: '4px' }}>Encryption</div>
              <div style={{ fontWeight: '600' }}>AES-256</div>
            </div>
          </div>

          <div style={{ marginTop: 'auto', padding: '20px', borderRadius: '16px', background: 'rgba(59, 130, 246, 0.1)', border: '1px solid rgba(59, 130, 246, 0.2)', display: 'flex', alignItems: 'center', gap: '15px' }}>
            <Zap size={32} color="var(--accent-blue)" />
            <div>
              <div style={{ fontWeight: '600', color: 'var(--text-primary)' }}>Performance Mode</div>
              <div style={{ fontSize: '12px', color: 'var(--text-secondary)' }}>System optimized for low latency</div>
            </div>
          </div>
        </div>

      </div>

      {/* ACTIVITY LOGS */}
      <section>
        <div style={{ display: 'flex', justifyContent: 'space-between', alignItems: 'center', marginBottom: '20px' }}>
          <h2 style={{ fontSize: '24px', fontWeight: '700', display: 'flex', alignItems: 'center', gap: '12px' }}>
            <History size={28} color="var(--accent-blue)" /> Access History
          </h2>
          <span style={{ fontSize: '13px', color: 'var(--text-secondary)', background: 'rgba(255,255,255,0.05)', padding: '4px 12px', borderRadius: '20px' }}>
            Last 10 entries
          </span>
        </div>

        <div style={{ display: 'flex', flexDirection: 'column', gap: '16px' }}>
          {logs.length === 0 ? (
            <div className="glass-card" style={{ padding: '40px', textAlign: 'center', color: 'var(--text-secondary)' }}>
              Waiting for system activity logs...
            </div>
          ) : (
            logs.map((log, index) => (
              <div key={log.id} className="glass-card" style={{ padding: '20px', display: 'flex', alignItems: 'center', gap: '24px', animation: `fadeIn 0.5s ease-out ${index * 0.1}s both` }}>
                <div style={{ position: 'relative' }}>
                  {log.photo_base64 ? (
                    <img src={log.photo_base64} alt="Identity" style={{ width: '140px', height: '100px', objectFit: 'cover', borderRadius: '12px', border: '1px solid var(--glass-border)' }} />
                  ) : (
                    <div style={{ width: '140px', height: '100px', background: 'rgba(255,255,255,0.05)', borderRadius: '12px', display: 'flex', alignItems: 'center', justifyContent: 'center', color: 'var(--text-secondary)' }}>
                      No Visual
                    </div>
                  )}
                  <div style={{ position: 'absolute', bottom: '-8px', right: '-8px', background: log.event === 'UNAUTHORIZED' ? 'var(--accent-red)' : 'var(--accent-blue)', borderRadius: '50%', padding: '6px' }}>
                    {log.event === 'UNAUTHORIZED' ? <ShieldCheck size={16} color="white" /> : <ShieldCheck size={16} color="white" />}
                  </div>
                </div>

                <div style={{ flex: 1 }}>
                  <div style={{ display: 'flex', justifyContent: 'space-between', marginBottom: '8px' }}>
                    <h4 style={{ fontSize: '18px', fontWeight: '600', color: log.event === 'UNAUTHORIZED' ? 'var(--accent-red)' : 'var(--text-primary)' }}>
                      {log.event === 'AUTHORIZED' ? 'System Access Granted' :
                       log.event === 'UNAUTHORIZED' ? '⚠️ UNAUTHORIZED ACCESS ATTEMPT' : 'Manual Operation'}
                    </h4>
                    <span style={{ fontSize: '12px', color: 'var(--text-secondary)' }}>
                      {new Date(log.timestamp * 1000).toLocaleString('tr-TR', { hour: '2-digit', minute: '2-digit', second: '2-digit', day: '2-digit', month: '2-digit' })}
                    </span>
                  </div>
                  
                  <div style={{ display: 'flex', gap: '20px' }}>
                    <div style={{ display: 'flex', alignItems: 'center', gap: '6px', fontSize: '14px', color: 'var(--text-secondary)' }}>
                      <Zap size={14} /> Method: <span style={{ color: 'var(--text-primary)', fontWeight: '500' }}>{log.method}</span>
                    </div>
                    <div style={{ display: 'flex', alignItems: 'center', gap: '6px', fontSize: '14px', color: 'var(--text-secondary)' }}>
                      <Activity size={14} /> ID: <span style={{ color: 'var(--text-primary)', fontWeight: '500' }}>{log.id.substring(0, 8)}...</span>
                    </div>
                  </div>
                </div>
              </div>
            ))
          )}
        </div>
      </section>

      <style>{`
        @keyframes fadeIn {
          from { opacity: 0; transform: translateY(10px); }
          to { opacity: 1; transform: translateY(0); }
        }
      `}</style>
    </div>
  );
}

export default App;