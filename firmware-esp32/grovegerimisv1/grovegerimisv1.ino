#include <WiFi.h>
#include <PubSubClient.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <DHT.h>
#include <math.h>
#include <WebServer.h> 

// ==========================================
// 1. KONFIGURASI PIN G.E.R.I.M.I.S (SENSOR & AI)
// ==========================================
#define DHTPIN 4
#define DHTTYPE DHT11
#define RAIN_PIN 36
#define LED_PIN 2

#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
#define OLED_RESET -1
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);
DHT dht(DHTPIN, DHTTYPE);

// ==========================================
// 2. KONFIGURASI PIN ROBO CAR (NAVIGASI)
// ==========================================
// Sensor Jarak & Alarm
#define TRIG_PIN 18  // Aman untuk Output
#define ECHO_PIN 35  // Aman untuk Input-Only
#define BUZZER_PIN 19

// Motor Kanan (L298N) - Rentang Aman
#define ENA 13    
#define IN_1 14   
#define IN_2 16   

// Motor Kiri (L298N) - Rentang Aman
#define ENB 25    
#define IN_3 26   
#define IN_4 23   

int speedCar = 150; // Kecepatan default (0-255)
int speed_low = 60; // Pengurangan kecepatan untuk berbelok

// ==========================================
// 3. KONFIGURASI JARINGAN, MQTT & SERVER
// ==========================================
const char* ssid = "aselole";       
const char* password = "dariasampaiz"; 

// Sesuaikan dengan kredensial Shiftr.io Anda
const char* mqtt_server = "gerimisaja.cloud.shiftr.io";
const char* mqtt_user = "gerimisaja";
const char* mqtt_pass = "qyWmH5h5nWhS0NVE";

WiFiClient espClient;
PubSubClient client(espClient);
WebServer server(80); 

unsigned long waktuSebelumnya = 0;
unsigned long terakhirCobaMqtt = 0; // Timer khusus untuk Reconnect MQTT
const long interval = 3000;         // Siklus pembacaan sensor tiap 3 detik

// ==========================================
// 4. DATABASE AI (NATIVE KNN K=3)
// ==========================================
const int JUMLAH_DATA = 6;
float dataTraining[JUMLAH_DATA][2] = {
  {24.0, 50.0}, {26.5, 55.0}, // Normal (0)
  {29.0, 45.0}, {31.0, 40.0}, // Peringatan (1)
  {34.0, 35.0}, {36.0, 30.0}  // Kritis (2)
};
int labelTraining[JUMLAH_DATA] = {0, 0, 1, 1, 2, 2};

int prediksiKNN(float suhuSaatIni, float kelembapanSaatIni, int K) {
  struct Tetangga { float jarak; int label; };
  Tetangga daftar[JUMLAH_DATA];

  for (int i = 0; i < JUMLAH_DATA; i++) {
    float dSuhu = suhuSaatIni - dataTraining[i][0];
    float dLembap = kelembapanSaatIni - dataTraining[i][1];
    daftar[i].jarak = sqrt((dSuhu * dSuhu) + (dLembap * dLembap));
    daftar[i].label = labelTraining[i];
  }

  for (int i = 0; i < JUMLAH_DATA - 1; i++) {
    for (int j = 0; j < JUMLAH_DATA - i - 1; j++) {
      if (daftar[j].jarak > daftar[j + 1].jarak) {
        Tetangga temp = daftar[j]; 
        daftar[j] = daftar[j + 1]; 
        daftar[j + 1] = temp;
      }
    }
  }

  int suara[3] = {0, 0, 0};
  for (int i = 0; i < K; i++) suara[daftar[i].label]++;

  int terpilih = 0; int maxSuara = suara[0];
  if (suara[1] > maxSuara) { terpilih = 1; maxSuara = suara[1]; }
  if (suara[2] > maxSuara) { terpilih = 2; }
  
  return terpilih;
}

