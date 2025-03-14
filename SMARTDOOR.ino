#include <Wire.h>
#include <LiquidCrystal_I2C.h>
#include <Keypad.h>
#include <ESP32Servo.h>
#include <SPI.h>
#include <MFRC522.h>

#include <WiFi.h>
#include <WiFiClientSecure.h>
#include <UniversalTelegramBot.h>
#include <ArduinoJson.h>

Servo myServo;
int servoPosition = 90; // Posisi awal servo
const int bukaPosisi = 90;  // Posisi servo untuk "Kunci Dibuka" untuk status tetapi juga dapat di panggil
const int tutupPosisi = 0;  // Posisi servo untuk "Pintu Terkunci" untuk status tetapi juga dapat di panggil

#define servo 2
#define RST_PIN 4  // Pin RST RFID
#define SS_PIN 5   // Pin SDA RFID

const int buzzer = 15;
const char* ssid = "Home"; // Nama WIFI yg akan di hubungkan
const char* passwordwifi = "Bukasaja"; //Password dari sandi tersebut

// inisialisasi Bot Token
#define BOTtoken "yourToken"  // Bot Token dari BotFather

// chat id dari @myidbot
#define CHAT_ID "YourChatID" // ID pemilik telegram tanpa memasukan id di sini tak dapat menggunakan telenya

WiFiClientSecure client; // wifi sebagai client
UniversalTelegramBot bot(BOTtoken, client); //Menghubungkan dengan telegram berdasarkan dengan Token

int botRequestDelay = 1000; // delay Chat
unsigned long lastTimeBotRan; 


// Inisialisasi RFID
MFRC522 rfid(SS_PIN, RST_PIN); // pin yg terhubung pada rfid

// UID kartu/tag RFID yang valid
byte validUIDs[][7] = {
  {0x00, 0x00, 0x00, 0x00},
  {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00} // UID kartu yang valid "7" karena id paling banyaknya 7
  
};
const int totalUIDs = sizeof(validUIDs) / 7;

// Inisialisasi keypad
const byte ROWS = 4;  // jumlah baris pada keypad
const byte COLS = 4;  // jumlah kolom pada keypad
char keys[ROWS][COLS] = {
  {'1', '2', '3', 'A'},
  {'4', '5', '6', 'B'},
  {'7', '8', '9', 'C'},
  {'*', '0', '#', 'D'}
};
byte rowPins[ROWS] = {13, 12, 14, 27}; // sesuaikan pin
byte colPins[COLS] = {26, 25, 33, 32}; // sesuaikan pin
Keypad keypad = Keypad(makeKeymap(keys), rowPins, colPins, ROWS, COLS);

int inputNumber = 0;  // Variabel penampung nilai angka
const int password = 1234;  // Password

// Inisialisasi LCD
LiquidCrystal_I2C lcd(0x27, 20, 4);  // Alamat I2C untuk LCD 20x4


// dari library chat tele
void handleNewMessages(int numNewMessages) {
  Serial.println("handleNewMessages");
  Serial.println(String(numNewMessages));

  for (int i=0; i<numNewMessages; i++) {
    String chat_id = String(bot.messages[i].chat_id); // mengidentifikasi apakah id tele yg mengirim pesan sama dengan yg terdaftar
    if (chat_id != CHAT_ID){
      bot.sendMessage(chat_id, "Unauthorized user", ""); // jika id tele pengguna di masukan dengan cara yg tak tepat
      continue;
    }
    
    String text = bot.messages[i].text; // mengirim chat berdasarkan variabel i
    Serial.println(text);

    String from_name = bot.messages[i].from_name; // daftar variabel i. triger printah dengan "/start"

     if (text == "/start") {
      String control = "Selamat Datang, " + from_name + ".\n";
      control += "Gunakan Commands Di Bawah Untuk Control Pintu.\n\n";
      control += "/BukaPintu Untuk Buka Pintu \n";
      control += "/BukaSaja Untuk Buka Pintu saja \n";
      control += "/TutupPintu Untuk Tutup Pintu \n";
      control += "/Alarm Untuk Alarm Keamanan \n";
      control += "/Status Untuk Cek Pintu Saat Ini \n";
      bot.sendMessage(chat_id, control, "");
    }

    // menjalankan perintah di atas berdasarkan parameter di atas
    if (text == "/BukaPintu") {
      bot.sendMessage(chat_id, "Membuka Pintu...");
      buzzerBenar();
      openLock();
    }
     if (text == "/BukaSaja") {
      bot.sendMessage(chat_id, "Membuka Pintu...");
      buzzerBenar();
      BukaSaja();
    }
    if (text == "/TutupPintu") {
      bot.sendMessage(chat_id, "Pintu Ditutup...");
      buzzerBenar();
      TutupPintu();
    }
    if (text == "/Alarm") {
      bot.sendMessage(chat_id, "Alaram Nyala...");
      lcd.clear();
      lcd.setCursor(0, 0);
      lcd.print("Anda Tidak");
      lcd.setCursor(0, 1);
      lcd.print("Diundang!!!");
      buzzerSalah();
      delay(3000);
      lcd.clear();
      lcd.setCursor(0, 0);
      lcd.print("Masukan Sandi!");
    }
  
    if (text == "/Status") {
      if (servoPosition == 90) {
        // Mengirim status pintu terbuka melalui bot
        bot.sendMessage(chat_id, "Kunci Terbuka");
      }else {
        // Mengirim status posisi servo lainnya
        bot.sendMessage(chat_id, "Pintu Terkunci");
      }
    }
  }
}



