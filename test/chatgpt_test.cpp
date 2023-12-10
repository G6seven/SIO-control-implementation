#include <Arduino.h>
#include <Bounce2.h>
#include "TimeRelease.h"

#define VERSIONINFO "SIO_Arduino_General 1.0.8"
#define COMPATIBILITY "SIOPlugin 0.1.1"

#define IOSize  19

// Outputs
#define IO0 3  
#define IO1 4
#define IO2 5
#define IO3 6
#define IO4 7
#define IO5 8
#define IO6 9
#define IO7 10
#define IO8 11
#define IO9 12
#define IO10 13

// Inputs
#define IO11 A0
#define IO12 A1
#define IO13 A2 
#define IO14 A3
#define IO15 A4
#define IO16 A5
#define IO17 A6
#define IO18 A7 

bool _debug = false;

int IOType[IOSize]{OUTPUT, OUTPUT, OUTPUT, OUTPUT, OUTPUT, OUTPUT, OUTPUT, OUTPUT, OUTPUT, OUTPUT, OUTPUT, INPUT, INPUT, INPUT, INPUT, INPUT, INPUT, INPUT, INPUT};
int IOMap[IOSize] {IO0, IO1, IO2, IO3, IO4, IO5, IO6, IO7, IO8, IO9, IO10, IO11, IO12, IO13, IO14, IO15, IO16, IO17, IO18};
String IOSMap[IOSize] {"IO0", "IO1", "IO2", "IO3", "IO4", "IO5", "IO6", "IO7", "IO8", "IO9", "IO10", "IO11", "IO12", "IO13", "IO14", "IO15", "IO16", "IO17", "IO18"};
int IO[IOSize];
Bounce Bnc[IOSize];
bool EventTriggeringEnabled = 1;

//void(* resetFunc) (void) = 0;
 
bool isOutPut(int IOP){
  return IOType[IOP] == OUTPUT; 
}

void ConfigIO() {
  Serial.println("Setting IO");
  for (int i = 0; i < IOSize; i++) {
    if (i > 10) { // if it is an input
      pinMode(IOMap[i], IOType[i]);
      Bnc[i].attach(IOMap[i], IOType[i]);
      Bnc[i].interval(5);
    } else {
      pinMode(IOMap[i], IOType[i]);
      digitalWrite(IOMap[i], LOW);
    }
  }
}

TimeRelease IOReport;
TimeRelease ReadyForCommands;

unsigned long reportInterval = 3000;

void setup() {
  Serial.begin(115200);
  delay(300);
  debugMsg(VERSIONINFO);
  debugMsg("Initializing IO");
  ConfigIO();

  reportIOTypes();
  debugMsg("End Types");

  for (int i = 0; i < IOSize; i++) {
    IO[i] = digitalRead(IOMap[i]);
  }

  IOReport.set(100ul);
  ReadyForCommands.set(reportInterval * 3);
  DisplayIOTypeConstants();
}

bool _pauseReporting = false;
bool ioChanged = false;

void loop() {
  checkSerial();
  if (!_pauseReporting) {
    ioChanged = checkIO();
    reportIO(ioChanged);
    if (ReadyForCommands.check()) {
      Serial.println("RR");
      ReadyForCommands.set(reportInterval * 3);
    }
  }
}

void reportIO(bool forceReport) {
  if (IOReport.check() || forceReport) {
    Serial.print("IO:");
    for (int i = 0; i < IOSize; i++) {
      IO[i] = digitalRead(IOMap[i]);
      Serial.print(IO[i]);
    }
    Serial.println();
    IOReport.set(reportInterval);
  }
}

bool checkIO() {
  bool changed = false;

  for (int i = 0; i < IOSize; i++) {
    if (!isOutPut(i)) {
      Bnc[i].update();

      if (Bnc[i].changed()) {
        changed = true;
        IO[i] = Bnc[i].read();
      }
    } else {
      if (IO[i] != digitalRead(IOMap[i])) {
        changed = true;
      }
    }
  }

  return changed;
}

