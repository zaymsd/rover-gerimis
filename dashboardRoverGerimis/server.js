const express = require('express');
const http = require('http');
const { Server } = require("socket.io");
const mqtt = require('mqtt');
const mysql = require('mysql2');

const app = express();
const server = http.createServer(app);
const io = new Server(server);

app.use(express.static('public'));

// ==========================================
// 1. KONFIGURASI DATABASE MYSQL
// ==========================================
const db = mysql.createConnection({
  host: 'localhost',      
  user: 'root',           
  password: '', // Sesuaikan password Anda
  database: 'db_gerimis'  
});

db.connect((err) => {
  if (err) {
    console.error('❌ Gagal terhubung ke Database MySQL:', err.message);
    return;
  }
  console.log('✅ Terhubung ke Database MySQL');
});

// ==========================================
// 2. KONFIGURASI MQTT BROKER (SHIFTR.IO)
// ==========================================
const mqttUrl = 'mqtt://gerimisaja:qyWmH5h5nWhS0NVE@gerimisaja.cloud.shiftr.io';
const mqttClient = mqtt.connect(mqttUrl);

// Buffer Objek untuk mencegah Race Condition Asynchronous
let sensorBuffer = {};

mqttClient.on('connect', () => {
  console.log('✅ Terhubung ke Broker MQTT Shiftr.io');
  mqttClient.subscribe('gerimis/#'); 
});

mqttClient.on('message', (topic, message) => {
  const payload = message.toString();
  
  // 1. Pancarkan langsung ke UI Dashboard via WebSockets
  io.emit('sensorData', { 
    topik_mqtt: topic, 
    nilai_sensor: payload 
  });

  // 2. Masukkan data ke dalam buffer berdasarkan topiknya
  if (topic === 'gerimis/suhu') sensorBuffer.suhu = parseFloat(payload);
  if (topic === 'gerimis/kelembapan') sensorBuffer.kelembapan = parseFloat(payload);
  if (topic === 'gerimis/status') sensorBuffer.status_hujan = payload;
  if (topic === 'gerimis/kelas_suhu') sensorBuffer.kelas_suhu = payload;
  if (topic === 'gerimis/jarak') sensorBuffer.jarak = parseInt(payload); // [BARU] Menangkap Jarak

  // 3. Evaluasi: Apakah buffer sudah berisi 5 data secara lengkap?
  if (
      sensorBuffer.suhu !== undefined && 
      sensorBuffer.kelembapan !== undefined && 
      sensorBuffer.status_hujan !== undefined && 
      sensorBuffer.kelas_suhu !== undefined &&
      sensorBuffer.jarak !== undefined
  ) {
    
    // Kunci data dengan Cloning
    const dataSiapSimpan = { ...sensorBuffer };
    
    // Kosongkan buffer untuk siklus dari ESP32 berikutnya
    sensorBuffer = {};

    // 4. Proses Insert ke MySQL (Kini dengan 5 Kolom Parameter)
    const queryInsert = 'INSERT INTO cuaca_logs (suhu, kelembapan, kelas_suhu, status_hujan, jarak, created_at) VALUES (?, ?, ?, ?, ?, NOW())';
    
    const nilaiKolom = [
        dataSiapSimpan.suhu, 
        dataSiapSimpan.kelembapan, 
        dataSiapSimpan.kelas_suhu, 
        dataSiapSimpan.status_hujan,
        dataSiapSimpan.jarak
    ];

    db.query(queryInsert, nilaiKolom, (err, result) => {
      if (err) {
        console.error("❌ Gagal menyimpan ke Database:", err.message);
      } else {
        console.log(`📥 [Log Tersimpan] -> Suhu: ${dataSiapSimpan.suhu}°C | Rh: ${dataSiapSimpan.kelembapan}% | Hujan: ${dataSiapSimpan.status_hujan} | AI: ${dataSiapSimpan.kelas_suhu} | Radar: ${dataSiapSimpan.jarak} cm`);
      }
    });
  }
});

// ==========================================
// 3. MENJALANKAN SERVER
// ==========================================
const PORT = 3000;
server.listen(PORT, () => {
  console.log(`🚀 Dashboard G.E.R.I.M.I.S berjalan aktif di http://localhost:${PORT}`);
});