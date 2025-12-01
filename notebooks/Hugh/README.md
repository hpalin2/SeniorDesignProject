## Hugh's New Journal
### **09/24/2025 — Project Reset + Planning**

**Objectives:**

* Re-align team progress and establish revised timeline.
* Begin PCB + component selection.

**Work Completed:**

* Met with Lukas to set milestones + deliverables.
* Built shared Google Doc component list.
* Selected Raspberry Pi & touchscreen model constraints.
* Part list: https://docs.google.com/document/d/1e973Mm_vhnLTH5vz_4C17udLJVd2HXl1_8vYD8PnHUg/edit?usp=sharing

**Reflection & Design Rationale:**

* Front-loading part selection reduces PCB revision count later.

**Next Steps:**

* Complete rough PCB layout
* Schedule review meeting with Jack
* Finalize BOM

---

### **09/26/2025 — Schematic & Routing Progress**

**Objectives:**

* Advance PCB creation + coordinate machining.

**Work Completed:**

* Led schematic creation.
* Generated machining drawing + emailed to shop.
* Gathered footprints for components.

**Reflection:**
Machine shop contacted early → reduces fabrication turnaround risk.
**notes**
We met today and worked on our schematic. I also created the drawing and emailed it to the machine shop. Upon doing so we have put them in the loop. We are going to try and finish our pcb today. I set up a meeting with Jack on Tuesday. I worked on developing the PCB and now it just needs to be routed. I found all the footprints for our parts
**Next Steps:**

* Route remaining traces
* Prepare mentor review questions

---

### **10/03/2025 — Charging Circuit Redesign**

**Objectives:**

* Implement power architecture changes.

**Work Completed:**

* Finished routing pass.
* Added rechargeable Li-ion circuit (mentor suggestion).
* Need new power mux package.

**Reflection:**
Recharge feature eliminates coin cell swaps → required for hospital workflow.
**notes**
This week we discussed multiple times the breadboard design. We have about finished our pcb design. Today I worked on finishing the routing of the pcb after we made changes, we added a charging circuit. We did this after talking to Jack Blevins, who suggested we just directly power our MCU from the battery. This way you won't have to switch out the coincell battery, it will recharge off the wall. Now we need to replace our power mux with a different one with different packaging
**Next Steps:**

* Select compatible mux footprint
* Re-route power stage

---

### **10/05/2025 — PCB QC + Pre-Flight**

**Objectives:**

* Prepare PCB for fab review.

**Work Completed:**

* Fixed footprint errors.
* Uploaded to PCBWay for final review.
* Guided schematic footer update.
**notes**
I fixed the pcb up and fixed some errors, uploaded it to pcbway to do one last review before we send it to the team to print it. Helped Suley with pcb today and had him update the schematic with the new footer one of the TAs sent us.
**Next Steps:**

* Fabrication submission
* Continue validation checklist

---

### **10/08/2025 — PCB Submitted**

**Objectives:** Submit board + begin documentation.

**Work Completed:**

* Corrected pad width.
* Divided design doc sections for writing.
**notes**
I fixed the pcb and submitted it under first pcb order. Specifically fixed the pad width of one of our ICs. Split up the work of the design doc and are looking forward to meeting again on Friday. I will be working on the design doc tomorrow. 10/10/25: We worked on the design doc together and developed our schedule. We have fully created a robust schedule that will allow us to complete our project on time. Talking to Suley it seems we need to make pcb revisions such as the voltage divider and buck converter.
**Next Steps:**

* Work design doc tomorrow
* Establish test procedure outline

---

### **10/10/2025 — Schedule + Revision Planning**

**Objectives:**

* Complete schedule + identify revision items.

**Work Completed:**

* Built semester schedule.
* Identified electrical fixes → voltage divider, buck converter.
**notes**
We worked on the design doc together and developed our schedule. We have fully created a robust schedule that will allow us to complete our project on time. Talking to Suley it seems we need to make pcb revisions such as the voltage divider and buck converter.
**Next Steps:**

* Execute PCB revision pass
* Verify regulator stability

---

### **10/11/2025 — Documentation Expansion**

**Objectives:**

* Expand subsystem details.

**Work Completed:**

* Rewrote sensor subsystem + testing criteria.
**notes**
Since Friday I have been putting in a bit of work to the design document. I am redoing some of our subsystems such as the sensor subsystem. We are planning to finish it on sunday.
**Next Steps:**

