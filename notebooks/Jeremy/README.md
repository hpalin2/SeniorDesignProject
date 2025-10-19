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

Change Mosquitto config by doing code /opt/homebrew/etc/mosquitto/mosquitto.conf 
and inputting this: 
listener 1883 0.0.0.0
allow_anonymous true

Then run mosquitto -v -c /opt/homebrew/etc/mosquitto/mosquitto.conf