2025-09-24 – Initial Part Sourcing

Today we met as a team and determined the part numbers we planned to use for our initial PCB prototype. We researched suitable power-path components, the ESP32 footprint, and the AAFS paddle switch.

2025-09-25 – First Schematic Draft

We finished designing the first full draft of our PCB schematic. We double-checked each component’s recommended application circuit and verified that our initial design decisions aligned with the datasheets. The entire schematic was completed.

2025-09-27 – PCB Routing (Round 1)

I routed the PCB for the first time. The layout is not final—will need to return and verify trace widths, clearances, and correct placement later.

2025-10-01 – PCB Revision + Pad Width Issues

Met with the team and continued working on the PCB. We discovered pad width issues with several components, forcing us to adjust footprints and re-route affected traces.

2025-10-04 – Document Work

Met with the team and worked on the Design Document requirements, outlining the system architecture and hardware subsystem.

2025-10-08 – Power Mux Issue Identified

Realized that our power mux required a minimum input threshold voltage that our 3.7 V battery could not reliably meet. We decided we would need to add a boost converter to step 3.7 V → 5 V before feeding the power mux.

2025-10-11 – Software Planning Begins

Met with the team to align on software requirements. I downloaded Crow and began setting up the development environment for writing the C++ client that will run on the Raspberry Pi.

2025-10-14 – Reviewing Schematic Again

Reviewed the schematic with updated power circuitry. Verified that the boost converter’s enable pin and feedback network were correctly configured. Cleaned up net labels.
<img width="1122" height="1404" alt="image" src="https://github.com/user-attachments/assets/6fcda5a1-fb62-4e06-87b1-5570f10a2121" />


2025-10-15 – PCB Ordering Prep

Met with the team to finalize the first PCB revision. Verified BOM entries and footprints before submission. The board and all components were ordered.

2025-10-18 – PCB Round 1 Received

Received the first-round PCB and soldered basic power components. Ensured we had the right ICs and passives available for bring-up.

2025-10-19 – Buck Converter Issue

Testing the board revealed that the buck converter was outputting 4.2 V instead of 3.3 V. Met with Jack Blevins to troubleshoot. Possible issues: incorrect feedback resistor values, solder bridging, or an unstable input from the boost converter.

2025-10-21 – Redesigning Power Section

Worked through issues and redesigned both the schematic and PCB to use a better buck converter and a simpler boost converter. Selected ICs with integrated features and larger footprints to simplify assembly and reduce soldering errors.

2025-10-22 – Additional Hardware Review

Reviewed all footprints and updated the BOM for the new board revision. Verified that minimum trace widths and thermal reliefs met manufacturer guidelines.

2025-10-23 – Ordering Round 2 parts + PCB Submission

Ordered the new components and finalized the second PCB revision. Submitted the Round 2 order. Met with the team to give a progress report.

2025-10-26 – Simulation + EE Validation

Ran through hand-calculations for the new power stages and validated in LTspice that inrush current and steady-state operation matched expectations. Updated documentation accordingly.

2025-10-29 – Board Bring-Up Planning

Created a bring-up checklist for when Round 2 boards arrive:

Verify continuity

Validate all rails with load resistors

Flash ESP32

Verify boost + buck stability under Wi-Fi load

2025-11-01 – Debugging Current Draw

Looked into the new board further and realized that the ESP32’s high peak current draw during Wi-Fi (400–500 mA) would cause brownout if the power stage or battery wasn’t sized appropriately. Added this to hardware risks.

2025-11-03 – Battery Research

To mitigate brownout issues, determined we need a 3.7 V Li-ion battery capable of 400–500 mA peak discharge current. Found and ordered a suitable cell that meets these requirements.

2025-11-07 – PCB Round 3 Prep

Reviewed updated schematic to ensure the power block meets the brownout, efficiency, and stability requirements. Verified that the charging IC configuration followed the datasheet.

2025-11-13 – Waiting for PCB (Round 3)

Waiting on the new PCB revision. All parts have arrived. Expecting to solder and assemble everything for the final demo next week.

2025-11-17 – Schematic Cross-Verification

Double-checked feedback resistor values for the buck and verified that diode orientation + filtering components follow recommended layouts. Wrote updated bring-up steps.

2025-11-20 – Full Assembly + Power Debugging

Assembled the new PCB and discovered that the charging-circuit resistor selection was incorrect.

We are using the MCP7515, which requires a bleed resistor to set charge current.
Our battery can tolerate at most 10 mA charge, but we reduced this to 5 mA for safety:

𝑅
=
5
 
𝑉
5
 
𝑚
𝐴
=
100
𝑘
Ω
R=
5mA
5V
	​

=100kΩ

Next, configured a voltage divider to ensure 0.8 V at the feedback pin of the buck converter based on the datasheet. Chose resistor values per recommended ratios:

(values shown in referenced image)

After adjusting the resistor network, we finally achieved a clean and stable 3.3 V output.
<img width="1436" height="1334" alt="image" src="https://github.com/user-attachments/assets/e2ac8ffb-feef-413e-84a2-c1d5f102818b" />


2025-11-23 – Full System Power Validation

Tested the boost + buck chain under dynamic Wi-Fi load. Observed stable voltage with minimal ripple. Logged current measurements for documentation.

2025-11-27 – Final Firmware Flashing Prep

Prepped the ESP32 flashing process and cleaned up solder pads to ensure we can flash efficiently during system integration. Verified UART connection.

2025-11-30 – Hardware–Software Integration Prep

Created a wiring diagram for connecting the PCB to the suction switch and PIR sensor. Prepared test firmware for each subsystem.

2025-12-02 – Hardware Integration

Integrated all sensors with the PCB. Verified the ESP32 boots correctly from the board’s power supply and that the charge/battery path behaves correctly.

2025-12-03 – Final Verification & Testing (R&V Table)

Performed final engineering verification against our Requirements & Verification (R&V) Table:

Power Requirements

✔ 3.3 V rail stable under 500 mA Wi-Fi bursts

✔ Battery charges at <5 mA

✔ Boost converter maintains 5 V output across load range

Sensor Requirements

✔ PIR motion sensor responds within specified debounce timing

✔ Suction paddle switch consistently triggers logic level thresholds

Communication Requirements

✔ ESP32 successfully publishes MQTT packets for suction + motion

✔ No brownouts during telemetry transmission

PCB Requirements

✔ All footprints correct

✔ No thermal issues during extended operation

<img width="1396" height="738" alt="image" src="https://github.com/user-attachments/assets/887a251d-cb24-4b48-a4e1-2ea4212c5b92" />
<img width="1646" height="738" alt="image" src="https://github.com/user-attachments/assets/8cf07e65-d695-4d89-af8f-6ae1eb4a59f0" />


Everything passed, and the board is fully functional for final demo.

