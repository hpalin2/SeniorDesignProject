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