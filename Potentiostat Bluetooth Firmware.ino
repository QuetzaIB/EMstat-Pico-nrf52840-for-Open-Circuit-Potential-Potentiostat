#include <Arduino.h>
#include "MathHelpers.h"
#include "MSComm.h"
#include "Adafruit_TinyUSB.h"
#include <bluefruit.h>

BLEService tempService = BLEService(0x1809);
BLECharacteristic controlCharacteristic = BLECharacteristic("12345678-1234-5678-1234-56789abcdef0");  // Custom UUID
BLECharacteristic tempCharacteristic = BLECharacteristic("12345678-1234-5678-1234-56789abcdef2");     // Temperature characteristic UUID

bool startMeasurement = false;

// Select demo
// 0 = LSV (connect to WE B of PalmSens Dummy Cell = 10k ohm resistor)
// 1 = EIS (connect to WE C of PalmSens Dummy Cell = Randless Circuit)
// 2 = SWV (connect to WE B of PalmSens Dummy Cell = 10k ohm resistor)
// 3 = OCP (connect WE to RE, only 2 leads)
#define DEMO_SELECT 3

// Please uncomment one of the lines below to select your device type
#define DEVICE_TYPE DT_ESPICO
//#define DEVICE_TYPE DT_ES4

#ifndef DEVICE_TYPE
#error "No device type selected, please uncomment one of the options above"
#endif

// Please uncomment one of the lines below to select the correct baudrate for your MethodSCRIPT device
// EmStat Pico: 230400
// EmStat4:     921600 (230400 for FW <= 1.1)
//#define MSCRIPT_DEV_BAUDRATE 921600
#define MSCRIPT_DEV_BAUDRATE 230400

#ifndef MSCRIPT_DEV_BAUDRATE
#error "No baudrate selected, please uncomment one of the options above"
#endif

// Set this to the port that will communicate with the MethodSCRIPT device.
#define SerialMSDev Serial1

int _nDataPoints = 0;
char _versionString[30];

static bool s_printSent = false;
static bool s_printReceived = false;
char const* CMD_VERSION_STRING = "t\n";

//LSV MethodSCRIPT

char const* LSV_ON_10KOHM = "e\n"
                            "var c\n"
                            "var p\n"
                            "set_pgstat_mode 3\n"
                            "set_max_bandwidth 200\n"
                            "set_range ba 500u\n"
                            "set_e -500m\n"
                            "cell_on\n"
                            "wait 1\n"
                            "meas_loop_lsv p c -500m 500m 50m 100m\n"
                            "pck_start\n"
                            "pck_add p\n"
                            "pck_add c\n"
                            "pck_end\n"
                            "endloop\n"
                            "cell_off\n"
                            "\n";

// OCP MethodSCRIPT
char const* OCP_ON = "e\n"
                     "var p\n"
                     "set_pgstat_mode 3\n"
                     "set_max_bandwidth 200\n"
                     "set_range ba 500u\n"
                     "set_e -500m\n"
                     "cell_off\n"
                     "wait 1\n"
                     // This value is tuned to give a continuous readout, it takes 1 measurement in 1000 ms
                     "meas_loop_ocp p 1000m 1\n"
                     "pck_start\n"
                     "pck_add p\n"
                     "pck_end\n"
                     "endloop\n"
                     "cell_off\n"
                     "\n";

// SWV MethodSCRIPT
char const* SWV_ON_10KOHM = "e\n"
                            "var p\n"
                            "var c\n"
                            "var f\n"
                            "var r\n"
                            "set_pgstat_mode 3\n"
                            "set_max_bandwidth 100000\n"
                            "set_range ba 100u\n"
                            "set_e -500m\n"
                            "cell_on\n"
                            "wait 1\n"
                            "meas_loop_swv p c f r -500m 500m 5m 25m 10\n"
                            "pck_start\n"
                            "pck_add p\n"
                            "pck_add c\n"
                            "pck_end\n"
                            "endloop\n"
                            "cell_off\n"
                            "\n";

