/**
 * A BLE client example that is rich in capabilities.
 * There is a lot new capabilities implemented.
 * author unknown
 * updated by chegewara and MoThunderz
 */

#include "BLEDevice.h"
#include <Crypto.h>
#include <AESLib.h>
AES aes;
byte AES_KEY[] = { 0x2B, 0x7E, 0x15, 0x16, 0x28, 0xAE, 0xD2, 0xA6, 0xAB, 0xF7, 0x15, 0x88, 0x09, 0xCF, 0x4F, 0x3C };
int rssi_threshold = -30 ;
BLEClient*  pClient  = BLEDevice::createClient();
int send_times = 0;
String originalString="";
int ledPin = 32 ;


// Define UUIDs:
static BLEUUID serviceUUID("4fafc201-1fb5-459e-8fcc-c5c9c331914b");
static BLEUUID charUUID("1c95d5e3-d8f7-413a-bf3d-7a2e5d7be87e");

// Some variables to keep track on device connected
static boolean doConnect = false;       //當true代表正在執行連線手續
static boolean connected = false;       //表示可以連線，先在connectToServer這個函式中被附值  其值是true才能在我們的loop執行真正要做的事
static boolean doScan = false;

// Define pointer for the BLE connection
static BLEAdvertisedDevice* myDevice;
BLERemoteCharacteristic* pRemoteChar;

// Callback function that is called whenever a client is connected or disconnected
class MyClientCallback : public BLEClientCallbacks {                                                              //(8)    我猜這個在內部程式會call 判斷並進入是哪個狀態 總之內部他會自己判斷
  void onConnect(BLEClient* pclient) {
  }

  void onDisconnect(BLEClient* pclient) {
    connected = false;                                                                                                                  
    Serial.println("onDisconnect");
  }
};

// Function that is run whenever the server is connected
bool connectToServer() {
  Serial.print("Forming a connection to ");                                                                             //(6)
  Serial.println(myDevice->getAddress().toString().c_str());
  pClient->setClientCallbacks(new MyClientCallback());                                                                  //go to call back(8)  up side 

  // Connect to the remove BLE Server.
  pClient->connect(myDevice);  // if you pass BLEAdvertisedDevice instead of address, it will be recognized type of peer device address (public or private)
  Serial.println(" - Connected to server");                                                                             //(9)  

  // 確認 RSSI 是否超越 threshold
  if (myDevice->haveRSSI() && myDevice->getRSSI() <= rssi_threshold) {
    Serial.println("RSSI below threshold, not connected");
    pClient->disconnect();
    return false;
  }

  // Obtain a reference to the service we are after in the remote BLE server.
  BLERemoteService* pRemoteService = pClient->getService(serviceUUID);
  if (pRemoteService == nullptr) {
    Serial.print("Failed to find our service UUID: ");
    Serial.println(serviceUUID.toString().c_str());
    pClient->disconnect();
    return false;
  }
  Serial.println(" - Found our service");

  connected = true;
  pRemoteChar = pRemoteService->getCharacteristic(charUUID);

  if(connectCharacteristic(pRemoteService, pRemoteChar) == false)            
    connected = false;

  if(connected == false) {
    pClient-> disconnect();
    Serial.println("At least one characteristic UUID not found");
    return false;
  }
  return true;
}

// Function to chech Characteristic
bool connectCharacteristic(BLERemoteService* pRemoteService, BLERemoteCharacteristic* l_BLERemoteChar) {
  // Obtain a reference to the characteristic in the service of the remote BLE server.
  if (l_BLERemoteChar == nullptr) {
    Serial.print("Failed to find one of the characteristics");
    Serial.print(l_BLERemoteChar->getUUID().toString().c_str());
    return false;
  }
  Serial.println(" - Found characteristic: " + String(l_BLERemoteChar->getUUID().toString().c_str()));
  return true;
}