void reportIOTypes() {
  Serial.print("IT:");
  for (int i = 0; i < IOSize; i++) {
    Serial.print(String(IOType[i]));
    Serial.print(",");
  }
  Serial.println();
  debugMsg("IOSize:" + String(IOSize));
}

void DisplayIOTypeConstants() {
  debugMsgPrefx(); Serial.print("INPUT:"); Serial.println(INPUT); // 1
  debugMsgPrefx(); Serial.print("OUTPUT:"); Serial.println(OUTPUT); // 2
  debugMsgPrefx(); Serial.print("INPUT_PULLUP:"); Serial.println(INPUT_PULLUP); // 5
}

void checkSerial() {
  if (Serial.available()) {
    String buf = Serial.readStringUntil('\n');
    buf.trim();
    if (_debug) { debugMsg("buf:" + buf); }
    int sepPos = buf.indexOf(" ");
    String command = "";
    String value = "";

    if (sepPos != -1) {
      command = buf.substring(0, sepPos);
      value = buf.substring(sepPos + 1);;
      if (_debug) {
        debugMsg("command:[" + command + "]");
        debugMsg("value:[" + value + "]");
      }
    } else {
      command = buf;
      if (_debug) {
        debugMsg("command:[" + command + "]");
      }
    }
    if (command.startsWith("N") || command == "M115") {
      ack();
      Serial.println("Received a command that looks like OctoPrint thinks I am a printer.");
      Serial.println("This is not a printer; it is a SIOController. Sending disconnect host action command.");
      Serial.println("To avoid this, you should place the name/path of this port in the [Blacklisted serial ports] section of Octoprint");
      Serial.println("This can be found in the settings dialog under 'Serial Connection'");
      Serial.println("//action:disconnect");
    } else if (command == "BIO") {
      ack();
      _pauseReporting = false;
      return;
    } else if (command == "EIO") {
      ack();
      _pauseReporting = true;
      return;
    } else if (command == "VC") {
      Serial.print("VI:");
      Serial.println(VERSIONINFO);
      Serial.print("CP:");
      Serial.println(COMPATIBILITY);
    } else if (command == "IC") {
      ack();
      Serial.print("IC:");
      Serial.println(IOSize - 1);
      return;
    } else if (command == "debug" && value.length() == 1) {
      ack();
      if (value == "1") {
        _debug = true;
        debugMsg("Serial debug On");
      } else {
        _debug = false;
        debugMsg("Serial debug Off");
      }
      return;
    } else if (command == "CIO") {
      ack();
      debugMsg("Live IO configuration feature is only partially supported.");
      if (value.length() == 0

) {
        debugMsg("ERROR: command value out of range");
      } else if (validateNewIOConfig(value)) {
        updateIOConfig(value);
      }
      return;
    } else if (command == "SIO") {
      ack();
      debugMsg("Live IO configuration feature is not available in this firmware");
      debugMsg("To change IO configuration, you must edit and update firmware");
      return;
    } else if (command == "IOT") {
      ack();
      reportIOTypes();
      return;
    } else if (command == "IO") {
      ack();
      if (value.length() < 3) {
        debugMsg("ERROR: command value out of range");
        return;
      }

      if (value.indexOf(" ") >= 1 && value.substring(value.indexOf(" ") + 1).length() == 1) {
        String sIOPoint = value.substring(0, value.indexOf(" "));
        String sIOSet = value.substring(value.indexOf(" ") + 1);

        if (_debug) {
          debugMsg("IO#:" + sIOPoint);
          debugMsg("GPIO#:" + String(IOMap[sIOPoint.toInt()]));
          debugMsg("Set to:" + sIOSet);
        }

        if (isOutPut(sIOPoint.toInt()) && !(sIOPoint.toInt() == 0 && sIOPoint != "0")) {
          if (sIOSet == "0" || sIOSet == "1") {
            if (sIOSet == "1") {
              digitalWrite(IOMap[sIOPoint.toInt()], HIGH);
            } else {
              digitalWrite(IOMap[sIOPoint.toInt()], LOW);
            }
          } else {
            debugMsg("ERROR: Attempt to set IO to a value that is not valid");
          }
        } else {
          debugMsg("ERROR: Attempt to set IO which is not an output");
        }
        reportIO(true);
      } else {
        debugMsg("ERROR: IO point or value is invalid");
      }
      return;
    } else if (command == "SI") {
      ack();
      if (value.length() > 2) {
        long newTime = value.toInt();
        if (newTime >= 500 && (String(newTime) == value)) {
          reportInterval = newTime;
          IOReport.clear();
          IOReport.set(reportInterval);
          debugMsg("Auto report timing changed to:" + String(reportInterval));
        } else {
          debugMsg("ERROR: command value out of range: Min(500) Max(2147483647)");
        }
        return;
      }
      debugMsg("ERROR: bad format number out of range.(500- 2147483647); actual sent [" + value + "]; Len[" + String(value.length()) + "]");
      return;
    } else if (command == "SE") {
      ack();
      if (value.length() == 1) {
        if (value == "1" || value == "0") {
          EventTriggeringEnabled = value.toInt();
        } else {
          debugMsg("ERROR: command value out of range");
        }
      } else {
        debugMsg("ERROR: command value not sent or bad format");
      }
      return;
    } else if (command == "restart" || command == "reset" || command == "reboot") {
      ack();
      debugMsg("Command not supported.");
    } else if (command == "GS") {
      ack();
      reportIO(true);
      return;
    } else {
      debugMsg("ERROR: Unrecognized command[" + command + "]");
    }
  }
}