void setup() {
  Serial.begin(115200);  // Untuk debugging
  lcd.init();
  lcd.backlight();
  lcd.setCursor(0, 0);
  lcd.print("SELAMAT DATANG"); 
  lcd.setCursor(0, 1);
  lcd.print("Masukan Sandi!");

  // Inisialisasi servo
  myServo.attach(servo); // pin servo yg telah di inisiasi pin servo pada pin 2
  myServo.write(90); // posisi awal 90

  // Inisialisasi buzzer
  pinMode(buzzer, OUTPUT);

  // Inisialisasi RFID
  SPI.begin();           // Memulai SPI
  rfid.PCD_Init();       // Inisialisasi RC522
  delay(2000);           // Waktu inisialisasi

  // Koneksi Ke Wifi
  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, passwordwifi);
  #ifdef ESP32
    client.setCACert(TELEGRAM_CERTIFICATE_ROOT);
  #endif
  while (WiFi.status() != WL_CONNECTED) {
    delay(1000);
    Serial.println("Connecting to WiFi..");
  }
  // Print ESP32 Local IP Address
  Serial.println(WiFi.localIP());
}

void loop() {
  // Baca RFID
  if (rfid.PICC_IsNewCardPresent() && rfid.PICC_ReadCardSerial()) {
      for (byte i = 0; i < rfid.uid.size; i++) {
      Serial.print(rfid.uid.uidByte[i], HEX);
      if (i < rfid.uid.size - 1) Serial.print(":");
    }
    Serial.println();

    if (checkRFID()) {
      lcd.clear();
      lcd.setCursor(0, 0);
      lcd.print("RFID Valid!");
      buzzerBenar();
      openLock();  // Buka kunci jika kartu/tag valid
      Serial.println("Chat Telegram...");
      bot.sendMessage(CHAT_ID, "Buka pintu BERHASIL via RFID.", "");
    } else {
      lcd.clear();
      lcd.setCursor(0, 0);
      lcd.print("RFID Tidak Valid!");
      buzzerSalah();
      delay(2000);
      lcd.clear();
      lcd.setCursor(0, 0);
      lcd.print("Masukan Sandi!");
      Serial.println("Chat Telegram...");
      bot.sendMessage(CHAT_ID, "Buka pintu GAGAL via RFID.", "");
    }
    rfid.PICC_HaltA(); // Hentikan pembacaan kartu/tag
  }

  // Baca Keypad
  char key = keypad.getKey();
  if (key) {
    lcd.clear();
    Serial.print("Tombol Ditekan: ");
    Serial.println(key);

    if (key >= '0' && key <= '9') {  // Jika angka ditekan
      inputNumber = inputNumber * 10 + (key - '0');  // Masukkan angka
      lcd.setCursor(0, 0);
      lcd.print("Masukan Sandi!");
      lcd.setCursor(0, 1);
      lcd.print("Input: ");
      lcd.print(inputNumber);
    } else if (key == '#') {  // Jika tombol '#' ditekan sebagai input data
      lcd.clear();
      if (inputNumber == password) {
        lcd.clear();
        lcd.setCursor(0, 0);
        lcd.print("Password Benar!");
        buzzerBenar();
        openLock();  // Buka kunci jika sandi benar
        Serial.println("Chat Telegram...");
        bot.sendMessage(CHAT_ID, "Buka pintu BERHASIL via KEYPAD.", "");
      } else {
        Serial.println("Password Salah!");
        lcd.setCursor(0, 0);
        lcd.print("Password Salah!");
        buzzerSalah();
        delay(2000);
        lcd.clear();
        lcd.setCursor(0, 0);
        lcd.print("Masukan Sandi!");
        Serial.println("Chat Telegram...");
        bot.sendMessage(CHAT_ID, "Buka pintu GAGAL via KEYPAD.", "");
      }
      inputNumber = 0; 
    } else if (key == '*') {  // Reset input jika tombol '*' ditekan
      inputNumber = 0;
      lcd.clear();
      lcd.setCursor(0, 0);
      lcd.print("Masukan Sandi!");
    }
  }
  
  if (millis() > lastTimeBotRan + botRequestDelay)  {
    int numNewMessages = bot.getUpdates(bot.last_message_received + 1);

    while(numNewMessages) {
      Serial.println("got response");
      handleNewMessages(numNewMessages);
      numNewMessages = bot.getUpdates(bot.last_message_received + 1);
    }
    lastTimeBotRan = millis();
  }

}

