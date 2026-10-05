# Bug Log

| Issue | Cause | Fix |
|---|---|---|
| Duplicate maintenance calculation | `maintenanceRisk` and `recommendation` were calculated twice | Removed the duplicate calculation block |
| State transition declaration error | `stateTransitionCount` and `lastStateTransition` were declared after they were used | Moved the declarations before the related functions |
| Initial Battery Health color | NORMAL state did not initially display the correct color after startup | Set the initial Battery Health color to green after Blynk connection |
| Wokwi development issue | Wokwi development was not working reliably | Continued development using VS Code and Arduino CLI |