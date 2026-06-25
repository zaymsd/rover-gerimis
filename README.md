<div align="center">
  
# 🚙 Smart Rover G.E.R.I.M.I.S
**Navigational Rover**

[![ESP32](https://img.shields.io/badge/ESP32-000000?style=for-the-badge&logo=espressif&logoColor=white)](#)
[![C++](https://img.shields.io/badge/C++-00599C?style=for-the-badge&logo=c%2B%2B&logoColor=white)](#)
[![Node.js](https://img.shields.io/badge/Node.js-339933?style=for-the-badge&logo=nodedotjs&logoColor=white)](#)
[![MQTT](https://img.shields.io/badge/MQTT-3C5280?style=for-the-badge&logo=mqtt&logoColor=white)](#)
[![Tailwind CSS](https://img.shields.io/badge/Tailwind_CSS-38B2AC?style=for-the-badge&logo=tailwind-css&logoColor=white)](#)

*Membawa Kecerdasan Buatan (AI) ke Ujung Ekosistem (Edge) melalui Mikrokontroler*

</div>

<br>

## 🌟 Tentang Proyek

**Smart Rover G.E.R.I.M.I.S** adalah proyek sistem robotik cerdas berbasis ESP32 yang dirancang untuk melakukan navigasi secara otonom sembari memantau kondisi lingkungan secara *real-time*. 

Keunikan proyek ini adalah implementasi algoritma **K-Nearest Neighbors (KNN)** secara *native* langsung di dalam mikrokontroler, Robot ini dapat memprediksi tingkat bahaya lingkungan berdasarkan korelasi antara Suhu dan Kelembapan Udara (Normal, Peringatan, atau Kritis). Seluruh data dari rover dikirimkan secara nirkabel melalui protokol **MQTT** ke Dashboard.

---

## ✨ Fitur Utama

- 🧠 **Native (KNN):** Prediksi kondisi lingkungan cerdas tertanam di ESP32.
- 📡 **Telemetry via MQTT:** Transmisi data ringan dan *real-time* ke sistem *cloud* (Shiftr.io).
- 🏎️ **Obstacle Avoidance:** Sensor ultrasonik untuk bermanuver menghindari rintangan secara otomatis.
- 🌡️ **Environmental Sensing:** Pembacaan Suhu, Kelembapan (DHT11), dan Sensor Hujan.
- 💻 **Real-time Web Dashboard:** UI/UX interaktif berbasis Node.js dan TailwindCSS dengan Socket.io.


---

## 📸 Workflow & Arsitektur

Berikut adalah gambaran arsitektur sistem dan alur kerja (workflow) dari Smart Rover G.E.R.I.M.I.S:

<div align="center">
  <img src="workflow/1.png" alt="Workflow 1" width="80%">
  <p><em>Gambar 1: Arsitektur dan Integrasi Sistem</em></p>
</div>

<br>

<div align="center">
  <img src="workflow/2.png" alt="Workflow 2" width="80%">
  <p><em>Gambar 2: Alur Komunikasi MQTT & Dashboard</em></p>
</div>

<br>

<div align="center">
  <img src="workflow/3.png" alt="Workflow 3" width="80%">
  <p><em>Gambar 3: Implementasi Hardware & Pengujian</em></p>
</div>

---

## 🛠️ Teknologi yang Digunakan

### Hardware ⚙️
- **ESP32 Dev Board** (Otak Utama)
- **L298N Motor Driver** & DC Motors (Penggerak)
- **DHT11 Sensor** (Suhu & Kelembapan)
- **Rain Sensor** (Deteksi Cuaca Hujan)
- **HC-SR04 Ultrasonic Sensor** (Navigasi)


### Software 💻
- **Firmware:** C++ (Arduino IDE), PubSubClient (MQTT)
- **Backend Dashboard:** Node.js, Express.js, Socket.io
- **Frontend Dashboard:** HTML, Vanilla JS, TailwindCSS V4
- **Broker:** Shiftr.io Cloud MQTT

---


<div align="center">
  
**Dibuat dengan ❤️ oleh Smart Rover G.E.R.I.M.I.S**

</div>
