#pragma once
#include <string>

using namespace std;

/*
This file contains the structs used for our application
Operating Room, Room Event
*/

// ───────────────────────────────────────────────
// Struct representing an operating room
// ───────────────────────────────────────────────
struct OperatingRoom {
    int id;
    string room_number;
    bool occupency;
    string schedule;
    bool suction_on;
};
// ───────────────────────────────────────────────
// Struct representing a room event
// ───────────────────────────────────────────────
struct RoomEvent {
    string procedure;
    string start_time;
    string end_time;
    bool active;
};