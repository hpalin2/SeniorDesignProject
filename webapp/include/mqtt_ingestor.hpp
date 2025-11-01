// include/mqtt_ingestor.hpp
#pragma once

#include <string>
#include <thread>
#include <atomic>

// Forward declarations to keep this header lightweight.
// (Definitions live in the .cpp)
class Repo;
struct mosquitto;

class MqttIngestor {
public:
    // Construct with broker info and a topic filter like "suction/+/state".
    MqttIngestor(Repo& repo,
                 std::string broker_host = "localhost",
                 int broker_port = 1883,
                 std::string topic_filter = "suction/+/state",
                 int qos = 1);

    // Non-copyable, movable (optional—enable if you want)
    MqttIngestor(const MqttIngestor&) = delete;
    MqttIngestor& operator=(const MqttIngestor&) = delete;
    MqttIngestor(MqttIngestor&&) = delete;
    MqttIngestor& operator=(MqttIngestor&&) = delete;

    // Start the MQTT loop in a background thread.
    // Returns false if initialization/connection failed.
    bool start();

    // Stop the loop and clean up resources (safe to call multiple times).
    void stop();

    ~MqttIngestor();

private:
    // mosquitto callbacks (registered per-connection)
    static void on_connect(struct mosquitto* m, void* userdata, int rc);
    static void on_disconnect(struct mosquitto* m, void* userdata, int rc);
    static void on_message(struct mosquitto* m, void* userdata, const struct mosquitto_message* msg);

    // Helper to parse "suction/<room>/state" → "<room>"
    static std::string extract_room_from_topic(const std::string& topic);

private:
    Repo& repo_;
    std::string host_;
    int         port_;
    std::string topic_;
    int         qos_;

    struct mosquitto* mosq_ = nullptr;
    std::thread       loop_thread_;
    std::atomic<bool> running_{false};
};