// Scan for BLE servers and find the first one that advertises the service we are looking for.
class MyAdvertisedDeviceCallbacks: public BLEAdvertisedDeviceCallbacks {
  //Called for each advertising BLE server.
  void onResult(BLEAdvertisedDevice advertisedDevice) {
    Serial.print("BLE Advertised Device found: ");                                                                           //(3)
    Serial.println(advertisedDevice.toString().c_str());
    // We have found a device, let us now see if it contains the service we are looking for.
    if (advertisedDevice.haveServiceUUID() && advertisedDevice.isAdvertisingService(serviceUUID) ) {  //(4-1) 如果有我們要的serviceID  
      BLEDevice::getScan()->stop();                                                                                                                  
      myDevice = new BLEAdvertisedDevice(advertisedDevice);
      doConnect = true;                                                                                                      //(4-2)則把doConnect 設為True
      doScan = true;
    } // Found our server  
  } // onResult
}; // MyAdvertisedDeviceCallbacks


String generateRandomString(int length) {
  String randomString = "";
  for (int i = 0; i < length; i++) {
    char randomChar = random(1, 126); // 生成隨機的ASCII值
    randomString += randomChar; // 將隨機字符加到randomString中
  }
  return randomString;
}

void setup() {
  Serial.begin(115200);
  Serial.println("Starting Arduino BLE Client application...");                                                             //(1)
  BLEDevice::init("");
  // Retrieve a Scanner and set the callback we want to use to be informed when we
  // have detected a new device.  Specify that we want active scanning and start the
  // scan to run for 5 seconds.
  BLEScan* pBLEScan = BLEDevice::getScan();
  pBLEScan->setAdvertisedDeviceCallbacks(new MyAdvertisedDeviceCallbacks());                                                //(2)
  pBLEScan->setInterval(1349);
  pBLEScan->setWindow(449);
  pBLEScan->setActiveScan(true);
  pBLEScan->start(5, false);
  pinMode(ledPin, OUTPUT); // 设置GPIO pin為输出模式
} // End of setup.

void loop() {
  // If the flag "doConnect" is true then we have scanned for and found the desired
  // BLE Server with which we wish to connect.  Now we connect to it.  Once we are 
  // connected we set the connected flag to be true.
  if (doConnect == true) {               
    if (connectToServer()) {            //connectToServer return true 代表確定連接好                                     //(5) 則進入connectToServer()  其會順便控制connected flag (是true才能在我們的loop執行真正要做的事)
      Serial.println("We are now connected to the BLE Server.");
      //這邊是(首次或重新)連接後要再次初始化的參數
      send_times = 0;
    } else {
      Serial.println("We have failed to connect to the server; there is nothin more we will do.");
    }
    doConnect = false;
  }

  // If we are connected to a peer BLE Server, update the characteristic each time we are reached
  // with the current time since boot.
  if (connected) {
    //讀資料 
    std::string rxValue = pRemoteChar->readValue();
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
    Serial.println("rx_byte length: " + String(rxValue_String.length()));
    // byte to string
    String decryptedString((char*)decryptedBytes);    
    Serial.println("decrypted message:" + decryptedString);

    Serial.println("------------------");
    if (send_times == 0){       
      originalString = generateRandomString(9);
      Serial.println("random originalString:" + originalString);
    }
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
    pRemoteChar->writeValue(encrypted_uint8, 16);
    Serial.println("the encrypted_string:" + encrypted_string);

    //判斷是否正確 正確則開門並斷開
    String anwser = "true!_" + originalString ;
    if (! strcmp(decryptedString.c_str(),anwser.c_str() )){
      Serial.println("correct !");
      pClient-> disconnect();
      // 在此加入開燈
      digitalWrite(ledPin, HIGH); // 點亮LED
      delay(2000); 
      digitalWrite(ledPin, LOW); // 關閉LED
      delay(100); 
    }
    //超過幾次都沒有正確  則斷開
    Serial.print("send_times = ");
    Serial.println(send_times);
    if(send_times >= 8 ){
      Serial.println("too much try,this is a bad guy");
      pClient-> disconnect();
    }
    send_times++;
    Serial.println("===============================");
  }else if(doScan){
    BLEDevice::getScan()->start(0);  // this is just example to start scan after disconnect, most likely there is better way to do it in arduino
  }

  // In this example "delay" is used to delay with one second. This is of course a very basic 
  // implementation to keep things simple. I recommend to use millis() for any production code
  delay(500);
}