* Finalize doc Sunday
* Begin test outline

---

### **10/13/2025 — Documentation Complete + PCB Rev 2**

**Objectives:**

* Submit board + finalize writing.
**notes**
Added our final changes to design document and submitted a new PCB for Lukas to order.
**Work Completed:**

* Submitted revised PCB.
* Completed design doc edits.

---

### **10/15/2025 — Mechanical Requirements + Software Split**

**Objectives:**

* Determine flow + valve geometry requirements.
* Split code roles.

**Work Completed:**

* Determined required hose diameter and valve spec for detection.
* Agreed initial software setup: local → Raspberry Pi.
**notes**
Talked to Greg about senior design project, we found out we need to find what valve size we need to have good flow for our sensor to still detect. We also need a shop vac hose size. As soon as we get that flow rate sensor hand it off to him and he will make it fit. We met and discussed how to split up the app. First Suley and I will try and get it working on our PCs (mine will be harder since they are both Mac OS). Then we will work on building the first webapp.
**Reflection:**
Mechanical tolerance now critical variable for flow-based switching.

---

### **10/16/2025 — Crow Deployed**

**Objectives:**

* Build webapp backend.

**Work Completed:**

* Compiled CROW manually (no Conda).
* Built local HTML server + merged code into repo.
**notes**
Worked on the webapp and built those changes throughout the day, and got it working on my computer. Got the changes and made a working html server with crows framework. Setting up crow is a pain on ubuntu since it doesn't have a build you can download from conda. Had to manually compile it and install it on my PC. Added my code changes to codeH.
**Next Steps:**

* Integrate DB + dynamic UI

---

### **10/17/2025 — Multi-Device Build Success**

**Objectives:**

* Get server working cross-machine.

**Work Completed:**

* Fixed dependencies + UI shells complete.
**notes**
Edited my environment to get crow working on laptop as well. I created the basic crow framework and design of our webapp that will be displayed onto the raspberry pi. It needs to be integrated with live data and not static using a sqlite database that will pull a history of the OR schedule data and the flow rate history.
**Next Steps:**

* Begin SQLite integration

---

### **10/18/2025 — Database Integration**

**Objectives:**

* Move UI from static → live DB.

**Work Completed:**

* Created `room_schedule` table + insert/update tests.
* Designed timestamp-based history system.

**Technical Commands:**

```
sqlite3 suction_sense.db
g++ main.cpp -o server -pthread -lsqlite3
```
**notes**
I worked on our webapp and now it pulls from a SQLite database that has one working table: room_schedule. This table allows us to store the history of OR room assignments daily. Now we just need to store the suction status. We can store the history at various time stamps throughout the day. This can get data expensive, so the idea is to store a current state for all rooms of suction status. Whenever we detect a change we can store a timestamp history of suctions status changes.
**Next Steps:**

* Add suction_state table
* Implement MQTT ingestion later

---

### **10/22/2025 — Hardware Progress + ESP32 Debugging**

**Objectives:**

* Get parts + fix brownouts.
**notes**
I worked on talking to Greg and the rest of the Carle team over email to discuss packages and test stand design. Worked with Jeremy on fixing ESP32 brownout, we were able to get a new breadboard from the locker. I am working on getting the parts to the machine shop so they can begin construction of our test stand.
**Work Completed:**

* Coordinated with machine shop; acquired breadboard.
* Investigated ESP brownout root causes.

---

### **10/24/2025 — Parts Confirmed + UI Improvement**

**Objectives:**

* Confirm mechanical interfaces + DB write-back.

**Work Completed:**

* Verified hose 2.5" OD.
* Picked up delayed parts.
* UI supports room scheduling.
**notes**
I forgot to mention the work I did yesterday: I went to Ace Hardware and measured the size of our extension pipe to determine if the vacuum hose was 2.5 in. diameter outer not inner. I also emailed Martin to get a status on when I can pick up our parts. Seems like I can grab it on Monday. I got the room calendar and state to update for room 2, but not the other rooms. The room_schedule works though.
**Commands Used:**

```
g++ main.cpp -o server -pthread -lsqlite3 
INSERT INTO room_schedule (...) VALUES (...);
UPDATE suction_state SET suction_on=1 WHERE room_id=2;
```

---

### **10/29/2025 — Parts Delivered + Build Begins**

**Objectives:**

