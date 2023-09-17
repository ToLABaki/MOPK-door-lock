#include <SPI.h>
#include <WiFi.h>
#include <Base64.h>
#include <esp_now.h>
#include <MFRC522.h>
#include <esp_wifi.h>

#define RFID_CS_PIN   16
#define RFID_RST_PIN  26
#define LOCK_CS_PIN   4
#define GREEN_LED_PIN 32
#define RED_LED_PIN   33
#define BUZZER_PIN    27

typedef struct struct_message{
  char uuid_tag[50];
  bool hasAccess;
} struct_message;

void OnDataRecv(const uint8_t * mac, const uint8_t *incomingData, int len);
void accessResult(bool hasAccess);
void checkMFRC522();

uint8_t memoryMACAddress[] = {0x24, 0x62, 0xAB, 0xDD, 0xAE, 0x28};
struct_message exchangeMessage;
esp_now_peer_info_t peerInfo;
String uidString = "NULL";
bool onAction = false;

MFRC522 rfid(RFID_CS_PIN, RFID_RST_PIN);

void setup() {
  // put your setup code here, to run once:
  Serial.begin(115200);
  while(!Serial){
    ;
  }

  SPI.begin(SCK, MISO, MOSI, SS);

  rfid.PCD_Init(); 
  delay(1000);

  WiFi.mode(WIFI_STA);
  if (esp_now_init() != ESP_OK) {
    Serial.println("Error initializing ESP-NOW");
    return;
  }

  memcpy(peerInfo.peer_addr, memoryMACAddress, 6);
  peerInfo.channel = 0;  
  peerInfo.encrypt = false;
  
  if (esp_now_add_peer(&peerInfo) != ESP_OK){
    Serial.println("Failed to add peer");
    return;
  }

  pinMode(GREEN_LED_PIN, OUTPUT);
  pinMode(RED_LED_PIN, OUTPUT);
  pinMode(BUZZER_PIN, OUTPUT);
  pinMode(LOCK_CS_PIN, OUTPUT);

  ledcSetup(0, 2000, 8);
  ledcAttachPin(BUZZER_PIN, 0);

  esp_now_register_recv_cb(OnDataRecv);
}

void loop() {
  // put your main code here, to run repeatedly:
  if(!rfid.PICC_IsNewCardPresent()){
    return;
  }

  rfid.PICC_ReadCardSerial();
  uidString = String(rfid.uid.uidByte[0]) + "-" + String(rfid.uid.uidByte[1]) + "-" + String(rfid.uid.uidByte[2]) + "-" + String(rfid.uid.uidByte[3]);
  if(!onAction){
    const char* uuid_tag = uidString.c_str();
    strcpy(exchangeMessage.uuid_tag, uuid_tag);
    exchangeMessage.hasAccess = false;

    esp_err_t result = esp_now_send(memoryMACAddress, (uint8_t *) &exchangeMessage, sizeof(exchangeMessage));
    if (result != ESP_OK) {
      Serial.println("Error sending the data");
    }
  }

  delay(2000);
}

void OnDataRecv(const uint8_t * mac, const uint8_t *incomingData, int len){
  memcpy(&exchangeMessage, incomingData, sizeof(exchangeMessage));

  accessResult(exchangeMessage.hasAccess);
}

void accessResult(bool hasAccess){
  uint8_t ledPin;
  int freq1, freq2;

  if(hasAccess){
    ledPin = GREEN_LED_PIN;
    freq1 = 1000;
    freq2 = 1200;

    digitalWrite(LOCK_CS_PIN, HIGH);
  } else {
    ledPin = RED_LED_PIN;
    freq1 = 1000;
    freq2 = 700;
  }

  digitalWrite(ledPin, HIGH);

  ledcWriteTone(0, freq1);
  delay(500);
  ledcWriteTone(0, 0);
  ledcWriteTone(0, freq2);
  delay(500);
  ledcWriteTone(0, 0);
  delay(1000);

  digitalWrite(ledPin, LOW);
  digitalWrite(LOCK_CS_PIN, LOW);
}

void checkMFRC522() {
  Serial.println(F("*****************************"));
  Serial.println(F("MFRC522 Digital self test"));
  Serial.println(F("*****************************"));
  rfid.PCD_DumpVersionToSerial();  // Show version of PCD - MFRC522 Card Reader
  Serial.println(F("-----------------------------"));
  Serial.println(F("Only known versions supported"));
  Serial.println(F("-----------------------------"));
  Serial.println(F("Performing test..."));
  bool result = rfid.PCD_PerformSelfTest(); // perform the test
  Serial.println(F("-----------------------------"));
  Serial.print(F("Result: "));
  if (result)
    Serial.println(F("OK"));
  else
    Serial.println(F("DEFECT or UNKNOWN"));
  Serial.println();
}