// EIS MethodSCRIPT
char const* EIS_ON_WE_C = "e\n"
                          "var f\n"
                          "var r\n"
                          "var j\n"
                          "set_pgstat_chan 0\n"
                          "set_pgstat_mode 3\n"
                          "set_range ba 50u\n"
                          "set_autoranging ba 50u 500u\n"
                          "cell_on\n"
                          "meas_loop_eis f r j 15m 200k 100 51 0m\n"
                          "pck_start\n"
                          "pck_add f\n"
                          "pck_add r\n"
                          "pck_add j\n"
                          "pck_end\n"
                          "endloop\n"
                          "on_finished:\n"
                          "cell_off\n"
                          "\n";

MSComm _msComm;

int write_wrapper(char c) {
  if (s_printSent) {
    Serial.write(c);
  }
  return SerialMSDev.write(c);
}

int read_wrapper() {
  int c = SerialMSDev.read();
  if (s_printReceived && (c != -1)) {
    Serial.write(c);
  }
  return c;
}

int VerifyMSDevice() {
  int i = 0;
  int isConnected = 0;
  RetCode code;

  SendScriptToDevice(CMD_VERSION_STRING);
  while (!SerialMSDev.available()) {
  }
  while (SerialMSDev.available()) {
    code = ReadBuf(&_msComm, _versionString);
    if (code == CODE_VERSION_RESPONSE) {
      if (strstr(_versionString, "espbl") != NULL) {
        Serial.println("EmStat Pico is connected in boot loader mode.");
        isConnected = 0;

      } else if (strstr(_versionString, "espico") != NULL) {
        Serial.println("Connected to EmStat Pico");
        isConnected = 1;
      } else if (strstr(_versionString, "es4_lr") != NULL) {
        Serial.println("Connected to EmStat4 LR");
        isConnected = 1;
      } else if (strstr(_versionString, "es4_hr") != NULL) {
        Serial.println("Connected to EmStat4 HR");
        isConnected = 1;
      } else if (strstr(_versionString, "es4_bl") != NULL) {
        Serial.println("Connected to EmStat4 Bootloader");
        isConnected = 0;
      }

    } else if (strstr(_versionString, "*\n") != NULL) {
      break;

    } else {
      Serial.println("Connected device is not MethodSCRIPT device");
      isConnected = 0;
    }
    delay(20);
  }
  return isConnected;
}

void SendScriptToDevice(char const* scriptText) {
  WriteStr(&_msComm, scriptText);
}

void PrintSubpackage(const MscrSubPackage subpackage) {

  switch (subpackage.variable_type) {

    case MSCR_VT_POTENTIAL:
    case MSCR_VT_POTENTIAL_CE:
    case MSCR_VT_POTENTIAL_SE:
    case MSCR_VT_POTENTIAL_RE:
    case MSCR_VT_POTENTIAL_GENERIC1:
    case MSCR_VT_POTENTIAL_GENERIC2:
    case MSCR_VT_POTENTIAL_GENERIC3:
    case MSCR_VT_POTENTIAL_GENERIC4:
    case MSCR_VT_POTENTIAL_WE_VS_CE:
      Serial.print("\tE [V]: ");
      break;

    case MSCR_VT_CURRENT:
    case MSCR_VT_CURRENT_GENERIC1:
    case MSCR_VT_CURRENT_GENERIC2:
    case MSCR_VT_CURRENT_GENERIC3:
    case MSCR_VT_CURRENT_GENERIC4:
      Serial.print("\tI [A]: ");
      break;

    case MSCR_VT_ZREAL:
      Serial.print("\tZreal[ohm]: ");
      break;

    case MSCR_VT_ZIMAG:
      Serial.print("\tZimag[ohm]: ");
      break;

    case MSCR_VT_CELL_SET_POTENTIAL:
      Serial.print("\tE set[V]: ");
      break;

    case MSCR_VT_CELL_SET_CURRENT:
      Serial.print("\tI set[A]: ");
      break;

    case MSCR_VT_CELL_SET_FREQUENCY:
      Serial.print("\tF set[Hz]: ");
      break;

    case MSCR_VT_CELL_SET_AMPLITUDE:
      Serial.print("\tA set[V]: ");
      break;

    case MSCR_VT_UNKNOWN:
    default:
      Serial.print("\tUnknown[?]: ");
      break;
  }

  Serial.print(sci(subpackage.value, 3));

  if (subpackage.metadata.status >= 0) {
    char const* status_str;
    if (subpackage.metadata.status == 0) {
      status_str = StatusToString((Status)0);
    } else {

      for (int i = 0; i < 31; i++) {
        if ((subpackage.metadata.status & (1 << i)) != 0) {
          status_str = StatusToString((Status)(1 << i));
          break;
        }
      }
    }
    char formatted_srt[64];
    snprintf(formatted_srt, 64, "\tstatus: %-20s", status_str);
    Serial.print(formatted_srt);
  }
  if (subpackage.metadata.range >= 0) {
    char const* range_str = range_to_string(subpackage.metadata.range, (VarType)subpackage.variable_type);
    Serial.print("\tRange: ");
    Serial.print(range_str);
  }
}


