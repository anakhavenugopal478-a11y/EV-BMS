#define BLYNK_TEMPLATE_ID "TMPL3IxGu6WPU"
#define BLYNK_TEMPLATE_NAME "EV BMS Telemetry"
#define BLYNK_AUTH_TOKEN "u5tR21eI44Ht5G9NFm28C9OKQ8GaS1Jm"

#include <WiFi.h>
#include <BlynkSimpleEsp32.h>
char wifiSSID[] = "Wokwi-GUEST";
char wifiPassword[] = "";

unsigned long wifiPreviousTime = 0;
const unsigned long wifiRetryInterval = 5000;

bool wifiConnecting = false;
#define QUEUE_SIZE 10
struct TelemetryEvent;

#include <Wire.h>
#include <LiquidCrystal_I2C.h>
#define POT_PIN 34
#define RELAY 19

LiquidCrystal_I2C lcd(0x27, 16, 2);
unsigned long lcdPreviousTime = 0;
const unsigned long lcdRefreshInterval = 500;
int lastDisplayedPage = -1;
float lastDisplayedVoltage = -1.0;
String lastDisplayedStatus = "";
int lastDisplayedRaw = -1;
bool lastDisplayedFault = false;
bool lastDisplayedRelay = false;
bool lastDisplayedCriticalFault = false;
int currentPage = 0;
unsigned long pagePreviousTime = 0;
const unsigned long pageInterval = 3000;
float voltage;
String status;
int statusCode;

