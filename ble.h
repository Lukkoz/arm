
#ifdef ARDUINO_ARCH_ESP321

#include <BLEDevice.h>
#include <BLEServer.h>
#include <BLEUtils.h>
#include <BLE2902.h>
BLEServer* pServer = NULL;
BLECharacteristic* pCharacteristic = NULL;

class MyServerCallbacks1 : public BLEServerCallbacks {
  void onConnect(BLEServer* pServer) override {
    Serial.println("Device connected");
    deviceConnected = true;
    deviceDisconnected = false;
  }

  void onDisconnect(BLEServer* pServer) override {
    Serial.println("Device disconnected");
    pServer->startAdvertising();
    deviceConnected = false;
    deviceDisconnected = true;


    // release all the pins on discontection
  }
};

void initializeBLE(){
  // Initialize BLE
  BLEDevice::init("ARM_TEST");  // Custom BLE name here

  // Create BLE server
  pServer = BLEDevice::createServer();
  pServer->setCallbacks(new MyServerCallbacks1());

  if (pServer == NULL) {
    Serial.println("Failed to create BLE server");
    while (1); // Halt the program
  }

    String SERVICE_UUID = "";
    String CHARACTERISTIC_UUID = "";
    SERVICE_UUID = SERVICE_UUID_ROBOT;
    CHARACTERISTIC_UUID = CHARACTERISTIC_UUID_ROBOT;
  


  BLEService *pService = pServer->createService(SERVICE_UUID);
  pCharacteristic = pService->createCharacteristic(
                    CHARACTERISTIC_UUID_ROBOT,
                    BLECharacteristic::PROPERTY_READ |
                    BLECharacteristic::PROPERTY_WRITE |
                    BLECharacteristic::PROPERTY_NOTIFY
                  );

  // Add descriptor for notifications
  pCharacteristic->addDescriptor(new BLE2902());

  // Start the BLE service
  pService->start();
}

#endif 