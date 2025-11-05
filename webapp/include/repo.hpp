#pragma once
#include <sqlite3.h>
#include <mutex>
#include <string>
#include <vector>
#include "models.hpp"

class Repo {
public:
    explicit Repo(const std::string& db_path);
    ~Repo();

    // non-copyable
    Repo(const Repo&) = delete;
    Repo& operator=(const Repo&) = delete;

    bool ok() const { return db_ != nullptr; }

    // Seed / init
    void seed_if_empty();

    // Queries
    std::vector<OperatingRoom> load_rooms();

    // Mutations
    void update_suction(int room_id, bool suction_on);
    void insert_room(const OperatingRoom& r);

    //map something like "OR 3" → rooms.id
    int ensure_room_id(const std::string& room_number);

private:
    // helpers (locked within public API)
    RoomEvent get_current_event_for_room(int room_id);
    bool get_latest_suction_status(int room_id);
    void log_suction_status(int room_id, bool suction_on);

    void exec_ddl(const char* sql);
    void init_schema();

private:
    sqlite3* db_{nullptr};
    std::mutex mtx_;
};
