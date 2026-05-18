import 'dart:convert';
import 'package:flutter/material.dart';
import 'package:firebase_core/firebase_core.dart';
import 'package:firebase_database/firebase_database.dart';

void main() async {
  WidgetsFlutterBinding.ensureInitialized();
  await Firebase.initializeApp(); // Firebase'i ateşle!
  runApp(const AkilliKasaApp());
}

class AkilliKasaApp extends StatelessWidget {
  const AkilliKasaApp({super.key});

  @override
  Widget build(BuildContext context) {
    return MaterialApp(
      debugShowCheckedModeBanner: false,
      theme: ThemeData(primarySwatch: Colors.blue, useMaterial3: true),
      home: const DashboardPage(),
    );
  }
}

class DashboardPage extends StatefulWidget {
  const DashboardPage({super.key});

  @override
  State<DashboardPage> createState() => _DashboardPageState();
}

class _DashboardPageState extends State<DashboardPage> {
  final DatabaseReference _dbRef = FirebaseDatabase.instance.ref();
  bool isLocked = true;
  bool isOnline = false;
  List<Map<dynamic, dynamic>> logs = [];

  @override
  void initState() {
    super.initState();
    _listenData();
  }

  void _listenData() {
    // 1. Durum ve Online takibi
    _dbRef.child('safe_001/status').onValue.listen((event) {
      final data = event.snapshot.value as Map?;
      if (data != null) {
        setState(() {
          isLocked = data['is_locked'] ?? true;
          isOnline = data['is_online'] ?? false;
        });
      }
    });

    // 2. Logların takibi
    _dbRef.child('safe_001/logs').onValue.listen((event) {
      final data = event.snapshot.value as Map?;
      if (data != null) {
        var tempList = <Map<dynamic, dynamic>>[];
        data.forEach((key, value) {
          tempList.add(value);
        });
        // En yeni log en üstte olsun
        tempList.sort((a, b) => b['timestamp'].compareTo(a['timestamp']));
        setState(() {
          logs = tempList;
        });
      }
    });
  }

  void _unlockSafe() {
    _dbRef.child('safe_001/control/remote_unlock').set(true);
  }

  @override
  Widget build(BuildContext context) {
    return Scaffold(
      appBar: AppBar(
        title: const Text("Kasa Komuta Merkezi"),
        backgroundColor: Colors.blueGrey[900],
        foregroundColor: Colors.white,
      ),
      body: Column(
        children: [
          // DURUM KARTI
          Container(
            margin: const EdgeInsets.all(16),
            padding: const EdgeInsets.all(24),
            decoration: BoxDecoration(
              color: isLocked ? Colors.red[50] : Colors.green[50],
              borderRadius: BorderRadius.circular(20),
              border: Border.all(color: isLocked ? Colors.red : Colors.green),
            ),
            child: Column(
              children: [
                Icon(isLocked ? Icons.lock : Icons.lock_open, size: 80, color: isLocked ? Colors.red : Colors.green),
                const SizedBox(height: 10),
                Text(isLocked ? "KİLİTLİ" : "AÇIK", style: const TextStyle(fontSize: 32, fontWeight: FontWeight.bold)),
                const SizedBox(height: 20),
                ElevatedButton.icon(
                  onPressed: isLocked ? _unlockSafe : null,
                  icon: const Icon(Icons.touch_app),
                  label: const Text("UZAKTAN KİLİDİ AÇ"),
                  style: ElevatedButton.styleFrom(backgroundColor: Colors.blue, foregroundColor: Colors.white),
                )
              ],
            ),
          ),
          
          // BAĞLANTI DURUMU
          Padding(
            padding: const EdgeInsets.symmetric(horizontal: 16),
            child: Row(
              children: [
                Icon(Icons.circle, size: 12, color: isOnline ? Colors.green : Colors.grey),
                const SizedBox(width: 8),
                Text(isOnline ? "Cihaz Online" : "Cihaz Offline", style: TextStyle(color: Colors.grey[700])),
              ],
            ),
          ),

          const Divider(height: 32),

          // LOG LİSTESİ
          const Text("Erişim Kayıtları", style: TextStyle(fontSize: 18, fontWeight: FontWeight.bold)),
          Expanded(
            child: ListView.builder(
              itemCount: logs.length,
              itemBuilder: (context, index) {
                final log = logs[index];
                final base64String = log['photo_base64']?.toString().split(',').last;
                
                return Card(
                  margin: const EdgeInsets.symmetric(horizontal: 16, vertical: 8),
                  child: ListTile(
                    leading: base64String != null 
                      ? Image.memory(base64Decode(base64String), width: 60, height: 60, fit: BoxFit.cover)
                      : const Icon(Icons.image_not_supported),
                    title: Text(log['event'] == 'UNLOCKED' ? "🔓 Kilit Açıldı" : "⚠️ Olay"),
                    subtitle: Text("Yöntem: ${log['method']}\n${DateTime.fromMillisecondsSinceEpoch((log['timestamp'] as int) * 1000)}"),
                  ),
                );
              },
            ),
          ),
        ],
      ),
    );
  }
}