enum SystemState
{
  NORMAL,
  DEGRADED,
  FAILSAFE,
  SHUTDOWN
};
SystemState currentState = NORMAL;
SystemState previousState = NORMAL;
enum FaultID
{
  NO_FAULT,
  BATTERY_FAULT,
  ADC_FAULT,
  RELAY_FAULT,
  COMMUNICATION_FAULT
};
String getFaultName(FaultID fault)
{
  if(fault == BATTERY_FAULT)
    return "BATTERY_FAULT";
  else if(fault == ADC_FAULT)
    return "ADC_FAULT";
  else if(fault == RELAY_FAULT)
    return "RELAY_FAULT";
  else if(fault == COMMUNICATION_FAULT)
    return "COMMUNICATION_FAULT";
  else
    return "NO_FAULT";
}
FaultID currentFault = NO_FAULT;
FaultID retainedFault = NO_FAULT;
bool relayMismatchFault = false;
extern bool highVoltageFaultConfirmed;
extern bool frozenReadingFault;
extern bool RangeFault;
extern bool voltageJumpFault;
extern bool recoveryInProgress;
extern bool recoveryVerified;
SystemState getNextState(SystemState state)
{
  if(state == NORMAL)
  {
    if(highVoltageFaultConfirmed)
    {
      retainedFault = BATTERY_FAULT;
      return FAILSAFE;
    }
    if(relayMismatchFault)
    {
      retainedFault = RELAY_FAULT;
      return DEGRADED;
    }
    if(frozenReadingFault || RangeFault ||
     voltageJumpFault)
    {
      retainedFault = ADC_FAULT;
      return DEGRADED;
    }
  }
  else if(state == DEGRADED)
  {
    if(highVoltageFaultConfirmed)
    {
      retainedFault = BATTERY_FAULT;
      return FAILSAFE;
    }
    if(!relayMismatchFault && !frozenReadingFault &&
       !RangeFault && !voltageJumpFault)
      return NORMAL;
  }
  else if(state == FAILSAFE)
  {
    if(highVoltageFaultConfirmed)
      return SHUTDOWN;

    if(!highVoltageFaultConfirmed && !recoveryInProgress && recoveryVerified)
      return NORMAL;
  }
  return state;
}
void checkRelayMismatch()
{
  int relayFeedback = digitalRead(18);

  if (digitalRead(RELAY) == HIGH && relayFeedback != LOW)
  {
    relayMismatchFault = true;
    currentFault = RELAY_FAULT;
    retainedFault = RELAY_FAULT;
  }
  else if (digitalRead(RELAY) == LOW && relayFeedback != HIGH)
  {
    relayMismatchFault = true;
    currentFault = RELAY_FAULT;
    retainedFault = RELAY_FAULT;
  }
  else
  {
    relayMismatchFault = false;
  }
}
String getSystemStateName()
{
  if(currentState == NORMAL)
    return "NORMAL";
  else if(currentState == DEGRADED)
    return "DEGRADED";
  else if(currentState == FAILSAFE)
    return "FAILSAFE";
  else
    return "SHUTDOWN";
}
unsigned int stateTransitionCount = 0;
String lastStateTransition = "No transition";
String getBatteryHealth() 
{
  if (currentState == NORMAL) {
    return "HEALTHY";
  }
  else if (currentState == DEGRADED) {
    return "ATTENTION";
  }
  else if (currentState == FAILSAFE) {
    return "CRITICAL";
  }
  else {
    return "SHUTDOWN";
  }
}
void logStateTransition(unsigned long timestamp)
{
  if(previousState != currentState)
  {
    stateTransitionCount++;
    String previousStateName;

    if(previousState == NORMAL)
    previousStateName = "NORMAL";
    else if(previousState == DEGRADED)
    previousStateName = "DEGRADED";
    else if(previousState == FAILSAFE)
    previousStateName = "FAILSAFE";
    else
    previousStateName = "SHUTDOWN";

    String currentStateName;

    if(currentState == NORMAL)
      currentStateName = "NORMAL";
    else if(currentState == DEGRADED)
      currentStateName = "DEGRADED";
    else if(currentState == FAILSAFE)
      currentStateName = "FAILSAFE";
    else
      currentStateName = "SHUTDOWN";

    lastStateTransition = previousStateName + " -> " + currentStateName;
    Blynk.virtualWrite(V19, lastStateTransition);
        if(currentState == NORMAL)
    {
      Blynk.setProperty(V17, "color", "#23C48E");
    }
    else if(currentState == DEGRADED)
    {
      Blynk.setProperty(V17, "color", "#FFC107");
    }
    else if(currentState == FAILSAFE)
    {
      Blynk.setProperty(V17, "color", "#D3435C");
    }
    else
    {
      Blynk.setProperty(V17, "color", "#000000");
    }
    Serial.print("[");
    Serial.print(timestamp);
    Serial.print(" ms] STATE TRANSITION | Previous: ");

    if(previousState == NORMAL)
      Serial.print("NORMAL");
    else if(previousState == DEGRADED)
      Serial.print("DEGRADED");
    else if(previousState == FAILSAFE)
      Serial.print("FAILSAFE");
    else
      Serial.print("SHUTDOWN");

    Serial.print(" | New: ");

    Serial.print(getSystemStateName());

    Serial.print(" | Fault ID: ");

    if(retainedFault == NO_FAULT)
      Serial.println("NO_FAULT");
    else if(retainedFault == BATTERY_FAULT)
      Serial.println("BATTERY_FAULT");
    else if(retainedFault == ADC_FAULT)
      Serial.println("ADC_FAULT");
    else if(retainedFault == RELAY_FAULT)
      Serial.println("RELAY_FAULT");
    else
      Serial.println("COMMUNICATION_FAULT");
  }
}
bool highVoltageFault = false;
bool highVoltageFaultConfirmed = false;

unsigned long previousTime = 0;
const unsigned long interval = 500;

const float highVoltage = 2.60;
const float highVoltageRecovery = 2.40;

unsigned long faultStartTime = 0;
const unsigned long faultDelay = 1000;

float previousVoltage = 0.0;
int previousRaw = -1;
unsigned long frozenStartTime = 0;
const unsigned long frozenDelay = 3000;
bool frozenReadingFault = false;

float previousVoltageJump = 0.0;
bool voltageJumpFault = false;
const float maxVoltageJump = 0.50;

bool RangeFault = false;
const float minValidVoltage = 0.10;
const float maxValidVoltage = 3.20;

