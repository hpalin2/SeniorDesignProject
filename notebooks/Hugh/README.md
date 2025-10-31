## Hugh's New Journal

9/24/25:

We met today with Lukas and have reorinted ourselves to finish our goals.

Our Goals:

* Rough PCB design by end of weekend
* Schedule with Jack to go over desgin decisions
* Create a final part list
* Split up PCB design

We worked on creating a part list on google docs:[https://docs.google.com/document/d/1e973Mm_vhnLTH5vz_4C17udLJVd2HXl1_8vYD8PnHUg/edit?usp=sharing](https://)

I worked on finding the correct Raspberry Pi parts including our touch screen. We should be good and done with our design decisions.

9/26/25:

We met today and worked on our schematic. I also created the drawing and emailed it to the machine shop. Upon doing so we have put them in the loop. We are going to try and finish our pcb today. I set up a meeting with Jack on Tuesday.
I worked on developing the PCB and now it just needs to be routed. I found all the footprints for our parts

10/3/25:

This week we discussed multiple times the breadboard design. We have about finished our pcb design. Today I worked on finishing the routing of the pcb after we made changes, we added a charging circuit. We did this after talking to Jack Blevins, who suggested we just directly power our MCU from the battery. This way you won't have to switch out the coincell battery, it will recharge off the wall. Now we need to replace our power mux with a different one with different packaging

10/5/25: I fixed the pcb up and fixed some errors, uploaded it to pcbway to do one last review before we send it to the team to print it. Helped Suley with pcb today and had him update the schematic with the new footer one of the TAs sent us.

10/8/25: I fixed the pcb and submitted it under first pcb order. Specifically fixed the pad width of one of our ICs. Split up the work of the design doc and are looking forward to meeting again on Friday. I will be working on the design doc tomorrow.
10/10/25: We worked on the design doc together and developed our schedule. We have fully created a robust schedule that will allow us to complete our project on time. Talking to Suley it seems we need to make pcb revisions such as the voltage divider and buck converter.

10/11/25: Since Friday I have been putting in a bit of work to the design document. I am redoing some of our subsystems such as the sensor subsystem. We are planning to finish it on sunday.

10/13/25: Added our final changes to design document and submitted a new PCB for Lukas to order.

10/15/25: Talked to Greg about senior design project, we found out we need to find what valve size we need to have good flow for our sensor to still detect. We also need a shop vac hose size. As soon as we get that flow rate sensor hand it off to him and he will make it fit. We met and discussed how to split up the app. First Suley and I will try and get it working on our PCs (mine will be harder since they are both Mac OS). Then we will work on building the first webapp.

10/16/25: Worked on the webapp and built those changes throughout the day, and got it working on my computer. Got the changes and made a working html server with crows framework. Setting up crow is a pain on ubuntu since it doesn't have a build you can download from conda. Had to manually compile it and install it on my PC. Added my code changes to codeH.

10/17/25: Edited my environment to get crow working on laptop as well. I created the basic crow framework and design of our webapp that will be displayed onto the raspberry pi. It needs to be integrated with live data and not static using a sqlite database that will pull a history of the OR schedule data and the flow rate history.

10/18/25: I worked on our webapp and now it pulls from a SQLite database that has one working table: room_schedule. This table allows us to store the history of OR room assignments daily. Now we just need to store the suction status. We can store the history at various time stamps throughout the day. This can get data expensive, so the idea is to store a current state for all rooms of suction status. Whenever we detect a change we can store a timestamp history of suctions status changes.

10/22/25: I worked on talking to Greg and the rest of the Carle team over email to discuss packages and test stand design. Worked with Jeremy on fixing ESP32 brownout, we were able to get a new breadboard from the locker. I am working on getting the parts to the machine shop so they can begin construction of our test stand.

10/24/25: I forgot to mention the work I did yesterday: I went to Ace Hardware and measured the size of our extension pipe to determine if the vacuum hose was 2.5 in. diameter outer not inner. I also emailed Martin to get a status on when I can pick up our parts. Seems like I can grab it on Monday. I got the room calendar and state to update for room 2, but not the other rooms. The room_schedule works though.

Commands:
sqlite3 suction_sense.db -> open our database
http://localhost:18080/ -> Local host of website
g++ main.cpp -o server -pthread -lsqlite3 -> command to compile our project
INSERT INTO room_schedule (room_id, procedure, start_time, end_time, date)
VALUES (1, 'Appendectomy', '13:00', '15:30', '2025-10-24');
-> insertion example for room schedule
UPDATE suction_state
SET suction_on = 1,
last_updated = datetime('now')
WHERE room_id = 2;

10/29/25: Last few days I have been working on getting our parts. I picked them up yesterday from Michael Martin, seems he might've gone on leave in the middle of our correspondence. It was very difficult to collect our parts. I finally got our parts to the machine shop and now they are constructing our test stand. Now we just have to connect everything and print a housing for our motion sensor.

10/31/2025: Last few days we worked on setting up our pcb. I brought all of our parts then Suley lost one so we have to reorder. Now we ordered 5 of every part and will work on our new pcb. We also bought a programmer for our ESP32 since we forgot to add a program header. I now have to work on fixing the UI for colors and auto page refreshing.