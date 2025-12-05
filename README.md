# 🐟 Automatic Fish Feeder Using ESP8266 🐟

Sistem ini berfungsi untuk memberi pakan ikan secara otomatis menggunakan **ESP8266** dan **servo**.
Data aktivitas dikirim ke server melalui **POST HTTPS**.

Perangkat dapat:
- Memberi pakan berdasarkan jadwal yang disediakan di API
- Mengirim log ke server
- Jadwal dikelola melalui web IoT

## 🏹 Tujuan

- Otomasi proses pemberian pakan ikan
- Menyediakan monitoring aktivitas
- Mengurangi human error dalam pemberian pakan

## 💻 Hardware

- ESP8266 NodeMCU
- NodeMCU Base v1.0
- Servo SG90
- Power Supply 9V 2A (disarankan 5V 2A)
- Kabel Jumper

<details>
<summary><h2>🏠 Arsitektur Sistem</h2></summary>

<img width="621" height="470" alt="Struktur-IoT-Ikan drawio (1)" src="https://github.com/user-attachments/assets/fe86c772-2e54-4383-a8b7-b6cd88b81a36" />
</details>

<details>
<summary><h2>🔃 Alur Kerja Sistem</h2></summary>

<img width="877" height="1569" alt="Struktur-IoT-Ikan drawio (2)" src="https://github.com/user-attachments/assets/f6866672-4a81-47a1-8456-ee0070c58871" />
</details>

<details>
<summary><h2>🗣 Pemanggilan Data dengan API Endpoint</h2></summary>

<img width="1508" height="2404" alt="fetchSchedules" src="https://github.com/user-attachments/assets/d3c9354c-32f8-447e-8c7c-05e6b295ccf5" />

ESP8266 mengambil/fetch data dari API dengan menggunakan method GET dan endpoint '/iotikan'. Mulai dari sini, Laravel sebagai back-end dari website akan menanganinya. Mulai dari route.

<img width="1190" height="324" alt="getroute" src="https://github.com/user-attachments/assets/5f9cae6a-1d24-47b5-8367-ec542e0f65ea" />

Ketika ada yang GET endpoint '/iotikan' pada website, method sendSchedule dari IotpakanController akan menanganinya.

<img width="1034" height="644" alt="sendSchedule" src="https://github.com/user-attachments/assets/f6040339-e9c3-47ff-8ad2-1b01058d0a5c" />

Setiap variabel time dan interval pada record dari tabel iotikans akan dipanggil. Setelah itu, data-datanya akan di-return dalam bentuk json.
</details>

<details>
<summary><h2>📩 Pengiriman Log dan Proses Penyimpanan</h2></summary>

<img width="1428" height="1028" alt="sendLog" src="https://github.com/user-attachments/assets/7c42bc4a-017b-4713-90b6-4fd024961b63" />

ESP mengirimkan data dalam bentuk json, dengan variabel log_message, yang nantinya akan diolah oleh back-end website supaya disimpan di database.

<img width="888" height="516" alt="receiveLogs" src="https://github.com/user-attachments/assets/c268692f-9b1b-44cd-832d-f8c8cfad4eac" />

Website menyimpan data yang dikirimkan ke dalam database dengan variabel log_message. Log-log tersebut nantinya akan ditampilkan di halaman website untuk dimonitor.

![image (16)](https://github.com/user-attachments/assets/8e3bcdae-b955-4d62-a0f0-24321ffbfd9d)
</details>