bool recoveryInProgress = false;
bool recoveryVerified = false;
unsigned long systemStartTime = 0;
unsigned int faultCount = 0;
bool previousFaultActive = false;

String lastFaultHistory = "No faults";
unsigned long recoveryStartTime = 0;
const unsigned long recoveryDelay = 3000;

const int WINDOW_SIZE = 5;
float voltageWindow[WINDOW_SIZE];
int windowIndex = 0;
bool windowFull = false;

const float noiseThreshold = 0.05;
bool lastDisplayedRecovery = false;

struct TelemetryEvent
{
  unsigned long timestamp;

  float cell1Voltage;
  float cell2Voltage;
  float cell3Voltage;
  float cell4Voltage;

  int weakestCell;
  float weakestVoltage;

  int strongestCell;
  float strongestVoltage;

  float voltageImbalance;

  int relayStatus;
  int faultState;
  int systemState;

  int wifiRSSI;
};

TelemetryEvent offlineQueue[QUEUE_SIZE];

int queueHead = 0;
int queueTail = 0;
int queueCount = 0;

bool enqueueTelemetry(const TelemetryEvent &event)
{
  if(queueCount >= QUEUE_SIZE)
  {
    Serial.println("Offline Queue Full");
    return false;
  }

  offlineQueue[queueTail] = event;
  queueTail++;

  if(queueTail >= QUEUE_SIZE)
  {
    queueTail = 0;
  }

  queueCount++;
  return true;
}

bool dequeueTelemetry(TelemetryEvent &event)
{
  if(queueCount == 0)
  {
    return false;
  }

  event = offlineQueue[queueHead];
  queueHead++;

  if(queueHead >= QUEUE_SIZE)
  {
    queueHead = 0;
  }

  queueCount--;
  return true;
}

TelemetryEvent createTelemetryEvent()
{
  TelemetryEvent event;

  event.timestamp = millis();

  // Use the live simulated voltage to represent four cells.
  event.cell1Voltage = voltage - 0.15;
  event.cell2Voltage = voltage - 0.05;
  event.cell3Voltage = voltage + 0.05;
  event.cell4Voltage = voltage + 0.15;

  // Keep cell values within a safe battery-cell range.
  event.cell1Voltage = constrain(event.cell1Voltage, 0.0, 4.2);
  event.cell2Voltage = constrain(event.cell2Voltage, 0.0, 4.2);
  event.cell3Voltage = constrain(event.cell3Voltage, 0.0, 4.2);
  event.cell4Voltage = constrain(event.cell4Voltage, 0.0, 4.2);

  float cells[4] = {
    event.cell1Voltage,
    event.cell2Voltage,
    event.cell3Voltage,
    event.cell4Voltage
  };

  event.weakestCell = 1;
  event.strongestCell = 1;

  for(int i = 1; i < 4; i++)
  {
    if(cells[i] < cells[event.weakestCell - 1])
      event.weakestCell = i + 1;

    if(cells[i] > cells[event.strongestCell - 1])
      event.strongestCell = i + 1;
  }

  event.weakestVoltage = cells[event.weakestCell - 1];
  event.strongestVoltage = cells[event.strongestCell - 1];

  event.voltageImbalance =
    event.strongestVoltage - event.weakestVoltage;

  event.relayStatus = digitalRead(RELAY);
  event.faultState = (currentFault != NO_FAULT);
  event.systemState = (int)currentState;

  if(WiFi.status() == WL_CONNECTED)
    event.wifiRSSI = WiFi.RSSI();
  else
    event.wifiRSSI = -100;

  return event;
}

TelemetryEvent lastSentEvent;
bool telemetryInitialized = false;

