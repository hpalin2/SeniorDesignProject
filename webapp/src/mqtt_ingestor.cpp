// src/mqtt_ingestor.cpp
#include "mqtt_ingestor.hpp"
#include <mosquitto.h>
#include <nlohmann/json.hpp>
#include <utility>
#include <string>
#include <thread>
#include <atomic>
#include <iostream>
#include "repo.hpp"

// -------- ctor / dtor --------

MqttIngestor::MqttIngestor(Repo& repo,
                           std::string broker_host,
                           int broker_port,
                           std::string topic_filter,
                           int qos)
    : repo_(repo),
      host_(std::move(broker_host)),
      port_(broker_port),
      topic_(std::move(topic_filter)),
      qos_(qos) {}

MqttIngestor::~MqttIngestor() {
    stop();
}

// -------- public API --------

bool MqttIngestor::start() {
    // Guard against double-start
    if (running_.load()) return true;

    mosquitto_lib_init();

    mosq_ = mosquitto_new("suction-ingestor", true /*clean session*/, this);
    if (!mosq_) {
        mosquitto_lib_cleanup();
        return false;
    }

    mosquitto_connect_callback_set(mosq_, &MqttIngestor::on_connect);
    mosquitto_message_callback_set(mosq_, &MqttIngestor::on_message);
    mosquitto_disconnect_callback_set(mosq_, &MqttIngestor::on_disconnect);

    mosquitto_reconnect_delay_set(mosq_, 1, 10, true);

    int rc = mosquitto_connect(mosq_, host_.c_str(), port_, 30 /* keepalive */);
    if (rc != MOSQ_ERR_SUCCESS) {
        mosquitto_destroy(mosq_);
        mosq_ = nullptr;
        mosquitto_lib_cleanup();
        return false;
    }

    running_.store(true);
    // Run the blocking loop on a background thread.
    loop_thread_ = std::thread([this]{
        mosquitto_loop_forever(mosq_, -1 /* use defaults */, 1);
    });

    return true;
}

void MqttIngestor::stop() {
    if (!running_.exchange(false)) return;

    if (mosq_) {
        // Trigger the loop to exit
        mosquitto_disconnect(mosq_);
    }
    if (loop_thread_.joinable()) {
        loop_thread_.join();
    }
    if (mosq_) {
        mosquitto_destroy(mosq_);
        mosq_ = nullptr;
    }
    mosquitto_lib_cleanup();
}

// -------- static callbacks --------

void MqttIngestor::on_connect(struct mosquitto* m, void* userdata, int rc) {
    auto* self = static_cast<MqttIngestor*>(userdata);
    if (!self) return;

    if (rc == 0) {
        // Connected: subscribe to the filter
        mosquitto_subscribe(m, nullptr, self->topic_.c_str(), self->qos_);
    } else {
        // (optional) log rc
    }
}

void MqttIngestor::on_disconnect(struct mosquitto* /*m*/, void* /*userdata*/, int /*rc*/) {
    // (optional) log or metrics
}

void MqttIngestor::on_message(struct mosquitto* /*m*/,
                              void* userdata,
                              const struct mosquitto_message* msg) {
    auto* self = static_cast<MqttIngestor*>(userdata);
    if (!self || !msg || !msg->payload || msg->payloadlen <= 0) return;

    try {
        std::string topic = msg->topic ? std::string(msg->topic) : std::string();
        std::string payload(static_cast<const char*>(msg->payload),
                            static_cast<size_t>(msg->payloadlen));

        // Expect "suction/<room>/state" → "<room>"
        std::string room_number = extract_room_from_topic(topic);
        if (room_number.empty()) {
            return; // ignore malformed topic
        }

        // Parse JSON: expect {"suction_on": true/false, ...}
        nlohmann::json j = nlohmann::json::parse(payload);
        bool suction_on = j.value("suction_on", false);
        bool occupancy     = j.value("motion", false);

        int room_id = self->repo_.ensure_room_id(room_number);
        if (room_id > 0) {
            self->repo_.update_suction(room_id, suction_on);
            self->repo_.update_occupancy(room_id, occupancy);
        }
    } catch (const std::exception& e) {
        std::cerr << "Error parsing MQTT message: " << e.what() << std::endl;
    }
}

std::string MqttIngestor::extract_room_from_topic(const std::string& topic) {
    // naive split: "suction/OR 1/state"
    auto first = topic.find('/');
    if (first == std::string::npos) return {};
    auto second = topic.find('/', first + 1);
    if (second == std::string::npos) return {};
    return topic.substr(first + 1, second - (first + 1)); // "OR 1"
}
