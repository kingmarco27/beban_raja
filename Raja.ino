#include <WiFi.h>
#include <FirebaseESP32.h>
#include <PZEM004Tv30.h>
#include <Wire.h>
#include <LiquidCrystal_I2C.h>

// Konfigurasi WiFi
#define WIFI_SSID "NAMA_WIFI"
#define WIFI_PASSWORD "PASSWORD_WIFI"

// Konfigurasi Firebase
#define API_KEY "API_KEY"
#define DATABASE_URL " " 
#define USER_EMAIL "EMAIL_FIREBASE"
#define USER_PASSWORD "PASSWORD_FIREBASE"

// Inisialisasi Firebase
FirebaseData fbdo;
FirebaseAuth auth;
FirebaseConfig config;

//  Inisialisasi PZEM
// (ubah pin sesuai wiring kamu)
PZEM004Tv30 pzemR(&Serial2, 16, 17);  // PZEM fasa R
PZEM004Tv30 pzemS(&Serial2, 18, 19);  // PZEM fasa S
PZEM004Tv30 pzemT(&Serial2, 21, 22);  // PZEM fasa T

// Inisialisasi LCD
LiquidCrystal_I2C lcd(0x27, 20, 4); 

// Variabel Send data ke firebase / 10 detik
unsigned long lastPageSwitch = 0;
int currentPage = 1;

unsigned long lastSend = 0;
const unsigned long intervalSend = 600000; // 10 menit = 600.000 ms

// Variabel Data 
float vR, vS, vT;
float iR, iS, iT;
float pfR, pfS, pfT, pfAvg;
float vRS, vST, vRT;
float iUnbalance;

void setup() {
  Serial.begin(115200);

  // LCD
  lcd.init();
  lcd.backlight();
  lcd.setCursor(0,0); lcd.print("Monitoring Listrik");
  delay(2000);
  lcd.clear();

  // WiFi
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  Serial.print("Menghubungkan WiFi");
  while (WiFi.status() != WL_CONNECTED) {
    delay(500); Serial.print(".");
  }
  Serial.println("\nWiFi Terhubung!");

  // Firebase
  config.api_key = API_KEY;
  config.database_url = DATABASE_URL;
  auth.user.email = USER_EMAIL;
  auth.user.password = USER_PASSWORD;

  Firebase.begin(&config, &auth);
  Firebase.reconnectWiFi(true);
}

void loop() {
  // data dari PZEM 
  vR = pzemR.voltage();
  vS = pzemS.voltage();
  vT = pzemT.voltage();

  iR = pzemR.current();
  iS = pzemS.current();
  iT = pzemT.current();

  pfR = pzemR.pf();
  pfS = pzemS.pf();
  pfT = pzemT.pf();
  pfAvg = (pfR + pfS + pfT) / 3.0;

  // Hitung tegangan antar fasa (asumsi)
  vRS = abs(vR - vS);
  vST = abs(vS - vT);
  vRT = abs(vR - vT);

  // ketidakseimbangan arus
  float iAvg = (iR + iS + iT) / 3.0;
  float iMax = max(iR, max(iS, iT));
  iUnbalance = ((iMax - iAvg) / iAvg) * 100.0;

  // Tampilan LCD (update tiap 20 detik)
  if (millis() - lastPageSwitch > 20000) { // ganti page tiap 20 detik
    currentPage++;
    if (currentPage > 3) currentPage = 1;
    lastPageSwitch = millis();

    lcd.clear();
    if (currentPage == 1) {
      lcd.setCursor(0,0); lcd.print("VR:"); lcd.print(vR); lcd.print(" V");
      lcd.setCursor(0,1); lcd.print("VS:"); lcd.print(vS); lcd.print(" V");
      lcd.setCursor(0,2); lcd.print("VT:"); lcd.print(vT); lcd.print(" V");
      lcd.setCursor(0,3); 
      lcd.print("RS:"); lcd.print(vRS); 
      lcd.print(" ST:"); lcd.print(vST); 
      lcd.print(" RT:"); lcd.print(vRT);

    } else if (currentPage == 2) {
      lcd.setCursor(0,0); lcd.print("IR:"); lcd.print(iR); lcd.print(" A");
      lcd.setCursor(0,1); lcd.print("IS:"); lcd.print(iS); lcd.print(" A");
      lcd.setCursor(0,2); lcd.print("IT:"); lcd.print(iT); lcd.print(" A");

    } else if (currentPage == 3) {
      lcd.setCursor(0,0); lcd.print("PF Avg: "); lcd.print(pfAvg,2);
      lcd.setCursor(0,1); lcd.print("Unbal: "); lcd.print(iUnbalance,1); lcd.print(" %");
    }
  }

  // Kirim ke Firebase tiap 10 menit
  if (Firebase.ready() && (millis() - lastSend > intervalSend)) {
    lastSend = millis(); // reset timer

    Firebase.setFloat(fbdo, "/Monitoring/Voltage/VR", vR);
    Firebase.setFloat(fbdo, "/Monitoring/Voltage/VS", vS);
    Firebase.setFloat(fbdo, "/Monitoring/Voltage/VT", vT);
    Firebase.setFloat(fbdo, "/Monitoring/Voltage/RS", vRS);
    Firebase.setFloat(fbdo, "/Monitoring/Voltage/ST", vST);
    Firebase.setFloat(fbdo, "/Monitoring/Voltage/RT", vRT);

    Firebase.setFloat(fbdo, "/Monitoring/Current/IR", iR);
    Firebase.setFloat(fbdo, "/Monitoring/Current/IS", iS);
    Firebase.setFloat(fbdo, "/Monitoring/Current/IT", iT);

    Firebase.setFloat(fbdo, "/Monitoring/PowerFactor/PF", pfAvg);
    Firebase.setFloat(fbdo, "/Monitoring/Unbalance/Percent", iUnbalance);

    Serial.println("Data terkirim ke Firebase!");
  }
}