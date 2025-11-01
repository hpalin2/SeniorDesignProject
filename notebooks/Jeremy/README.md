**2025-09-24 - Part Sourcing and PCB Planning**

---

Today we focused on sourcing all of our parts and delegating out work for the PCB design. I helped research our AAFS component and determine the proper resistance needed for our voltage divider. In addition, I also contacted the facility manager at Carle to see if I could get connected with Epic.

**Our Goals:**
Rough PCB design by end of weekend
Schedule with Jack to go over desgin decisions
Create a final part list
Split up PCB design

We worked on creating a part list on google docs:https://docs.google.com/document/d/1e973Mm_vhnLTH5vz_4C17udLJVd2HXl1_8vYD8PnHUg/edit?usp=sharing

**2025-09-25 - PCB Schematic Work**

---

I worked on creating the PCB schematic in KiCad. While importing in the parts, I realized that the power mux we initially wanted to use didn't have a built in PMOS, so I suggested we use the TI TPS2120 instead to accomplish our goal. I imported in a custom layout to be used in KiCad.

**2025-09-26 - PCB Schematic Work cont'd**

---

I finished creating the PCB schematic in KiCad. I worked with Suleymaan to finish wiring up the schematic according to our block diagram


**2025-10/1 - Meeting With Mentor**

---

Today my team and I met with a mentor, Jack Blevins, to go over our PCB design schematic. Overall he was satisfied with our layout, but suggested that we switch over to a charging circuit instead of the power mux we had originally planned to account for random power surges that might blow our system.

I also made some changes to our pcb schematic to account for us using a 3.7V coin cell instead of outright 12V input.


**2025-10/3 - PCB & Breadboard Work**

---

We worked on getting parts for our breadboard demo and also finishing up our PCB schematic for review. We ran into some issues since the sensor we used from the shop was actually an IR sensor, and we couldn't properly test it. Luckily, we were able to find an RCWL-1601 distance sensor, but we also struggled to get it wired up correctly by the end of the day. 

For our PCB, it turns out that we couldn't use the powermux module that we picked out because of it's form factor, so we consulted with TA's to find one that had the correct footprint while also performing the same function


**2025-10/4 - Breadboard Work Cont.**

---

I was finally able to wire up the sensor properly and test it, I wrote some C code to detect whether an object was within 10cm; if so, one of the gpio pins would output a voltage to light up an led. This is intended to show the functionality of our motion detector.

**2025-10/6 - Software Planning**

---

Today I began planning the software architecture for our Suction Sense module. 
First, I broke down our system requirements into high-level architecture:

1. Local Message Bus – The ESP32 requires a reliable mechanism to transmit suction and motion sensor data over Wi-Fi or LAN to the Raspberry Pi. To achieve this, I plan to use an MQTT broker running locally on the Pi, allowing each ESP32 node to publish messages to specific topics.

2. Ingestion Service – This service will run continuously on the Raspberry Pi, subscribing to the MQTT topics and collecting incoming data from all operating rooms. It will validate and store the telemetry in a local database. This will allow us to generate historical reports and also maintain functionality in case one of the services gets interrupted.

3. Rules Engine – Once data is being stored and updated, the next layer will process it to determine the current state of each operating room. The engine will compare suction activity with the schedule data and identify when suction has unnecessary usage.

4. User Interface (UI) – A local web dashboard will be hosted on the Raspberry Pi and displayed on the 7″ touchscreen. It will provide a color-coded status view of each operating room (e.g., green for normal operation, red for unnecessary suction). The interface will refresh automatically using WebSocket connections for real-time updates.

5. Data Storage – A lightweight SQLite/PostgreSQL database will hold suction data, schedule information, and device data. This can be used for historical data reports

**2025-10/8 - Design Document Planning**

---

Met with group to go over new Design Document requirements and effectively split up the work


**2025-10/10 - Design Document Work**

---

Worked on the MCU Subsystem(Requirements + Verification) and Pi + Software Subsystem(Requirements + Verification) sections of our design document. Additionally, we also met and planned out our schedule for the rest of the semester

**2025-10/13 - Design Document Work cont'd**

---

Continued work on Design Document. I created the cost analysis table and additionally added onto the software and MCU subsystems, as well as contributing to the overall formatting of the document. We submitted our document today.


**2025-10/14 - Software Design Stage 1**

---

The goal is to be able to build and run a simple crow web app. I was able to host a local crow server and build/run it properly today. I'm hoping to present it to my teammates tomorrow and get them set up on the codebase.

**2025-10/15 - Software Design Stage 1**

Today, my team and I met to discuss software planning as well as machine shop logistics. Below is information I gathered on setting up the MQTT broker/client for our app to read telemetry from the ESP32

---
1. Set up simple crow app -> make webpage Hugh/Suley
2. Set up a MQTT Server, stream data from motion -> Jeremy
3. Inside Crow app, set up MQTT Client -> subscribed to server on ESP32 -> Jeremy

Mosquitto (broker): runs on Mac/Pi, listens on port, routes messages between clients.

Paho (client library): used inside C++ Crow service to connect to the broker, subscribe/publish.

ESP32 client: Runs arduiono sketch that connects to the broker and publishes telemetry.

I got mosquitto installed. Here are the steps I took to test it:

1. First, install mosquitto using homebrew. brew install mosquitto
2. Run mosquitto. This will create a broker on port 1883
3. Create mosquitto subscriber. mosquitto_sub -h 127.0.0.1 -p 1883 -t 'test/#' -v
4. Create mosquitto publisher. mosquitto_pub -h 127.0.0.1 -p 1883 -t 'test/hello' -m 'hi'

