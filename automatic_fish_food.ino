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
unsigned long fetchInterval = 2 * 60 * 60 * 1000; // 2 jam

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

  sendLog("Mengambil informasi jadwal...");

  if (!client.connect(host, httpsPort)) {
    sendLog("Koneksi gagal ke server!");
    return;
  }

  // Kirim GET request
  client.print(String("GET ") + endpoint + " HTTP/1.1\r\n" +
               "Host: " + host + "\r\n" +
               "User-Agent: ESP8266\r\n" +
               "Connection: close\r\n\r\n");

  // Tunggu respons
  unsigned long timeout = millis();
  while (client.available() == 0) {
    if (millis() - timeout > 5000) { 
      sendLog("Timeout: Tidak ada respons dari server");
      client.stop();
      return;
    }
  }

  // ====== Ambil Status Line ======
  String statusLine = client.readStringUntil('\n');

  // ====== Ambil Header ======
  String line = "";
  String headers = "";
  while (client.connected()) {
    line = client.readStringUntil('\n');
    if (line == "\r" || line.length() == 0) break;
    headers += line + "\n";
  }

  // ====== Ambil Raw Body ======
  delay(100);
  String rawBody = "";
  while (client.available()) {
    rawBody += client.readString();
  }
  client.stop();

  // Jika body kosong
  if (rawBody.length() == 0) {
    sendLog("Body kosong dari server.");
    return;
  }

  // ====== Bersihkan menjadi JSON murni ======
  String json = "";
  int lastBracket = rawBody.lastIndexOf(']');
  if (lastBracket != -1) {
    for (int i = lastBracket; i >= 0; i--) {
      if (rawBody[i] == '[') {
        String candidate = rawBody.substring(i, lastBracket + 1);
        if (candidate.indexOf("\"time\"") != -1) {
          json = candidate;
          break;
        }
      }
    }
  }

  if (json.length() == 0) {
    sendLog("Gagal menemukan JSON dalam respons.");
    return;
  }

  // ====== PARSE JSON ======
  DynamicJsonDocument doc(4096);
  DeserializationError error = deserializeJson(doc, json);

  if (error) {
    sendLog("JSON Parsing Error: " + String(error.f_str()));
    sendLog("JSON gagal: " + json);
    return;
  }

  // Simpan dan bandingkan jadwal
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

  bool changed = false;
  if (oldCount != scheduleCount) changed = true;
  else {
    for (int i = 0; i < scheduleCount; i++) {
      if (schedules[i].time != oldSchedules[i].time ||
          schedules[i].interval != oldSchedules[i].interval) {
        changed = true;
        break;
      }
    }
  }

  if (changed) {
    sendLog("Jadwal diperbarui:");
    for (int i = 0; i < scheduleCount; i++) {
      sendLog("Time: " + schedules[i].time +
              " | Interval: " + String(schedules[i].interval));
      delay(1000);
    }
  }

  // ============================================================
  //               LOG DEBUG AKHIR (DIPINDAH KE BAWAH)
  // ============================================================
  sendLog("=== RESPONS SERVER ===");
  // sendLog("STATUS: " + statusLine);
  // sendLog("HEADERS:\n" + headers);
  // sendLog("RAW BODY:\n" + rawBody);
  // sendLog("JSON MURNI:\n" + json);
  sendLog("=== END DEBUG ===");
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
