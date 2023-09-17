#include "FS.h"
#include <SD.h>
#include <SPI.h>
#include <WiFi.h>
#include <esp_now.h>
#include <AsyncTCP.h>
#include <esp_wifi.h>
#include "index_html.h"
#include <HTTPClient.h>
#include <ESPAsyncWebServer.h>

#define SD_CS_PIN 5

typedef struct struct_message{
  char uuid_tag[50];
  bool hasAccess;
} struct_message;

void handleEvent(AsyncWebSocket *server, AsyncWebSocketClient *client, AwsEventType type, void *arg, uint8_t *data, size_t len);
void receiveMessageViaWebSocket(void *arg, uint8_t *data, size_t len, AsyncWebSocketClient *client);
void OnDataRecv(const uint8_t * mac, const uint8_t *incomingData, int len);
bool appendToFile(fs::FS &fs, const char * path, const char * message);
bool writeToFile(fs::FS &fs, const char * path, const char * message);
String readFromFile(fs::FS &fs, const char * path);
const char* addNewRFID(fs::FS &fs, const char * path, String uuidString);
const char* deleteRFID(fs::FS &fs, const char * path, int idToDelete);
bool checkIfRFIDExists(fs::FS &fs, const char * path, String uuidString);
bool deleteRFID(String uuidString);
bool checkIfRFIDHasAccess(String uuidString);
void initWebSocket();

const char* esp_ssid = "MOPK2";
// const char* esp_pass = "E!ma!$thnM0PK2";
const char* esp_pass = "MOPK2MOPK2";
uint8_t mopk2AccessMACAddress[] = {0x08, 0x3A, 0x8D, 0x15, 0x12, 0x20};
AsyncWebSocketClient* asyncWebsocketClient = NULL;
struct_message exchangeMessage;
struct_message sendMessageBack;
esp_now_peer_info_t peerInfo;
bool addAction = false;
bool deleteAction = false;

char lineByLine[256];

AsyncWebServer asyncWebServer(80);
AsyncWebSocket asyncWebSocket("/rfid/controller");

void setup() {
  // put your setup code here, to run once:
  Serial.begin(115200);
  while(!Serial){
    ;
  }

  WiFi.mode(WIFI_AP_STA);
  while(!WiFi.softAP(esp_ssid, esp_pass)){
    delay(100);
  }

  if(!SD.begin(SD_CS_PIN)){
    Serial.println("Card Mount Failed");
    return;
  }

  if (esp_now_init() != ESP_OK) {
    Serial.println("Error initializing ESP-NOW");
    return;
  }

  memcpy(peerInfo.peer_addr, mopk2AccessMACAddress, 6);
  peerInfo.channel = 0;  
  peerInfo.encrypt = false;
  
  if (esp_now_add_peer(&peerInfo) != ESP_OK){
    Serial.println("Failed to add peer");
    return;
  }

  esp_now_register_recv_cb(OnDataRecv);

  delay(1000);
  initWebSocket();
}

void loop() {
  // put your main code here, to run repeatedly:

}

void receiveMessageViaWebSocket(void *arg, uint8_t *data, size_t len, AsyncWebSocketClient *client) {
  AwsFrameInfo *info = (AwsFrameInfo*)arg;
  if (info->final && info->index == 0 && info->len == len && info->opcode == WS_TEXT) {
    data[len] = 0;
    char* message = (char*)data;
    char* action = strtok(message, ":");

    if(strcmp(action, "ADD") == 0){
      addAction = true;
    }
    
    if(strcmp(action, "DELETE") == 0){
      deleteAction = true;
      char* idToDelete_str = strtok(NULL, ":");
      int idToDelete_int = atoi((const char*)idToDelete_str);
      client->text(deleteRFID(SD, "/RFIDs.txt", idToDelete_int));
    }
  }
}

void handleEvent(AsyncWebSocket *server, AsyncWebSocketClient *client, AwsEventType type, void *arg, uint8_t *data, size_t len) {
  switch (type) {
    case WS_EVT_CONNECT:
      asyncWebsocketClient = client;
      break;
    case WS_EVT_DISCONNECT:
      asyncWebsocketClient = NULL;
      break;
    case WS_EVT_DATA:
      receiveMessageViaWebSocket(arg, data, len, client);
      break;
    case WS_EVT_PONG:
      break;
    case WS_EVT_ERROR:
      break;
  }
}

void initWebSocket() {
  asyncWebSocket.onEvent(handleEvent);
  asyncWebServer.addHandler(&asyncWebSocket);

  asyncWebServer.on("/", HTTP_GET, [](AsyncWebServerRequest *request){
    request->send(200, "text/html", index_html);
  });

  asyncWebServer.begin();
}

void OnDataRecv(const uint8_t * mac, const uint8_t *incomingData, int len){
  memcpy(&exchangeMessage, incomingData, sizeof(exchangeMessage));

  if(addAction){
    // writeToFile(SD, "/RFIDs.txt", (String("1_") + String(exchangeMessage.uuid_tag) + String("_1\n")).c_str());
    asyncWebsocketClient->text(addNewRFID(SD, "/RFIDs.txt", String(exchangeMessage.uuid_tag)));
    addAction = false;
  } else if(deleteAction) {
    deleteAction = false;
  } else {
    strcpy(sendMessageBack.uuid_tag, (const char*)exchangeMessage.uuid_tag);
    if(checkIfRFIDExists(SD, "/RFIDs.txt", String(exchangeMessage.uuid_tag))) {
      sendMessageBack.hasAccess = true;
    }
    else{
      sendMessageBack.hasAccess = false;
    }

    esp_err_t result = esp_now_send(mac, (uint8_t *) &sendMessageBack, sizeof(sendMessageBack));
    if (result != ESP_OK) {
      Serial.println("Error sending the data");
    }
  }
}