bool telemetryChanged(const TelemetryEvent &event)
{
  if(!telemetryInitialized)
    return true;

  if(abs(event.cell1Voltage - lastSentEvent.cell1Voltage) >= 0.05)
    return true;

  if(abs(event.cell2Voltage - lastSentEvent.cell2Voltage) >= 0.05)
    return true;

  if(abs(event.cell3Voltage - lastSentEvent.cell3Voltage) >= 0.05)
    return true;

  if(abs(event.cell4Voltage - lastSentEvent.cell4Voltage) >= 0.05)
    return true;

  if(event.weakestCell != lastSentEvent.weakestCell)
    return true;

  if(event.strongestCell != lastSentEvent.strongestCell)
    return true;

  if(abs(event.voltageImbalance - lastSentEvent.voltageImbalance) >= 0.05)
    return true;

  if(event.relayStatus != lastSentEvent.relayStatus)
    return true;

  if(event.faultState != lastSentEvent.faultState)
    return true;

  if(event.systemState != lastSentEvent.systemState)
    return true;

  if(abs(event.wifiRSSI - lastSentEvent.wifiRSSI) >= 10)
    return true;

  return false;
}
// Task 6 - Composite Risk Score
  float calculateRiskScore(float imbalance, unsigned int faults, float soc) {
  float imbalanceRisk = constrain((imbalance / 0.20) * 40.0, 0.0, 40.0);

  float faultRisk = constrain(faults * 10.0, 0.0, 40.0);

  float socRisk = 0.0;

  if (soc < 50.0)
  {
    socRisk = ((50.0 - soc) / 50.0) * 20.0;
  }

  float totalRisk = imbalanceRisk + faultRisk + socRisk;

  return constrain(totalRisk, 0.0, 100.0);
}

void sendTelemetryEvent(const TelemetryEvent &event, bool queuedData)
{
  Blynk.virtualWrite(V0, event.cell1Voltage);
  Blynk.virtualWrite(V1, event.cell2Voltage);
  Blynk.virtualWrite(V2, event.cell3Voltage);
  Blynk.virtualWrite(V3, event.cell4Voltage);

  Blynk.virtualWrite(V4, event.weakestCell);
  Blynk.virtualWrite(V5, event.weakestVoltage);
  Blynk.virtualWrite(V6, event.strongestCell);
  Blynk.virtualWrite(V7, event.strongestVoltage);

  Blynk.virtualWrite(V8, event.relayStatus);
  Blynk.virtualWrite(V9, event.faultState);
  Blynk.virtualWrite(V10, event.wifiRSSI);
  Blynk.virtualWrite(V11, queueCount);

  String eventStateName;

  if(event.systemState == NORMAL)
    eventStateName = "NORMAL";
  else if(event.systemState == DEGRADED)
    eventStateName = "DEGRADED";
  else if(event.systemState == FAILSAFE)
    eventStateName = "FAILSAFE";
  else
    eventStateName = "SHUTDOWN";

  Blynk.virtualWrite(V12, eventStateName);

  if(queuedData)
    Blynk.virtualWrite(V13, "QUEUED");
  else
    Blynk.virtualWrite(V13, "LIVE");

  Blynk.virtualWrite(V14, 80.0);
  Blynk.virtualWrite(V15, event.voltageImbalance);
  float riskScore = calculateRiskScore(event.voltageImbalance,faultCount,80.0);

  Blynk.virtualWrite(V16, riskScore);

  Serial.print("Risk Score: ");
  Serial.println(riskScore);

  String batteryHealth = getBatteryHealth();

  Blynk.virtualWrite(V17, batteryHealth);

  Serial.print("Battery Health: ");
  Serial.println(batteryHealth);

  float maintenanceRisk = calculateRiskScore(event.voltageImbalance, faultCount, 80.0);
  String recommendation = getMaintenanceRecommendation(maintenanceRisk);

  Blynk.virtualWrite(V18, recommendation);

  unsigned long uptime = (millis() - systemStartTime) / 1000;

  Blynk.virtualWrite(V21, uptime);
  Blynk.virtualWrite(V22, faultCount);
  
  Serial.print("Maintenance: ");
  Serial.println(recommendation);

  if(queuedData)
    Serial.println("Queued Telemetry Sent");
  else
    Serial.println("Live Telemetry Sent");
}
String getMaintenanceRecommendation(float riskScore)
{
  if (currentState == SHUTDOWN) {
    return "Immediate service required";
  }
  else if (currentState == FAILSAFE) {
    return "Inspect battery and protection";
  }
  else if (currentState == DEGRADED) {
    return "Check battery condition";
  }
  else if (riskScore >= 60.0) {
    return "Monitor battery closely";
  }
  else if (riskScore >= 30.0) {
    return "Schedule battery inspection";
  }
  else {
    return "No maintenance required";
  }
}

