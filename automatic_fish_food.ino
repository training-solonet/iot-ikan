#include <ESP8266WiFi.h>
#include <WiFiClientSecure.h>
#include <ArduinoJson.h>
#include <NTPClient.h>
#include <WiFiUdp.h>
#include <Servo.h>

// ================= Variabel untuk menyambungkan ke Wi-Fi ==================
const char* ssid = "CJI_PKL";
const char* pass = "solonetjaya";

// ================= Variabel untuk API ==================
const char* host = "monitoring.connectis.my.id";
const int httpsPort = 443;
String endpoint = "/api/iotikan";
String logEndpoint = "/api/logs";

// ================= Pendefinisian Servo ================
Servo servo;
int servoPin = D4;

// =============== Pendefinisian NTP Time untuk mengetahui waktu ===============
WiFiUDP ntpUDP;
NTPClient timeClient(ntpUDP, "pool.ntp.org", 25200, 60000); // +7 GMT

// =============== Pendefinisian struktur json yang akan di-fetch di API ===================
struct Schedule {
  String time;
  int interval;
};

Schedule schedules[10];
int scheduleCount = 0;

// Pendefinisian variabel lastFetch dan interval setiap fetch data
unsigned long lastFetch = 0;
unsigned long fetchInterval = 5 * 60 * 1000; // 5 menit

// ================== Function untuk mengirim log ==================
void sendLog(String msg) {
  Serial.println(msg);  // Pesan yang akan dikirim akan ditampilkan di serial monitor

  WiFiClientSecure client; // Pendefinisian untuk HTTPS
  client.setInsecure();

  // Jika tidak bisa tersambung ke API, maka langsung return
  if (!client.connect(host, httpsPort)) {
    Serial.println("Log POST failed (no connection)");
    return;
  }

  // Mengirim json dengan method POST
  String body = "{\"log_message\":\"" + msg + "\"}";

  client.print(String("POST ") + logEndpoint + " HTTP/1.1\r\n" +
               "Host: " + host + "\r\n" +
               "Content-Type: application/json\r\n" +
               "Content-Length: " + body.length() + "\r\n" +
               "Connection: close\r\n\r\n" +
               body);
  delay(1000);
}

// ================= Function untuk meng-fetch data dari API ==============
void fetchSchedule() {
  WiFiClientSecure client;
  client.setInsecure();

  sendLog("Mengambil informasi jadwal..."); // Mengirimkan log bahwa device meng-fetch data

  // Jika gagal, maka akan di print di serial monitor
  if (!client.connect(host, httpsPort)) {
    Serial.println("Koneksi gagal!");
    return;
  }

  client.print(String("GET ") + endpoint + " HTTP/1.1\r\n" +
               "Host: " + host + "\r\n" +
               "User-Agent: ESP8266\r\n" +
               "Connection: close\r\n\r\n");

  String payload;
  while (client.available() == 0) delay(100);

  while (client.available()) {
    payload += client.readStringUntil('\n');
  }

  client.stop();
  delay(500);

  // Proses JSON
  int jsonIndex = payload.indexOf('[');
  if (jsonIndex < 0) return;

  String jsonString = payload.substring(jsonIndex);
  DynamicJsonDocument doc(1024);
  deserializeJson(doc, jsonString);

  // Simpan jadwal lama & bandingkan
  Schedule oldSchedules[10];
  int oldCount = scheduleCount;
  for (int i = 0; i < oldCount; i++) {
    oldSchedules[i] = schedules[i];
  }

  scheduleCount = doc.size();
  for (int i = 0; i < scheduleCount; i++) {
    schedules[i].time = doc[i]["time"].as<String>();
    schedules[i].interval = doc[i]["interval"].as<int>();
  }

  // Membandingkan jadwal lama dan hasil fetch
  bool changed = false;
  if (oldCount != scheduleCount) {
    changed = true;
  } else {
    for (int i = 0; i < scheduleCount; i++) {
      if (schedules[i].time != oldSchedules[i].time ||
          schedules[i].interval != oldSchedules[i].interval) {
        changed = true;
        break;
      }
    }
  }

  if (changed) {
    sendLog("Jadwal diupdate!");  // Mengirim log bahwa jadwal berhasil diupdate
  }
}

// =========== Fungsi untuk menjalankan servo ==============
void runServo(int duration) {
  sendLog("Servo dibuka");
  servo.write(180);
  delay(duration);

  servo.write(0);
  sendLog("Servo ditutup");
}

// ================== Setup untuk ESP =================
void setup() {
  Serial.begin(9600);

  servo.attach(servoPin);
  servo.write(0);

  WiFi.begin(ssid, pass); // Menghubungkan ke Wi-Fi
  Serial.println("Menghubungkan ke WiFi...");

  while (WiFi.status() != WL_CONNECTED) {
    Serial.print(".");
    delay(300);
  }

  sendLog("WiFi tersambung!"); // Mengirim log bahwa Wi-Fi tersambung

  timeClient.begin(); // Memulai untuk mengambil waktu
  timeClient.update(); // Mengupdate waktu

  fetchSchedule(); // Meng-fetch data dari API
  lastFetch = millis(); // Menghitung jumlah milisekon sejak ESP dihidupkan
}

// ================== LOOP =================
void loop() {

  timeClient.update(); // Mengupdate waktu saat ini
  String now = timeClient.getFormattedTime().substring(0, 5); // Memformat waktu

  // ------ Meng-fetch data dari API jika millis dikurangi dengan lastFetch lebih dari 5 detik ------
  if (millis() - lastFetch > fetchInterval) {
    fetchSchedule();
    lastFetch = millis(); // Mereset lastFetch supaya menunda penge-fetch-an data 5 detik setelahnya
  }

  // ------ Mengecek apakah waktu schedule sama dengan waktu sekarang ini ------
  for (int i = 0; i < scheduleCount; i++) {
    if (now == schedules[i].time) {
      sendLog("Waktu sesuai, membuka servo di pukul " + now);
      runServo(schedules[i].interval);
      delay(60000); // Delay 1 menit supaya tidak terulang lagi di menit yang sama, meskipun berbeda detik
    }
  }

  delay(1000); // Delay satu detik supaya tidak terlalu cepat
}