On the subscriber terminal, you should be able to see "test/hello hi" outputted.


**2025-10/17 - Software Design Stage 1 cont'd**

Met with group to discuss software design plan and attempt to set up the MQTT publisher on the ESP32. I was having some issues with brownout, so I planned on going to the lab on Saturday to utilize the DC power supply

**2025-10/17 - Software Design Stage 1 cont'd**

Unfortunately, even with the DC power supply, my esp32 seems to still be browning out. However, I discovered that to actually receive messages I need to manually configure my mosquitto protocol as shown below:

Change Mosquitto config by doing code /opt/homebrew/etc/mosquitto/mosquitto.conf 
and inputting this: 
listener 1883 0.0.0.0
allow_anonymous true

Then run mosquitto -v -c /opt/homebrew/etc/mosquitto/mosquitto.conf

I'm struggling to understand why it's still browning out however, since the DC power supply of 5V should be stable. I'm planning on consulting a TA or someone more experienced for some help

**2025-10/17 - Software Design Stage 1 cont'd**

Hugh did a good job getting the UI mocked up, so today I just figured out how to compile it on my end to get the static website running. I plan to demo this on Tuesday as a part of the capstone meeting with sharon. 

Build command for mac: clang++ -std=c++17 \
  -I"$(brew --prefix crow)/include" \
  -I"$(brew --prefix asio)/include" \
  -pthread -o server main.cpp

**2025-10/20 - ESP Debugging**

  The ESP32 module we bought continuously browns out when we try running basic Wifi and BLE sketches, which doesn't make any sense. I went to the lab with Suleymaan to try and solve the issue by supplying DC power to the board instead of laptop power, but this didn't solve the issue. After an entire day of debugging, I think it might be an issue with the dev board


**2025-10/21 - Suction Sense Check-In**

I attended a meeting with Sharon and Nathan to do a quick check-in with the capstone course director people. I went over the schedule we made for the project, and gave a quick software demo of our static website UI.

**2025-10/22 - Group meeting**

Hugh and I attended OH at 4pm, and we were able to get our hands on a different ESP32 Dev Board. It is able to successfully run both the wifi and ble sketch, and we were able to check it out which solved a huge roadblock

We had our team meeting today at 6pm

Software Recap:

To live update site:
run ./server
open another terminal, run sq3lite suction_sense.db -> this opens the DB (Run direct SQL commands)
		- DB updates, but page needs manual refresh -> Page should automatically live update and refresh
		- DB needs to be modified to track state, currently tracks history and updates with every new telemtry update 


**2025-10/23 - ESP32 MQTT Publishing**

I was able to publish mock telemtry data from the ESP32 to the locally hosted MQTT broker, and also have it read properly by the MQTT Subscriber.

To get this set up:
1. Open up a terminal and run "mosquitto". This starts the MQTT Broker
2. Open up a second terminal. Run "mosquitto_sub -h (Your broker machine's IP) -p 1883 -t 'suction/#' -v" 
    - On the ESP, this function: mqtt.publish("suction/dev1/telemetry", payload.c_str()) published the string telemetry payload to the topic suction/dev1/telemetry, so in the terminal command the 'suction/#' directs the subscription to this topic
3. Flash the "esp_mqtt_test.c" sketch to the esp32
4. You should see an output of suction/dev1/telemetry {"room_id":"OR-DEV","flow_lpm":0.45,"motion":true} appear in the sub terminal


**2025-10/23 - Breadboard Demo 2**

Today I worked on getting our breadboard demo wired up and working. The ESP32 now is able to stream motion status to an external server and display the output in the terminal

The code breadboard_demo2.c contains the esp32 code for this demo. On the server side, follow the steps for hosting the MQTT broker and subscriber

**2025-10/29 - Breadboard Demo**

We demo'd our esp32 streaming data to the mosquitto server. We also showed our UI design to the TA

**2025-10/31 - Group Meeting**

We are on track for our software development, however we are very behind on our hardware due to some ordering and design hiccups

Jeremy - Figure out how to read from mosquitto subscriber and use it to update the DB. Also, configure the Pi to be able to read ESP32 Messages
Refactor & separate code

Hugh - Auto Refresh UI, Change colors dynamically based on state

Suley - Order parts and continue fixing PCB

I refactored our codebase to be much more modular, separating the backend, api routing, and frontend/UI.

New Build Command for Mac: 
  c++ -std=gnu++17 \
  -Iinclude -I/opt/homebrew/include \
  src/util.cpp src/repo.cpp src/views.cpp src/api.cpp src/mqtt_ingestor.cpp src/main.cpp \
  -L/opt/homebrew/lib -lsqlite3 -lmosquitto -pthread \
  -Wl,-rpath,/opt/homebrew/lib \
  -o suction_sense

**2025-11/1 - MQTT_Ingestor**

Today I continued reorganizing our codebase into a more modular format. Then, I created an mqtt_ingestor that allows this application to subscribe to our ESP32 and collect suction telemetry messages. Those messages then are used to update the DB, which in turn updates our UI in near real-time. 

1. To get this to work, compile and start the application normally
2. In another terminal: Run mosquitto -v. This starts the broker
3. To test, send mock publisher messages to the broker ex: mosquitto_pub -h (target IP) -p 1883 -t 'suction/OR 1/state' -m '{"suction_on":false}'
4. Observe the UI dynamically change based on the status of suction