* Stage hardware assembly.
**notes**
Last few days I have been working on getting our parts. I picked them up yesterday from Michael Martin, seems he might've gone on leave in the middle of our correspondence. It was very difficult to collect our parts. I finally got our parts to the machine shop and now they are constructing our test stand. Now we just have to connect everything and print a housing for our motion sensor.
**Work Completed:**

* Delivered to machine shop.
* Planning suction housing enclosure.

---

### **10/31/2025 — PCB Bring-Up**

**Objectives:**

* Populate board + fix UI loop.
**notes**
Last few days we worked on setting up our pcb. I brought all of our parts then Suley lost one so we have to reorder. Now we ordered 5 of every part and will work on our new pcb. We also bought a programmer for our ESP32 since we forgot to add a program header. I now have to work on fixing the UI for colors and auto page refreshing.
**Work Completed:**

* Ordered replacement components.
* UI now auto-refresh WIP.

---

### **11/04/2025 — Web + Report Progress**

**Objectives:**

* Continue web build.
**notes**
Worked on finishing my progress on the webapp as well as finishing the individual progress report. We are meeting today at 5 to do some work and discuss.
**Work Completed:**

* Finalized web progress + wrote report.
* Team work session planned.

---

### **11/07/2025 — UI Complete**

**Objectives:**

* Finalize interface behavior.
**notes**
Jeremy and I fixed the UI it now works as intended we just have to fix the database schema. The last few days I have been working on that.
**Work Completed:**

* UI now fully functional (with Jeremy).
* DB schema next target.

---

### **11/13/2025 — Test Stand Near Completion**

**Objectives:**

* Finalize machining status.
**notes**
This week I worked on cordinating with the machine shop to ensure our test stand would be finished. We have come to the final design of the test stand and they are finishing assembly. Soon enough we will have completed and begin testing. Just waiting on the PCB from course staff.
**Work Completed:**

* Test stand assembly nearly complete.
* Await PCB for integrated testing.

---

### **11/16/2025 — First PCB Success + Schema Revision**

**Objectives:**

* Integrate board + DB.
**notes**
I am now working on fixing our database schema as we are not able to interface with EPIC. We got our first board working after a lot of fixes. Now we have a boolean for our database schema all we have to do is test this with our board.
**Work Completed:**

* Fixed board + new boolean schema implemented.

---

### **11/17/2025 — Flow Test #1**

**Objectives:**

* Characterize suction behavior.
**notes**
I picked up our test stand and began testing with it to try and understand the mechanics of our paddle since we have it in person now. Upon testing it I was able to see that turning on our vacuum at any spring level we would detect flow. We began having issues with the paddle getting stuck after suction.
**Work Completed:**

* Paddle detects flow reliably at all spring loads.
* Sticking observed post-suction.

---

### **11/18/2025 — Detection Reliability Debug**

**Objectives:**

* Improve paddle consistency.
**notes**
Jeremy and I tested the firmware of detecting suction and wirelessly communicating it with our computer and we ran into the issue of the paddle getting stuck and our detection being very inconsistent. We tried our connections and they weren't the issue, we believe it might be an issue with our spring. We are going to keep trying different changes to the mechanic settings of our switch to improve its accuracy.
**Work Completed:**

* Wireless telemetry passing.
* Switch inconsistent due to contact alignment.

---

### **11/19/2025 — Mechanical Fix Achieved**

**Objectives:**

* Fix sticking issue.

**Work Completed:**

* Paddle trimmed → no sticking.
* Switch reoriented → reliable triggering.
**notes**
I worked on getting our test stand paddle sensor working, I did this by working with Skee from the machine shop. He was able to fix our issues of the paddle getting stuck by cutting off some of the material. I then worked on fixing our issue of our detection being inconsistent, by proding the switch with a screwdriver and doing research I found our switch wasn't connecting 100% of the time to our paddle. By adjusting its position with a screw driver it is fixed, we have debated on hot glueing the switch to the contact area with the paddle if this issue keeps coming up.
**Next Steps:**

* Long-cycle stress testing
* Mount switch permanently

---

### **11/20/2025 — Full Housing Started**

**Objectives:**

* Begin case + enclosure print.
**notes**
Worked on generating the first iteration of our housing for our pcb and motion sensor. This will allow us to have a nice pretty stand to complement our test stand for suction and keep everything pretty. I am now working on creating a final print for our housing.
**Work Completed:**

* Built housing for PCB + motion sensor.
* Preparing final model for print.
