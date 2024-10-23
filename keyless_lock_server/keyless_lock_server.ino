/*
  Based on Neil Kolban example for IDF: https://github.com/nkolban/esp32-snippets/blob/master/cpp_utils/tests/BLE%20Tests/SampleNotify.cpp
  Ported to Arduino ESP32 by Evandro Copercini
  updated by chegewara and MoThunderz
*/
#include <BLEDevice.h>
#include <BLEServer.h>
#include <BLEUtils.h>
#include <BLE2902.h>
#include <Crypto.h>
#include <AESLib.h>

// Initialize all pointers
BLEServer* pServer = NULL;                        // Pointer to the server
BLECharacteristic* pCharacteristic = NULL;      // Pointer to Characteristic 2
BLE2902 *pBLE2902;                              // Pointer to BLE2902 of Characteristic

// Some variables to keep track on device connected
bool deviceConnected = false;
bool oldDeviceConnected = false;

// Variable that will continuously be increased and written to the client
uint32_t value = 0;

// See the following for generating UUIDs:
// https://www.uuidgenerator.net/
// UUIDs used in this example:
#define SERVICE_UUID          "4fafc201-1fb5-459e-8fcc-c5c9c331914b"
#define CHARACTERISTIC_UUID "1c95d5e3-d8f7-413a-bf3d-7a2e5d7be87e"


byte AES_KEY[] = { 0x2B, 0x7E, 0x15, 0x16, 0x28, 0xAE, 0xD2, 0xA6, 0xAB, 0xF7, 0x15, 0x88, 0x09, 0xCF, 0x4F, 0x3C };
AES aes;


// Callback function that is called whenever a client is connected or disconnected
class MyServerCallbacks: public BLEServerCallbacks {
    void onConnect(BLEServer* pServer) {
      deviceConnected = true;
    };

    void onDisconnect(BLEServer* pServer) {
      deviceConnected = false;
      Serial.println("Client disconnected. Restarting...");
      delay(1000);
      ESP.restart();    
    }
};

void setup() {
  Serial.begin(115200);
  // Create the BLE Device
  BLEDevice::init("ESP32");
  // Create the BLE Server
  pServer = BLEDevice::createServer();
  pServer->setCallbacks(new MyServerCallbacks());

  // Create the BLE Service
  BLEService *pService = pServer->createService(SERVICE_UUID);

  // Create a BLE Characteristic
  pCharacteristic = pService->createCharacteristic(
                      CHARACTERISTIC_UUID,
                      BLECharacteristic::PROPERTY_READ   |
                      BLECharacteristic::PROPERTY_WRITE  |                      
                      BLECharacteristic::PROPERTY_NOTIFY
                    );  

  // Create a BLE Descriptor  

  pBLE2902 = new BLE2902();
  pBLE2902->setNotifications(true);
  pCharacteristic->addDescriptor(pBLE2902);

  // Start the service
  pService->start();

  // Start advertising
  BLEAdvertising *pAdvertising = BLEDevice::getAdvertising();
  pAdvertising->addServiceUUID(SERVICE_UUID);
  pAdvertising->setScanResponse(false);
  pAdvertising->setMinPreferred(0x0);  // set value to 0x00 to not advertise this parameter
  BLEDevice::startAdvertising();
  Serial.println("Waiting a client connection to notify...");
}

void loop() {
  // notify changed value
  if (deviceConnected) {
    //讀
    std::string rxValue = pCharacteristic->getValue();
    String rxValue_String=rxValue.c_str();
    //std string to byte
    byte* rx_byte = (byte*)rxValue_String.c_str(); 
    Serial.println("rxValue: " + String(rxValue.c_str()));
    Serial.println("rxValue.length(): " + String(rxValue.length()));
    // 印出未解密的資訊
    Serial.println("Characteristic 2 (收到的): ");
    for (int i = 0; i < 16 ; i++) {
      Serial.print(rx_byte[i], HEX);
      Serial.print(",");
    }
    Serial.print(rxValue.c_str());      
    //解密
    byte decryptedBytes[16];
    byte iv[16] = {0};
    aes.set_key((byte*)AES_KEY, sizeof(AES_KEY));
    aes.decrypt(rx_byte, decryptedBytes);
    //Serial.println("decrypt succes"+String(aes.decrypt(rx_byte, decryptedBytes))); //印出是否解密成功
    Serial.println("rx_byte length: " + String(rxValue_String.length()));
    // byte to string
    String decryptedString((char*)decryptedBytes);    
    Serial.println("decrypted message:" + decryptedString);
    Serial.println("----------------------");
    //========================================== 傳訊息
    String originalString = "true!_" + decryptedString  ; //經過實驗小魚等魚15個字元
    //string to byte
    byte* byteArray = (byte*)originalString.c_str();
    //加密
    byte encrypted_byte[16];
    aes.set_key((byte*)AES_KEY, sizeof(AES_KEY));
    aes.encrypt(byteArray, encrypted_byte);
    //印出加密後的 byte array
    Serial.print("Characteristic 2 (加密後): ");
    for (int i = 0; i < 16 ; i++) {
      Serial.print(encrypted_byte[i], HEX);
      Serial.print(",");
    }
    Serial.println(" ");
    //要轉成uint8 array  將string轉為uint8 array 以便指定送出的大小  指定傳16 byte
    String encrypted_string((char*)encrypted_byte);         //byte to string
    uint8_t* encrypted_uint8 = (uint8_t*)encrypted_string.c_str();    //string to uint8
    pCharacteristic->setValue(encrypted_uint8, 16);
    Serial.println("the encrypted_string:"+ encrypted_string);
    Serial.println("=============================");  
    delay(500);
  }
  delay(500);
}