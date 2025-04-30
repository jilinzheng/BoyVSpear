#include <BLEDevice.h>
#include <BLEUtils.h>
#include <BLEServer.h>
#include <BLE2902.h> // Needed for notifications

// Service UUID: Identifies the overall service provided by the ESP32
#define SERVICE_UUID "90920b02-5bb3-4d6d-8322-d744ccf56a04"

// Characteristic UUIDs: Identify specific data points within the service
#define CHARACTERISTIC_UUID_X "c7db9d15-3b2c-4e76-b7c1-57e6178dfb6c"
#define CHARACTERISTIC_UUID_Y "d43bfa36-280a-495c-9e10-99f5d1bdc43e"
#define CHARACTERISTIC_UUID_BTN "0bc7ad76-3ffd-4da4-8e3e-09613eddf3c4"


enum CMD {
  LEFT,
  RIGHT,
  UP,
  DOWN,
  NEUTRAL
};

enum BTN_STATE {
  PRESSED,  // software pullup resistor, pressing button gives LOW/0
  RELEASED
};


const int JOY_X = A4; 
const int JOY_Y = A5;
const int JOY_BTN = 6;
const int LEFT_THRESHOLD = 1600;
const int RIGHT_THRESHOLD = 1640;
const int UP_THRESHOLD = 1620;
const int DOWN_THRESHOLD = 1660;

int x_adc, y_adc;
int old_x_cmd = NEUTRAL, old_y_cmd = NEUTRAL, old_btn_val = RELEASED;
int new_x_cmd = NEUTRAL, new_y_cmd = NEUTRAL, new_btn_val = RELEASED;
// true to trigger first update
bool x_cmd_changed = true, y_cmd_changed = true, btn_val_changed = true;
// for setting/writing to BLE characteristics
char x_cmd_str[4], y_cmd_str[4], btn_val_str[4];

BLEServer *pServer = NULL;
BLECharacteristic *pCharacteristicX = NULL;
BLECharacteristic *pCharacteristicY = NULL;
BLECharacteristic *pCharacteristicBtn = NULL;
bool deviceConnected = false;
bool oldDeviceConnected = false;


// --- Server Callback Class ---
// Handles connection and disconnection events
class MyServerCallbacks: public BLEServerCallbacks {
  void onConnect(BLEServer* pServer) {
    deviceConnected = true;
    Serial.println("Device connected");
  }

  void onDisconnect(BLEServer* pServer) {
    deviceConnected = false;
    Serial.println("Device disconnected");
    // Restart advertising to allow new connections
    BLEDevice::startAdvertising();
    Serial.println("Restart advertising");
  }
};


void setup() {
  Serial.begin(115200);
  Serial.println("Starting ESP32 BLE Server for Joystick Data...");
  pinMode(JOY_BTN, INPUT_PULLUP);

  BLEDevice::init("ESP32_Joystick_Server");
  pServer = BLEDevice::createServer();
  pServer->setCallbacks(new MyServerCallbacks()); // Set connection callbacks

  BLEService *pService = pServer->createService(SERVICE_UUID);

  // X-axis Characteristic
  pCharacteristicX = pService->createCharacteristic(
                      CHARACTERISTIC_UUID_X,
                      BLECharacteristic::PROPERTY_READ | // Client can read the value
                      BLECharacteristic::PROPERTY_NOTIFY // Server can notify client of changes
                    );
  pCharacteristicX->addDescriptor(new BLE2902()); // Needed for notifications

  // Y-axis Characteristic
  pCharacteristicY = pService->createCharacteristic(
                      CHARACTERISTIC_UUID_Y,
                      BLECharacteristic::PROPERTY_READ |
                      BLECharacteristic::PROPERTY_NOTIFY
                    );
  pCharacteristicY->addDescriptor(new BLE2902());

  // Button Characteristic
  pCharacteristicBtn = pService->createCharacteristic(
                      CHARACTERISTIC_UUID_BTN,
                      BLECharacteristic::PROPERTY_READ |
                      BLECharacteristic::PROPERTY_NOTIFY
                    );
  pCharacteristicBtn->addDescriptor(new BLE2902());

  // Start the service
  pService->start();

  // Start advertising
  BLEAdvertising *pAdvertising = BLEDevice::getAdvertising();
  pAdvertising->addServiceUUID(SERVICE_UUID);
  pAdvertising->setScanResponse(true);
  pAdvertising->setMinPreferred(0x06);  // Helps with iPhone connectivity issues
  pAdvertising->setMinPreferred(0x12);
  BLEDevice::startAdvertising();

  Serial.println("Characteristics defined. Waiting for client connection...");
}

void loop() {
  x_adc = analogRead(JOY_X); 
  y_adc = analogRead(JOY_Y);
  new_btn_val = digitalRead(JOY_BTN);

  // Serial.print("x_adc = "); Serial.print(x_adc);
  // Serial.print("\t y_adc = "); Serial.println(y_adc);

  if (x_adc < LEFT_THRESHOLD) new_x_cmd = LEFT;
  else if (x_adc > RIGHT_THRESHOLD) new_x_cmd = RIGHT;
  else new_x_cmd = NEUTRAL;

  if (y_adc < UP_THRESHOLD) new_y_cmd = UP;
  else if (y_adc > DOWN_THRESHOLD) new_y_cmd = DOWN;
  else new_y_cmd = NEUTRAL;

  x_cmd_changed = (new_x_cmd != old_x_cmd);
  y_cmd_changed = (new_y_cmd != old_y_cmd);
  btn_val_changed = (new_btn_val != old_btn_val);

  // --- If connected, update characteristics and notify ---
  if (deviceConnected) {
    if (x_cmd_changed) {
      old_x_cmd = new_x_cmd; // update the last cmd
      switch (new_x_cmd) {
        case LEFT: Serial.print("x_cmd = left \n"); break;
        case RIGHT: Serial.print("x_cmd = right \n"); break;
        default: Serial.print("x_cmd = neutral \n");
      }
      sprintf(x_cmd_str, "%d", new_x_cmd);
      pCharacteristicX->setValue(x_cmd_str);
      pCharacteristicX->notify(); // notify client
    }

    if (y_cmd_changed) {
      if (y_cmd_changed) {
        old_y_cmd = new_y_cmd;
        switch (new_y_cmd) {
          case UP: Serial.print("y_cmd = up\n"); break;
          case DOWN: Serial.print("y_cmd = down\n"); break;
          default: Serial.print("y_cmd = neutral\n");
        }
        sprintf(y_cmd_str, "%d", new_y_cmd);
        pCharacteristicY->setValue(y_cmd_str);
        pCharacteristicY->notify();
      }
    }

    if (btn_val_changed) {
      old_btn_val = new_btn_val;
      if (!new_btn_val) Serial.print("btn_val = pressed\n");
      else Serial.print("btn_val = released\n");
      sprintf(btn_val_str, "%d", new_btn_val); // send '0' for pressed, '1' for not pressed
      pCharacteristicBtn->setValue(btn_val_str);
      pCharacteristicBtn->notify();
    }
  }

  // --- Handle connection/disconnection status changes ---
  // If device just disconnected
  if (!deviceConnected && oldDeviceConnected) {
      delay(500); // Give BLE stack time to process
      pServer->startAdvertising(); // Restart advertising
      Serial.println("Start advertising");
      oldDeviceConnected = deviceConnected;
  }

  // If device just connected
  if (deviceConnected && !oldDeviceConnected) {
      oldDeviceConnected = deviceConnected;
  }

  delay(100);
}