void processTelemetry()
{
  bool connected = (WiFi.status() == WL_CONNECTED && Blynk.connected());

  if(connected)
  {
    if(queueCount > 0)
    {
      TelemetryEvent queuedEvent;

      if(dequeueTelemetry(queuedEvent))
      {
        sendTelemetryEvent(queuedEvent, true);
      }

      return;
    }

    TelemetryEvent currentEvent = createTelemetryEvent();

    if(telemetryChanged(currentEvent))
    {
      sendTelemetryEvent(currentEvent, false);

      lastSentEvent = currentEvent;
      telemetryInitialized = true;
    }
  }
  else
  {
    TelemetryEvent currentEvent = createTelemetryEvent();

    if(telemetryChanged(currentEvent))
    {
      if(enqueueTelemetry(currentEvent))
      {
        Serial.println("Telemetry Stored In Offline Queue");

        lastSentEvent = currentEvent;
        telemetryInitialized = true;
      }
    }
  }
}

void setup() 
{
  pinMode(RELAY, OUTPUT);
  Serial.begin(115200);
  systemStartTime = millis();
  WiFi.mode(WIFI_STA);
  WiFi.begin(wifiSSID, wifiPassword);
  Blynk.setProperty(V17, "color", "#23C48E");
  Blynk.config(BLYNK_AUTH_TOKEN);
  wifiConnecting = true;
  lcd.init();
  lcd.backlight();
  lcd.setCursor(0, 0);
  lcd.print("BMS System");
  digitalWrite(RELAY, LOW);
  pinMode(18, INPUT_PULLUP);
}

