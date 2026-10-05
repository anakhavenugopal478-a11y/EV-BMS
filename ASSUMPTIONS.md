# Assumptions

1. The potentiometer is used to simulate battery cell voltage input in the Wokwi environment.

2. The ESP32 is the main controller for battery monitoring, protection logic, telemetry, and state management.

3. The Blynk dashboard is used for live monitoring and analytics during the simulation.

4. The simulated battery consists of four monitored cell voltages.

5. SoC is currently represented by a fixed test value of 80% in the simulation.

6. The relay is used as the battery protection control output.

7. Wi-Fi connectivity is required for live Blynk telemetry; when connectivity is unavailable, telemetry events are stored in the offline queue.