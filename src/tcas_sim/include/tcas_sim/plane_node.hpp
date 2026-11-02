#pragma once
#include "rclcpp/rclcpp.hpp"
#include "interfaces/msg/pose.hpp"

struct AircraftState {
  double latitude = 0.0;
  double longitude = 0.0;
  double altitude = 0.0;
  double heading = 0.0;
  double velocity = 0.0;
  double vertical_speed = 0.0;
};

template <typename T>
struct StampedValue {
    T value;
    std::chrono::steady_clock::time_point timestamp;

    // value setter with timestamp
    void set(T value) {
      this->value = value;
      this->timestamp = std::chrono::steady_clock::now();
    }

    // helper to easily check the age of the value
    long long age_ms() const {
        auto duration = std::chrono::steady_clock::now() - timestamp;
        return std::chrono::duration_cast<std::chrono::milliseconds>(duration).count();
    }
};

struct StampedAircraftState {
  StampedValue<double> latitude;
  StampedValue<double> longitude;
  StampedValue<double> altitude;
  StampedValue<double> heading;
  StampedValue<double> velocity;
  StampedValue<double> vertical_speed;
};


class Plane : public rclcpp::Node {
public:
  Plane();

protected:
  rclcpp::TimerBase::SharedPtr timer_;
  rclcpp::Publisher<interfaces::msg::Pose>::SharedPtr publisher_;
  std::string broadcast_topic = "/airspace_broadcast";
  AircraftState aircraft_state{};
  bool is_active;  // for user planes always true, for static planes only if connected to flight
  std::string icao_hex;

  virtual struct AircraftState get_current_pos_state() = 0;
  void broadcast_position(struct AircraftState& aircraft_state) const;
  void timer_callback();
};