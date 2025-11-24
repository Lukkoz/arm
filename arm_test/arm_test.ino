#define CHARACTERISTIC_UUID_ROBOT "c42e42e4-8214-420c-944d-e127cc0f20ba"
#define SERVICE_UUID_ROBOT        "a917e658-9c1a-4901-bbb8-92d54cfa2fdd" 

#define CHARACTERISTIC_UUID_IO "19680482-86af-4892-ab79-962e98f41045"  // UUID for IO
#define SERVICE_UUID_IO        "30a96603-6e34-49d8-9d64-a13f68fefab6"

#define type_device "ROBOT"
#include "Robot_commands.h" 
#include <BLEDevice.h>
#include <BLEServer.h>
#include <BLEUtils.h>
#include <BLE2902.h>

BLEServer* pServer = NULL;
BLECharacteristic* pCharacteristic = NULL;

int stop = false;
int pauze = false;

bool deviceConnected = false;
bool deviceDisconnected = false;

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

void sent_message(String message){
 
    if (pCharacteristic->getValue() == "") {
      String responseData = message;
      pCharacteristic->setValue(responseData.c_str());
      pCharacteristic->notify(); // Notify the client that data has been sent
      delay(3);
      pCharacteristic->setValue("");
    }
  
  Serial.println("message send:");
  Serial.println(message);
}

void get_message(){
        if (pCharacteristic->getValue() != "") {
        String recievedData = pCharacteristic->getValue().c_str();
        pCharacteristic->setValue("");  // Clear after reading
        process_message(recievedData);  // Process command in a separate function
        recievedData = "";
    }
}

void process_message(String command){
    Serial.println("Command arrived:");
    Serial.print(command);
    Serial.println("x");

     if (command == "CONNECT") {
        String Message = "ROBOT_CONNECTED";
        sent_message(Message);
        return;
     }

     int spaceIndex = command.indexOf(' ');
      String typeOfCommand = command.substring(0, spaceIndex);
      String coordinates = command.substring(spaceIndex + 1);
      typeOfCommand.trim();
 if (type_device  == "ROBOT" and typeOfCommand.equals("MoveL")) MoveL(command);
    else if (type_device  == "ROBOT" and typeOfCommand.equals("MoveJ")) MoveJ(command);
    else if (type_device  == "ROBOT" and typeOfCommand.equals("MoveJoint")) MoveJoint(command);
    else if (type_device  == "ROBOT" and typeOfCommand.equals("jogL")) jogL(command);
    else if (type_device  == "ROBOT" and typeOfCommand.equals("jogJ")) jogJ(command);
    else if (type_device == "ROBOT" and typeOfCommand.equals("OffsetJ")) offsetJ(command);
    else if (type_device == "ROBOT" and typeOfCommand.equals("OffsetL")) offsetL(command);
    else if (type_device == "ROBOT" and typeOfCommand.equals("Home")) home_Joints(command);

    // commands tool
    // else if (typeOfCommand.equals("Tool_move_to")) tool_move_to(command);
    // else if (typeOfCommand.equals("Tool_state")) tool_state(command);  

    // commands IO
    // else if (typeOfCommand.equals("IO_digitalWrite")) IO_digitalWrite(command);

    // Settings robot
    else if (type_device == "ROBOT" and typeOfCommand.equals("Set_number_of_joints")) set_number_of_joints(command);
    else if (type_device == "ROBOT" and typeOfCommand.equals("Set_motor_type")) set_motor_type(command);
    else if (type_device == "ROBOT" and typeOfCommand.equals("Set_servo_pin")) set_servo_pin(command);
    else if (type_device == "ROBOT" and typeOfCommand.equals("Set_servo_pulse")) set_servo_pulse(command);
    else if (type_device == "ROBOT" and typeOfCommand.equals("Set_servo_position")) set_servo_pos(command);
    else if (type_device == "ROBOT" and typeOfCommand.equals("Set_motor_pin")) set_motor_pin(command);
    else if (type_device == "ROBOT" and typeOfCommand.equals("Set_ena_pin")) set_enable_pin(command);
    else if (type_device == "ROBOT" and typeOfCommand.equals("Set_switch_pin")) set_switch_pin(command);
    else if (type_device == "ROBOT" and typeOfCommand.equals("Set_max_pos")) set_max_pos(command);
    else if (type_device == "ROBOT" and typeOfCommand.equals("Set_lim_pos")) set_lim_pos(command);
    else if (type_device == "ROBOT" and typeOfCommand.equals("Set_step_deg")) set_step_deg(command);
    else if (type_device == "ROBOT" and typeOfCommand.equals("Set_dir_joints")) set_direction_joint(command);
    else if (type_device == "ROBOT" and typeOfCommand.equals("Set_dh_par")) set_dh_par(command);
    else if (type_device == "ROBOT" and typeOfCommand.equals("Set_speed")) set_speed(command);
    else if (type_device == "ROBOT" and typeOfCommand.equals("Set_home_settings")) set_homing(command);
    else if (type_device == "ROBOT" and typeOfCommand.equals("Set_extra_joint")) set_extra_joint(command);
    

    // Settings Tool
    // else if (typeOfCommand.equals("Set_tools")) set_TOOL(command);
    // else if (typeOfCommand.equals("Set_tool_frame")) set_tool_frame(command);

    // // Settings IO
    // else if (typeOfCommand.equals("Set_io_pin")) set_IO_pin(command);
    else {sent_message(command + "got!");sent_message("END");}
        
}

void setup_arm(){
      Serial.begin(57600);
      initializeBLE();
      pServer->getAdvertising()->start();

}

void arm_loop(){
    get_message();

}

void setup(){
    setup_arm();
}

void loop(){
    arm_loop();
}