// ==========================================
// 5. FUNGSI PENGGERAK ROBOT (AKSI MOTOR)
// ==========================================
void goForword(){ 
  digitalWrite(IN_1, HIGH); digitalWrite(IN_2, LOW); analogWrite(ENA, speedCar);
  digitalWrite(IN_3, LOW); digitalWrite(IN_4, HIGH); analogWrite(ENB, speedCar);
}
void goBack(){ 
  digitalWrite(IN_1, LOW); digitalWrite(IN_2, HIGH); analogWrite(ENA, speedCar);
  digitalWrite(IN_3, HIGH); digitalWrite(IN_4, LOW); analogWrite(ENB, speedCar);
}
void goRight(){ 
  digitalWrite(IN_1, LOW); digitalWrite(IN_2, HIGH); analogWrite(ENA, speedCar);
  digitalWrite(IN_3, LOW); digitalWrite(IN_4, HIGH); analogWrite(ENB, speedCar);
}
void goLeft(){
  digitalWrite(IN_1, HIGH); digitalWrite(IN_2, LOW); analogWrite(ENA, speedCar);
  digitalWrite(IN_3, HIGH); digitalWrite(IN_4, LOW); analogWrite(ENB, speedCar);
}
void goForwordRight(){
  digitalWrite(IN_1, HIGH); digitalWrite(IN_2, LOW); analogWrite(ENA, speedCar-speed_low);
  digitalWrite(IN_3, LOW); digitalWrite(IN_4, HIGH); analogWrite(ENB, speedCar);
}
void goForwordLeft(){
  digitalWrite(IN_1, HIGH); digitalWrite(IN_2, LOW); analogWrite(ENA, speedCar);
  digitalWrite(IN_3, LOW); digitalWrite(IN_4, HIGH); analogWrite(ENB, speedCar-speed_low);
}
void goBackRight(){ 
  digitalWrite(IN_1, LOW); digitalWrite(IN_2, HIGH); analogWrite(ENA, speedCar-speed_low);
  digitalWrite(IN_3, HIGH); digitalWrite(IN_4, LOW); analogWrite(ENB, speedCar);
}
void goBackLeft(){ 
  digitalWrite(IN_1, LOW); digitalWrite(IN_2, HIGH); analogWrite(ENA, speedCar);
  digitalWrite(IN_3, HIGH); digitalWrite(IN_4, LOW); analogWrite(ENB, speedCar-speed_low);
}
void stopRobot(){  
  digitalWrite(IN_1, LOW); digitalWrite(IN_2, LOW); analogWrite(ENA, 0);
  digitalWrite(IN_3, LOW); digitalWrite(IN_4, LOW); analogWrite(ENB, 0);
}

// ==========================================
// 6. HANDLER KONTROL WEB (0 LATENCY)
// ==========================================
void HTTP_handleRoot(void) {
  if(server.hasArg("State")){
    String cmd = server.arg("State");
    Serial.println("Perintah: " + cmd);
    
    // Eksekusi instan pergerakan motor
    if (cmd == "F") goForword();
    else if (cmd == "B") goBack();
    else if (cmd == "L") goLeft();
    else if (cmd == "R") goRight();
    else if (cmd == "I") goForwordRight();
    else if (cmd == "G") goForwordLeft();
    else if (cmd == "J") goBackRight();
    else if (cmd == "H") goBackLeft();
    else if (cmd == "0") speedCar = 100;
    else if (cmd == "1") speedCar = 120;
    else if (cmd == "5") speedCar = 200;
    else if (cmd == "9") speedCar = 255;
    else if (cmd == "S") stopRobot();
  }
  
  // Kirim respon cepat ke aplikasi Android agar koneksi tidak menggantung
  server.send(200, "text/plain", "OK"); 
}

unsigned long waktuSensorJarak = 0; // Timer khusus agar tidak lag
int jarakTerakhir = 999; // Menyimpan data jarak secara global
// Fungsi membaca jarak ultrasonik (dalam cm)
int bacaJarak() {
  digitalWrite(TRIG_PIN, LOW);
  delayMicroseconds(2);
  digitalWrite(TRIG_PIN, HIGH); // Tembakkan suara
  delayMicroseconds(10);
  digitalWrite(TRIG_PIN, LOW);
  
  // Timeout 30000ms sangat penting agar ESP32 tidak "hang" jika tidak ada pantulan
  long durasi = pulseIn(ECHO_PIN, HIGH, 30000); 
  
  if (durasi == 0) return 999; // Jika tidak ada halangan, kembalikan nilai 999 cm
  
  int jarak = durasi * 0.034 / 2; // Rumus kecepatan suara
  return jarak;
}
// ==========================================
// 7. SETUP UTAMA
// ==========================================
void setup() {
  Serial.begin(115200);
  
  pinMode(RAIN_PIN, INPUT);
  pinMode(LED_PIN, OUTPUT); digitalWrite(LED_PIN, LOW);
  
  pinMode(TRIG_PIN, OUTPUT);
  pinMode(ECHO_PIN, INPUT);
  pinMode(BUZZER_PIN, OUTPUT);
  
  pinMode(ENA, OUTPUT); pinMode(IN_1, OUTPUT); pinMode(IN_2, OUTPUT);
  pinMode(ENB, OUTPUT); pinMode(IN_3, OUTPUT); pinMode(IN_4, OUTPUT);

  dht.begin();
  
  if(!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) {
    Serial.println(F("Gagal memuat OLED")); 
    for(;;);
  }
  
  Serial.print("\nMenghubungkan WiFi...");
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) { delay(500); Serial.print("."); }
  Serial.println("\nWiFi OK! IP Address: " + WiFi.localIP().toString());

  server.on("/", HTTP_handleRoot);
  server.onNotFound(HTTP_handleRoot);
  server.begin();

  client.setServer(mqtt_server, 1883);
}