// Fungsi untuk memeriksa UID RFID. UID di baca perHEX
bool checkRFID() {
  for (int i = 0; i < totalUIDs; i++) {
    bool match = true;
    for (byte j = 0; j < 4; j++) {
      if (rfid.uid.uidByte[j] != validUIDs[i][j]) {
        match = false;
        break;
      }
    }
    if (match) {
      Serial.println("RFID Valid!");
      delay(1000);
      buzzerBenar();
      return true;
    }
  }
  Serial.println("RFID Tidak Valid!");
  buzzerSalah();
  return false;
}

// Fungsi untuk membuka kunci otomatis
void openLock() {

  servoPosition = tutupPosisi; //untuk refresh "servoPosition" ke yg baru
  lcd.clear();
  lcd.setCursor(0, 1);
  lcd.print("Kunci Dibuka!");

  Serial.println("Membuka kunci...");
  Serial.println("Servo ke posisi 0 derajat");
  myServo.write(0);  // Menggerakkan servo ke posisi 0 derajat
  delay(3000);       // Tunggu selama 2 detik

  Serial.println("Servo ke posisi 90 derajat");
  myServo.write(90); // Menggerakkan servo ke posisi 90 derajat
  delay(2000);       // Tunggu selama 2 detik

  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Masukan Sandi!");
}

// fungsi unyuk membuka kunci saja dan tak balik lagi
void BukaSaja() {

  servoPosition = bukaPosisi; //untuk refresh "servoPosition" ke yg baru
  lcd.clear();
  lcd.setCursor(0, 1);
  lcd.print("Kunci Dibuka!");

  Serial.println("Membuka kunci...");
   Serial.println("Servo ke posisi 0 derajat");
  myServo.write(0);  // Menggerakkan servo ke posisi 0 derajat atau Pinyu Terbuka
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Masukan Sandi!");
}

// fungsi untuk tutup pintu atau servo ke posisi 90
void TutupPintu() {

  servoPosition = tutupPosisi; //untuk refresh "servoPosition" ke yg baru
  lcd.clear();
  lcd.setCursor(0, 1);
  lcd.print("Pintu Dikunci!");

  Serial.println("Pintu Dikunci...");
  Serial.println("Servo ke posisi 90 derajat");
  myServo.write(90);  // Menggerakkan servo ke posisi 90 derajat atau Pintu Tertutup
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Masukan Sandi!");
}

// untuk membunyikan buzzer benar secara manual
void buzzerBenar() {
  for (int i = 0; i < 1; i++) { 
    digitalWrite(buzzer, HIGH); // Aktifkan buzzer
    delay(200);                    // Tunggu 200 ms
    digitalWrite(buzzer, LOW);  // Matikan buzzer
    delay(100);                    // Jeda antar bunyi
  }
}

// untuk membunyikan buzzer salah secara manual
void buzzerSalah() {
  for (int i = 0; i < 3; i++) { 
    digitalWrite(buzzer, HIGH); // Aktifkan buzzer
    delay(300);                    // Tunggu 300 ms
    digitalWrite(buzzer, LOW);  // Matikan buzzer
    delay(100);                    // Jeda antar bunyi
  }
}