const char* addNewRFID(fs::FS &fs, const char * path, String uuidString){
  String wholeFile = readFromFile(fs, path);
  String tmpWholeFile = "";
  String finalResponse = "";

  char* tokenByNewLine = (char*)wholeFile.c_str();
  char* restString = tokenByNewLine;
  char* lastRFID = "";
  char* tmpHelperString = "";

  int lastRFID_id = 0;
  int newRFID_id = lastRFID_id + 1;

  bool rowAdded = false;

  while((tokenByNewLine = strtok_r(restString, "\n", &restString))) {
    rowAdded = false;
    tmpHelperString = tokenByNewLine;
    lastRFID = strtok_r(tmpHelperString, "_", &tmpHelperString);
    if(String(tmpHelperString).endsWith(uuidString+String("_1"))){
      finalResponse = uuidString+String(" already exists with id ")+String(lastRFID);
    } else if(String(tmpHelperString).endsWith(uuidString+String("_0"))){
      tmpWholeFile += String(lastRFID)+String("_")+uuidString+String("_1\n");
      finalResponse = uuidString+String(" added with id ")+String(lastRFID);
      rowAdded = true;
    }

    if(!rowAdded){
      tmpWholeFile += String(lastRFID)+String("_")+String(tmpHelperString)+String("\n");
    }
  }
  writeToFile(fs, path, tmpWholeFile.c_str());
  if(!finalResponse.equals("")){
    return finalResponse.c_str();
  }

  lastRFID_id = atoi((const char*)lastRFID);
  newRFID_id = lastRFID_id + 1;
  if(appendToFile(SD, "/RFIDs.txt", String(String(newRFID_id)+"_"+uuidString+"_1\n").c_str())){
    return (uuidString+String(" added with id ")+String(newRFID_id)).c_str();
  }

  return "Something gone wrong... please try again later";
}

const char* deleteRFID(fs::FS &fs, const char * path, int idToDelete){
  String wholeFile = readFromFile(fs, path);
  String tmpWholeFile = "";
  String finalResponse = "";

  char* tokenByNewLine = (char*)wholeFile.c_str();
  char* restString = tokenByNewLine;
  char* lastRFID = "";
  char* tmpHelperString = "";

  int lastRFID_id = 0;

  bool rowAdded = false;

  while((tokenByNewLine = strtok_r(restString, "\n", &restString))){
    rowAdded = false;
    tmpHelperString = tokenByNewLine;
    lastRFID = strtok_r(tmpHelperString, "_", &tmpHelperString);
    lastRFID_id = atoi((const char*)lastRFID);
    if(lastRFID_id == idToDelete && String(tmpHelperString).endsWith(String("_1"))){
      char* uuidString_str = strtok_r(tmpHelperString, "_", &tmpHelperString);
      tmpWholeFile += String(lastRFID)+String("_")+String(uuidString_str)+String("_0\n");
      finalResponse = String("User deleted with id ")+String(lastRFID);
      rowAdded = true;
    } else if(lastRFID_id == idToDelete && String(tmpHelperString).endsWith(String("_0"))){
      tmpWholeFile += String(lastRFID)+String("_")+String(tmpHelperString)+String("\n");
      finalResponse = String("User already deleted with id ")+String(lastRFID);
      rowAdded = true;
    }

    if(!rowAdded){
      tmpWholeFile += String(lastRFID)+String("_")+String(tmpHelperString)+String("\n");
    }
  }
  writeToFile(fs, path, tmpWholeFile.c_str());
  if(!finalResponse.equals("")){
    return finalResponse.c_str();
  }

  return (String("No user found with id ")+String(idToDelete)).c_str();
}

bool checkIfRFIDExists(fs::FS &fs, const char * path, String uuidString){
  String wholeFile = readFromFile(fs, path);
  char* tokenByNewLine = (char*)wholeFile.c_str();
  char* restString = tokenByNewLine;

  while((tokenByNewLine = strtok_r(restString, "\n", &restString))) {
    if(String(tokenByNewLine).endsWith(uuidString+String("_1"))){
      return true;
    } else if(String(tokenByNewLine).endsWith(uuidString+String("_0"))){
      return false;
    }
  }

  return false;
}

String readFromFile(fs::FS &fs, const char * path){
  String fileContent = "";
  File file = fs.open(path, FILE_READ);
  
  if(!file){
    return "ERROR";
  }
  
  while(file.available()){
    fileContent += file.readString();
  }
  file.close();
  
  return fileContent;
}

bool writeToFile(fs::FS &fs, const char * path, const char * message){
  bool fileWriteStatus = false;
  File file = fs.open(path, FILE_WRITE);
  
  if(!file){
    return false;
  }
  fileWriteStatus = file.print(message);
  
  file.close();
  return fileWriteStatus;
}

bool appendToFile(fs::FS &fs, const char * path, const char * message){
  bool fileAppendStatus = "";
  File file = fs.open(path, FILE_APPEND);
  
  if(!file){
    return false;
  }
  
  fileAppendStatus = file.print(message);
  file.close();

  return fileAppendStatus;
}
