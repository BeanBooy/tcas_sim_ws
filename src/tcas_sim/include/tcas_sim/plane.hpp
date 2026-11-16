#pragma once
#include "rclcpp/rclcpp.hpp"
#include "interfaces/msg/pose.hpp"

typedef struct AircraftState {
  double latitude = 0.0;
  double longitude = 0.0;
  double altitude = 0.0;
  double heading = 0.0;
  double velocity = 0.0;
  double vertical_speed = 0.0;
  double pitch = 0.0;
  double roll = 0.0;
} aircraft_state_t;

template <typename T>
struct StampedValue {
    T value;
    std::chrono::steady_clock::time_point timestamp = std::chrono::steady_clock::now();

    // value setter with timestamp
    void set(T value) {
      this->value = value;
      this->timestamp = std::chrono::steady_clock::now();
    }

    // helper to easily check the age of the value
    double age_sec() const {
        std::chrono::duration<double> duration = std::chrono::steady_clock::now() - timestamp;
        return duration.count();
    }
};

typedef struct StampedAircraftState {
  StampedValue<double> latitude;
  StampedValue<double> longitude;
  StampedValue<double> altitude;
  StampedValue<double> heading;
  StampedValue<double> velocity;
  StampedValue<double> vertical_speed;
} stamped_aircraft_state_t;


class Plane : public rclcpp::Node {
public:
  Plane();

protected:
  rclcpp::TimerBase::SharedPtr broadcast_timer;
  rclcpp::Publisher<interfaces::msg::Pose>::SharedPtr broadcast_publisher;
  std::string broadcast_topic = "/airspace_broadcast";
  AircraftState aircraft_state{};
  bool is_active;  // for user planes always true, for static planes only if connected to flight
  std::string icao_hex;

  virtual aircraft_state_t get_current_pos_state() = 0;
  void broadcast_position(aircraft_state_t& aircraft_state) const;
  void broadcast_timer_callback();
};