// ==========================================
// 8. LOOP UTAMA (ANTI-BLOCKING SYSTEM)
// ==========================================
void loop() {
  // --- PRIORITAS 1: Kontrol Robot Android (Instan) ---
  server.handleClient(); 

  // ----------------------------------------------------
// ----------------------------------------------------
  // PRIORITAS 1.5: Sensor Anti-Tabrak (Mengecek tiap 100ms)
  // ----------------------------------------------------
  unsigned long waktuSekarangJarak = millis();
  if (waktuSekarangJarak - waktuSensorJarak >= 100) {
    waktuSensorJarak = waktuSekarangJarak;
    
    // [UPDATE] Simpan hasil bacaan ke variabel global
    jarakTerakhir = bacaJarak(); 
    
    if (jarakTerakhir <= 20) { 
      stopRobot(); 
      digitalWrite(BUZZER_PIN, HIGH); 
      // Hapus Serial.println jika tidak perlu agar Serial Monitor tidak banjir
    } else {
      digitalWrite(BUZZER_PIN, LOW); 
    }
  }

  // --- PRIORITAS 2: Jaga Koneksi MQTT (Non-Blocking) ---
  if (!client.connected()) {
    unsigned long waktuSekarang = millis();
    // Coba reconnect hanya setiap 5 detik agar robot tidak lag
    if (waktuSekarang - terakhirCobaMqtt >= 5000) { 
      terakhirCobaMqtt = waktuSekarang;
      Serial.println("Mencoba koneksi MQTT...");
      if (client.connect("ESP32_Rover", mqtt_user, mqtt_pass)) {
        Serial.println("Terhubung ke MQTT Broker!");
      }
    }
  } else {
    client.loop(); 
  }

  // --- PRIORITAS 3: Eksekusi AI & Sensor (Setiap 3 Detik) ---
  unsigned long waktuSekarang = millis();
  if (waktuSekarang - waktuSebelumnya >= interval) {
    waktuSebelumnya = waktuSekarang;

    float suhu = dht.readTemperature();
    float kelembapan = dht.readHumidity();
    int nilaiHujan = analogRead(RAIN_PIN);
    bool dhtError = isnan(suhu) || isnan(kelembapan);
    
    String statusHujan = "Kering";
    if (nilaiHujan < 2800) {
      statusHujan = "Hujan Lebat"; digitalWrite(LED_PIN, HIGH);
    } else if (nilaiHujan < 3400) {
      statusHujan = "Gerimis"; digitalWrite(LED_PIN, HIGH);
    } else {
      digitalWrite(LED_PIN, LOW);
    }

    String statusSuhu = "Menunggu";

    if (!dhtError) {
      int hasilKlasifikasi = prediksiKNN(suhu, kelembapan, 3);
      
      if (hasilKlasifikasi == 0) statusSuhu = "Normal";
      else if (hasilKlasifikasi == 1) statusSuhu = "Peringatan";
      else if (hasilKlasifikasi == 2) { 
        statusSuhu = "Kritis"; 
        tone(BUZZER_PIN, 1000, 500); // Alarm jika suhu ekstrem
      }

      // Transmisi Data HANYA jika MQTT terhubung
      if (client.connected()) {
        client.publish("gerimis/suhu", String(suhu).c_str());
        client.publish("gerimis/kelembapan", String(kelembapan).c_str());
        client.publish("gerimis/status", statusHujan.c_str());
        client.publish("gerimis/kelas_suhu", statusSuhu.c_str());
        client.publish("gerimis/jarak", String(jarakTerakhir).c_str());
      }
    }

    // Perbarui Layar OLED
    display.clearDisplay(); 
    display.setTextSize(1); 
    display.setTextColor(SSD1306_WHITE);
    
    display.setCursor(0, 0); display.println(F("--- ROVER GERIMIS ---"));
    display.setCursor(0, 15); display.print(F("Suhu : "));
    if (dhtError) display.print(F("ERR")); else { display.print(suhu, 1); display.print(F(" C")); }
    
    display.setCursor(0, 25); display.print(F("Rh   : "));
    if (dhtError) display.print(F("ERR")); else { display.print(kelembapan, 1); display.print(F(" %")); }
    
    display.setCursor(0, 40); display.print(F("Hujan: ")); display.print(statusHujan);
    
    display.setCursor(0, 52); display.print(F("AI   : "));
    if (statusSuhu == "Kritis" || statusSuhu == "Peringatan") {
      display.setTextColor(SSD1306_BLACK, SSD1306_WHITE); 
      display.print(statusSuhu);
      display.setTextColor(SSD1306_WHITE, SSD1306_BLACK);
    } else {
      display.print(statusSuhu);
    }

    display.display();
  }
}