# Smart-Medi-Box-IoT-Project
The Smart Medi Box is a device that alerts users when to take medicine. Initialy, the user should enter  their time zone and then they can set an alarm to remind them when to take medicine. The Medi Box can  be protected from excessive light using LDRs. 

Smart medibox is a smart device that,
1. Remind the user to take medicine at time through alarms.
2. Help to keep medicine in the required conditions via monitoring temperature and humidity continously and notifying the user if there is a bad condition.
3. Controlling the light entering the Medibox through a motorized curtain.

This project was done in two phases. In the first face, a MediBox which can remind the user to take medicine at time through alarms and monitor the humidity and temprature in the MediBox and alert the user if the condition with visual and audio aids, is out of the deisred range was coded and simulated using Wokwi. The code for this can be found above as first_code.ino. The Wokwi link : https://wokwi.com/projects/391985014915776513.

![Screenshot 2024-05-21 200821](https://github.com/javin-5/Smart-Medi-Box-IoT-Project/assets/121782593/e633c64c-2151-43df-a124-d6694a475d53)

In the second phase, some additional functions were added,
1. Controlling the light entering the Medibox through a motorized curtain.
2. Using Node-Red create a deashboard for real-time monitoring.

The data between the ESP32 board and the Node-Red are communicated used the MQTT protocol. below is the final circuit on Wokwi. The code can be found in final_code.ino. The Wokwi link : https://wokwi.com/projects/397675674017241089.

![Screenshot 2024-05-21 200716](https://github.com/javin-5/Smart-Medi-Box-IoT-Project/assets/121782593/32eb429c-e0e3-4e58-a4e2-411a25d78ab2)