void setup() {
  Serial.begin(230400);
  SerialMSDev.begin(MSCRIPT_DEV_BAUDRATE);
  while (!SerialMSDev) {
  }

  Bluefruit.begin();
  Bluefruit.setTxPower(4);
  // Change the name of the device here
  Bluefruit.setName("EMStat Pico Bluetooth");

  startAdvertising();

  tempService.begin();

  tempCharacteristic.setProperties(CHR_PROPS_NOTIFY);
  tempCharacteristic.setPermission(SECMODE_OPEN, SECMODE_NO_ACCESS);
  tempCharacteristic.setFixedLen(4);
  tempCharacteristic.begin();

  controlCharacteristic.setProperties(CHR_PROPS_WRITE | CHR_PROPS_READ);
  controlCharacteristic.setPermission(SECMODE_OPEN, SECMODE_OPEN);
  controlCharacteristic.setFixedLen(1);
  controlCharacteristic.begin();
  controlCharacteristic.setWriteCallback(controlWriteCallback);
  Serial.println("BLE Temperature Service is running...");

  if (s_printReceived == true) {
    Serial.println("s_printReceived");
  }
  RetCode code = MSCommInit(&_msComm, DEVICE_TYPE, &write_wrapper, &read_wrapper);
}

int package_nr = 0;

void loop() {
  if (startMeasurement) {
    SendScriptToDevice(OCP_ON);
    delay(50);
    MscrPackage package;
    char current[20];
    char readingStatus[16];
    while (SerialMSDev.available()) {
      RetCode code = ReceivePackage(&_msComm, &package);
      switch (code) {
        case CODE_RESPONSE_BEGIN:  // Measurement response begins
          Serial.println();
          Serial.print("Response begin");
          Serial.println();
          break;
        case CODE_MEASURING:
          Serial.println();
          Serial.print("Measuring...");
          Serial.println();
          package_nr = 0;
          break;
        case CODE_OK:
          if (package_nr++ == 0) {
            Serial.println();
            Serial.println("Receiving measurement response:");
          }
          Serial.print(package_nr);
          for (int i = 0; i < package.nr_of_subpackages; i++) {
            PrintSubpackage(package.subpackages[i]);
          }
          Serial.println();
          break;
        case CODE_MEASUREMENT_DONE:
          Serial.println();
          Serial.println("Measurement completed.");
          break;
        case CODE_RESPONSE_END:
          Serial.print(package_nr);
          Serial.println(" data point(s) received.");
          break;
        default:
          Serial.println();
          Serial.print("Failed to parse package: ");
          Serial.println(code);
      }
    }
    //measurement in milivolts, the result of the measurement is notified in the temp characteristic
    for (int i = 0; i < package.nr_of_subpackages; i++) {
      float floatVolt = (package.subpackages[i].value) * 10000;
      int intVolt = floatVolt;
      tempCharacteristic.notify32((intVolt));
      Serial.println(intVolt);
      delay(50);
    }
    delay(2000);
  }
}

void startAdvertising() {
  Bluefruit.Advertising.addFlags(BLE_GAP_ADV_FLAGS_LE_ONLY_GENERAL_DISC_MODE);
  Bluefruit.Advertising.addTxPower();
  Bluefruit.Advertising.addService(tempService);
  Bluefruit.ScanResponse.addName();
  Bluefruit.Advertising.start();
}


void controlWriteCallback(uint16_t conn_handle, BLECharacteristic* chr, uint8_t* data, uint16_t len) {
  if (len == 1) {
    startMeasurement = data[0];
    // Send 0x00 to stop measuring, send 0x01 to start measuring
    Serial.print("Measurement command received: ");
    Serial.println(startMeasurement ? "Start" : "Stop");
  }
}