void loop() 
{
  unsigned long currentTime = millis();
  if(WiFi.status() == WL_CONNECTED)
  {
    wifiConnecting = false;
    Blynk.run();
  }
  else
  {
    wifiConnecting = true;

    if(currentTime - wifiPreviousTime >= wifiRetryInterval)
    {
      wifiPreviousTime = currentTime;
      WiFi.disconnect();
      WiFi.begin(wifiSSID, wifiPassword);
    }
  }
  if(currentTime - previousTime >= interval)
  {
    previousTime = currentTime;
  } 
  int raw = analogRead(POT_PIN);
  voltage = (raw / 4095.0)*3.3;

  voltageWindow[windowIndex] = voltage;
  windowIndex++;
  if(windowIndex >= WINDOW_SIZE)
  {
    windowIndex = 0;
    windowFull = true;
  }
  if(windowFull)
  {
    float minimumVoltage = voltageWindow[0];
    float maximumVoltage = voltageWindow[0];
    float totalVoltage = voltageWindow[0];

    for(int i = 1; i < WINDOW_SIZE; i++)
    {
      if(voltageWindow[i] < minimumVoltage)
      {
        minimumVoltage = voltageWindow[i];
      }

      if(voltageWindow[i] > maximumVoltage)
      {
        maximumVoltage = voltageWindow[i];
      }

      totalVoltage = totalVoltage + voltageWindow[i];
    }

    float voltageRange = maximumVoltage - minimumVoltage;
    float averageVoltage = totalVoltage / WINDOW_SIZE;

    float voltageDifference = abs(voltage - averageVoltage);

    if(voltageRange <= noiseThreshold)
    {
      Serial.println("Sensor Noise Detected");
    }
    else if(voltageDifference > noiseThreshold)
    {
      Serial.println("Rapid Voltage Change Detected");
    }
  }
  if(voltage < minValidVoltage || voltage > maxValidVoltage)
  {
    if(!RangeFault)
    {
      RangeFault = true;
      currentFault = ADC_FAULT;
      retainedFault = ADC_FAULT;
      Serial.println("Out Of Range Measurement Detected");
    }
  }
  else
  {
    RangeFault = false;
  }
  if(previousVoltageJump != 0.0)
  {
    float voltageDifference = abs(voltage - previousVoltageJump);
    
      if(voltageDifference >= maxVoltageJump)
      {
        if(!voltageJumpFault)
        {
          voltageJumpFault = true;
          currentFault = ADC_FAULT;
          retainedFault = ADC_FAULT;
          Serial.println("Unrealistic Voltage Jump Detected");
        }
      }
      else
      {
        voltageJumpFault = false;
      }
  }
  previousVoltageJump = voltage;

  if(raw == previousRaw)
  {
    if(frozenStartTime == 0)
    {
      frozenStartTime = currentTime;
    }
 
    if(!frozenReadingFault && (currentTime - frozenStartTime >= frozenDelay))
     
    {
      frozenReadingFault = true;
      currentFault = ADC_FAULT;
      retainedFault = ADC_FAULT;
      Serial.println("Sensor Frozen Reading Fault Detected");
    }
  }
  else
  {
    previousRaw = raw;
    frozenStartTime = 0;
    frozenReadingFault = false;
  }

  if(voltage >= highVoltage)
  {
    if(highVoltageFault == false)
    {
      highVoltageFault = true;
      faultStartTime = currentTime;
      highVoltageFaultConfirmed = false;
      currentFault = BATTERY_FAULT;
      retainedFault = BATTERY_FAULT;
      Serial.println("High Voltage Fault Detected");
    }
    if(!highVoltageFaultConfirmed && (currentTime - faultStartTime >= faultDelay))
    {
      highVoltageFaultConfirmed = true;
      Serial.println("High Voltage Fault Confirmed");
    }
  }
  else if(voltage <= highVoltageRecovery)
  {
    if(highVoltageFault)
    {
      Serial.println("High Voltage Fault Cleared");
      recoveryInProgress = true;
      recoveryVerified = false;
      recoveryStartTime = currentTime;
      Serial.println("Recovery Started");
      highVoltageFault = false;
    }
    faultStartTime = 0;
    highVoltageFaultConfirmed = false;
  }

  if(recoveryInProgress)
  {
   if(currentTime - recoveryStartTime >= recoveryDelay)
   {
     recoveryInProgress = false;
     recoveryVerified = true;
     Serial.println("Recovery Completed");
   }
  }
  previousState = currentState;
  checkRelayMismatch();
  currentState = getNextState(currentState);

  bool currentFaultActive =
  highVoltageFaultConfirmed ||
  frozenReadingFault ||
  RangeFault ||
  voltageJumpFault ||
  relayMismatchFault;

if(currentFaultActive && !previousFaultActive)
{
  faultCount++;

  lastFaultHistory = getFaultName(retainedFault);

  Blynk.virtualWrite(V20, lastFaultHistory);

  Serial.print("Task 6 Fault Count: ");
  Serial.println(faultCount);

  Serial.print("Fault History: ");
  Serial.println(lastFaultHistory);
}

previousFaultActive = currentFaultActive;

  logStateTransition(currentTime);

  Serial.print("DEBUG Frozen Fault: ");
  Serial.println(frozenReadingFault);

  Serial.print("DEBUG System State: ");
  Serial.println(getSystemStateName());
   
  if(voltage < 1.5)
  {
    status = ("Low Battery");
    statusCode = 0;
  }
  else if(voltage <= 2.5)
  {
    status = ("Normal");
    statusCode = 1;
  }
  else
  {
    status = ("High Battery");
    statusCode = 2;
  }

  if(recoveryInProgress)
  {
    digitalWrite(RELAY, HIGH);
  }
  else if(statusCode == 0)
  {
    digitalWrite(RELAY, HIGH);
  }
  else if(statusCode == 1)
  {
    digitalWrite(RELAY, LOW);
  }
  else
  {
    digitalWrite(RELAY, HIGH);
  }
  Serial.print("Raw ADC value: ");
  Serial.println(raw);
  Serial.print("Voltage: ");
  Serial.print(voltage, 2);
  Serial.println(" V");
  Serial.print("Battery State: ");
  Serial.println(status);
  Serial.print("StatusCode: ");
  Serial.println(statusCode);
  Serial.print("High Voltage Fault: ");
  Serial.println(highVoltageFault);
  Serial.println();

if(highVoltageFault)
{
  if(!lastDisplayedCriticalFault)
  {
    lcd.setCursor(0, 0);
    lcd.print("HIGH VOLTAGE    ");

    lcd.setCursor(0, 1);
    lcd.print("FAULT ACTIVE    ");

    lastDisplayedCriticalFault = true;
    lastDisplayedPage = -1;
  }
}
else
{
  if(lastDisplayedCriticalFault)
  {
    lastDisplayedCriticalFault = false;
    lastDisplayedPage = -1;
  }

  if(currentTime - pagePreviousTime >= pageInterval)
  {
    pagePreviousTime = currentTime;

    currentPage++;

    if(currentPage >= 3)
    {
      currentPage = 0;
    }
  }

  if(currentTime - lcdPreviousTime >= lcdRefreshInterval)
  {
    lcdPreviousTime = currentTime;

    bool relayOn;

    if(recoveryInProgress || statusCode == 0 || statusCode == 2)
    {
      relayOn = true;
    }
    else
    {
      relayOn = false;
    }
    if(currentPage == 0)
    {
      if(currentPage != lastDisplayedPage)
      {
        lcd.setCursor(0, 0);
        lcd.print("Volt: ");
        lcd.print(voltage, 2);
        lcd.print(" V     ");

        lcd.setCursor(0, 1);
        lcd.print(status);
        lcd.print("             ");

        lastDisplayedPage = 0;
        lastDisplayedRaw = raw;
        lastDisplayedStatus = status;
        lastDisplayedVoltage = voltage;
      }
      else if(raw != lastDisplayedRaw || status != lastDisplayedStatus)
      {
        lcd.setCursor(0, 0);
        lcd.print("Volt: ");
        lcd.print(voltage, 2);
        lcd.print(" V     ");

        lcd.setCursor(0, 1);
        lcd.print(status);
        lcd.print("             ");

        lastDisplayedRaw = raw;
        lastDisplayedStatus = status;
        lastDisplayedVoltage = voltage;
      }
    }
    else if(currentPage == 1)
    {
      if(currentPage != lastDisplayedPage ||
         lastDisplayedRelay != relayOn ||
         lastDisplayedStatus != status ||
         recoveryInProgress != lastDisplayedRecovery)
      {
        lcd.setCursor(0, 0);

        if(recoveryInProgress)
        {
          lcd.print("System: FAULT   ");
        }
        else
        {
          lcd.print("System: ");
          lcd.print(getSystemStateName());
        }

        lcd.setCursor(0, 1);

        if(relayOn)
        {
          lcd.print("Relay: ON       ");
        }
        else
        {
          lcd.print("Relay: OFF      ");
        }

        lastDisplayedPage = 1;
        lastDisplayedRelay = relayOn;
        lastDisplayedStatus = status;
        lastDisplayedRecovery = recoveryInProgress;
      }
    }
    else
    {
      if(currentPage != lastDisplayedPage ||
         raw != lastDisplayedRaw ||
         highVoltageFault != lastDisplayedFault)
      {
        lcd.setCursor(0, 0);
        lcd.print("ADC: ");
        lcd.print(raw);
        lcd.print("           ");

        lcd.setCursor(0, 1);
        lcd.print("Fault: ");
        lcd.print(highVoltageFault);
        lcd.print("          ");

        lastDisplayedPage = 2;
        lastDisplayedRaw = raw;
        lastDisplayedFault = highVoltageFault;
      }
    }
  }
} 
  processTelemetry();
}