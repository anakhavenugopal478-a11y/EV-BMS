# EV-BMS: ESP32 Battery Management System

An ESP32 based battery management system for EV applications. It monitors cell voltages, detects battery faults, controls a protection relay, and sends live telemetry to a Blynk dashboard. The whole thing is simulated and tested in Wokwi.

## What it does
- Reads battery voltage through the ESP32 ADC
- Detects fault conditions and moves between system states
- Switches a relay to protect the battery when something goes wrong
- Shows status on an I2C LCD, LEDs and a buzzer
- Publishes live data and analytics to a Blynk dashboard

## Hardware (simulated in Wokwi)
| Part | Purpose |
|---|---|
| ESP32 | Main controller |
| Potentiometer | Simulates battery voltage as ADC input |
| Relay | Battery protection |
| Buzzer | Fault indication |
| LEDs | Status indication |
| I2C LCD | Displays system information |

## System states
| State | Meaning |
|---|---|
| NORMAL | Battery operating normally |
| DEGRADED | Warning or abnormal condition detected |
| FAILSAFE | Serious fault detected, protection activated |
| SHUTDOWN | Critical condition, system shuts down |

## Dashboard
The Blynk dashboard shows cell voltages, weakest and strongest cell, voltage imbalance, battery SoC, relay status, fault state, fault count, risk score, uptime, system state and fault history.

## Files
- `EV_BMS_FINAL.ino`: firmware source code
- `diagram.json`: Wokwi circuit
- `wokwi.toml`: Wokwi project configuration

## How to run
1. Open the project in VS Code with the Wokwi extension, or upload the files to wokwi.com.
2. Start the simulation.
3. Turn the potentiometer to change the battery voltage and watch the state change.

## Author
Anakha Venugopal
