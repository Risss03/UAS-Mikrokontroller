#include <WiFi.h>
#include <HTTPClient.h>
#include <DHTesp.h>
#include <LiquidCrystal_I2C.h>
#include <ESP32Servo.h>

// Konfigurasi WiFi
const char* ssid = "Wokwi-GUEST";
const char* password = "";

// Konfigurasi ThingSpeak
const char* THINGSPEAK_API_KEY = "LC09B0R00LTX150A";
const char* server = "https://api.thingspeak.com/update";

// Inisialisasi objek
DHTesp dht;
LiquidCrystal_I2C lcd(0x27, 16, 2);
Servo servo;

// Pin
#define DHT_PIN 15
#define SOIL_PIN 34
#define SERVO_PIN 18

// Fungsi untuk menentukan status tanah berdasarkan persentase
String getSoilStatus(int percent) {
  if (percent < 30) return "Kering";
  else if (percent < 60) return "Sedang";
  else return "Basah";
}

void setup() {
  Serial.begin(115200);

  // Setup sensor dan servo
  dht.setup(DHT_PIN, DHTesp::DHT22);
  servo.attach(SERVO_PIN);
  lcd.init();
  lcd.backlight();

  // Koneksi WiFi
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("\nWiFi connected");
}

void loop() {
  TempAndHumidity data = dht.getTempAndHumidity();
  int soil = analogRead(SOIL_PIN);
  int soilPercent = map(soil, 4095, 0, 0, 100); // FC-28: 0 (basah) – 4095 (kering)
  String statusTanah = getSoilStatus(soilPercent);

  // Tampilkan ke LCD
  lcd.setCursor(0, 0);
  lcd.print("T:");
  lcd.print(data.temperature, 1);
  lcd.print("C H:");
  lcd.print(data.humidity, 0);
  lcd.print("%");

  lcd.setCursor(0, 1);
  lcd.print("Tanah:");
  lcd.print(statusTanah);
  lcd.print("    "); // Untuk menghapus sisa karakter sebelumnya

  // Aktuasi Servo
  if (soilPercent < 30) {
    servo.write(90);  // Posisi aktif (nyiram)
    delay(1000);
  } else {
    servo.write(0);   // Posisi idle
  }

  // Kirim ke ThingSpeak
  if (WiFi.status() == WL_CONNECTED) {
    HTTPClient http;
    String url = server;
    url += "?api_key=";
    url += THINGSPEAK_API_KEY;
    url += "&field1=" + String(data.temperature, 1);
    url += "&field2=" + String(data.humidity, 1);
    url += "&field3=" + String(soilPercent);
    url += "&field4=" + statusTanah;

    Serial.println("Mengirim ke ThingSpeak:");
    Serial.println(url);

    http.begin(url);
    int httpResponseCode = http.GET();
    if (httpResponseCode > 0) {
      String response = http.getString();
      Serial.println("Response: " + response);
    } else {
      Serial.print("Gagal mengirim data. Kode error: ");
      Serial.println(httpResponseCode);
    }
    http.end();
  }

  // Debug ke Serial
  Serial.print("Suhu: ");
  Serial.print(data.temperature, 1);
  Serial.print("C, Kelembaban: ");
  Serial.print(data.humidity, 1);
  Serial.print("%, Kelembaban Tanah: ");
  Serial.print(soilPercent);
  Serial.print("% (");
  Serial.print(statusTanah);
  Serial.println(")");

  delay(15000); // Delay minimum ThingSpeak
}