void ack() {
  Serial.println("OK");
}

bool validateNewIOConfig(String ioConfig) {
  if (ioConfig.length() != IOSize) {
    if (_debug) { debugMsg("IOConfig validation failed(Wrong len)"); }
    return false;
  }

  for (int i = 0; i < IOSize; i++) {
    int pointType = ioConfig.substring(i, i + 1).toInt();
    if (pointType > 4) {
      if (_debug) {
        debugMsg("IOConfig validation failed"); debugMsg("Bad IO Point type: index[" + String(i) + "] type[" + pointType + "]");
      }
      return false;
    }
  }
  if (_debug) { debugMsg("IOConfig validation good"); }
  return true;
}

void updateIOConfig(String newIOConfig) {
  for (int i = 2; i < IOSize; i++) {
    int nIOC = newIOConfig.substring(i, i + 1).toInt();
    if (IOType[i] != nIOC) {
      IOType[i] = nIOC;
      if (nIOC == OUTPUT) {
        digitalWrite(IOMap[i], LOW);
      }
    }
  }
}

int getIOType(String typeName) {
  if (typeName == "INPUT") { return 1; }
  if (typeName == "OUTPUT") { return 2; }
  if (typeName == "INPUT_PULLUP") { return 5; }
  Serial.print("ERROR: unrecognized IOType name IOType[" + typeName + "]");
  return 0;
}

String getIOTypeString(int ioType) {
  if (ioType == 1) { return "INPUT"; }
  if (ioType == 2) { return "OUTPUT"; }
  if (ioType == 5) { return "INPUT_PULLUP"; }
  Serial.print("ERROR: unrecognized IOType value["); Serial.print(ioType); Serial.println("]");
  return "";
}

bool strToUnsignedLong(String& data, unsigned long& result) {
  data.trim();
  long tempResult = data.toInt();
  if (String(tempResult) != data) {
    return false;
  } else if (tempResult < 0) {
    return false;
  }
  result = tempResult;
  return true;
}

void debugMsg(String msg) {
  debugMsgPrefx();
  Serial.println(msg);
}

void debugMsgPrefx() {
  Serial.print("DG:");
}