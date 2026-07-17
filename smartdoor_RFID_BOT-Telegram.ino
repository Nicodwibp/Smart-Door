#include <WiFi.h>
#include <WiFiClientSecure.h>
#include <UniversalTelegramBot.h>
#include <ArduinoJson.h>
#include <SPI.h>
#include <MFRC522.h>

#define RELAY_PIN 26
#define SS_PIN 5
#define RST_PIN 22

const char* ssid = "Tenda_5E6D88";
const char* password = "asdf12345";

const char* botToken = "8242848568:AAH2lNoivHpPOrB0WkxAf7gamSCQzqpjn3c";
const char* chatID = "6779468906";

WiFiClientSecure client;
UniversalTelegramBot bot(botToken, client);

MFRC522 rfid(SS_PIN, RST_PIN);

// GANTI UID KARTU PUNYAMU
byte uidValid[4] = {0x93, 0x42, 0x65, 0x56};

bool pintuTerbuka = false;
unsigned long lastTelegramCheck = 0;
unsigned long waktuBuka = 0;
const unsigned long autoLockDelay = 5000;

void setup() {
  Serial.begin(115200);

  pinMode(RELAY_PIN, OUTPUT);
  digitalWrite(RELAY_PIN, HIGH);

  SPI.begin();
  rfid.PCD_Init();

  WiFi.begin(ssid, password);
  client.setInsecure();

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }

  configTime(0, 0, "pool.ntp.org");

  bot.sendMessage(chatID, "Smart door siap. Bot dan RFID aktif", "");
}

void loop() {
  if (millis() - lastTelegramCheck > 1000) {
    cekTelegram();
    lastTelegramCheck = millis();
  }

  cekRFID();
  cekAutoLock();
}

void cekTelegram() {
  int jumlahPesan = bot.getUpdates(bot.last_message_received + 1);

  for (int i = 0; i < jumlahPesan; i++) {
    String text = bot.messages[i].text;
    String id = bot.messages[i].chat_id;

    if (id != chatID) continue;

    if (text == "/bukapintu") {
      bukaPintu();
      bot.sendMessage(chatID, "Pintu dibuka lewat bot", "");
    }

    if (text == "/tutuppintu") {
      tutupPintu();
      bot.sendMessage(chatID, "Pintu dikunci lewat bot", "");
    }

    if (text == "/status") {
      if (pintuTerbuka) {
        bot.sendMessage(chatID, "Status pintu terbuka", "");
      } else {
        bot.sendMessage(chatID, "Status pintu terkunci", "");
      }
    }
  }
}

void cekRFID() {
  if (!rfid.PICC_IsNewCardPresent()) return;
  if (!rfid.PICC_ReadCardSerial()) return;

  bool valid = true;
  for (byte i = 0; i < 4; i++) {
    if (rfid.uid.uidByte[i] != uidValid[i]) {
      valid = false;
      break;
    }
  }

  if (valid) {
    bukaPintu();
    bot.sendMessage(chatID, "RFID valid. Pintu terbuka", "");
  } else {
    bot.sendMessage(chatID, "RFID tidak valid. Akses ditolak", "");
  }

  rfid.PICC_HaltA();
  rfid.PCD_StopCrypto1();
}

void bukaPintu() {
  digitalWrite(RELAY_PIN, LOW);
  pintuTerbuka = true;
  waktuBuka = millis();
}

void tutupPintu() {
  digitalWrite(RELAY_PIN, HIGH);
  pintuTerbuka = false;
}

void cekAutoLock() {
  if (pintuTerbuka && millis() - waktuBuka >= autoLockDelay) {
    tutupPintu();
    bot.sendMessage(chatID, "Pintu tertutup otomatis", "");
  }
}
