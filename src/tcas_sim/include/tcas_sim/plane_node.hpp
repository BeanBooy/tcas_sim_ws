#pragma once
#include "rclcpp/rclcpp.hpp"
#include "tcas_sim/msg/pose.hpp"

struct AircraftState {
  double latitude = 0.0;
  double longitude = 0.0;
  double altitude = 0.0;
  double heading = 0.0;
  double velocity = 0.0;
};

class Plane : public rclcpp::Node {
public:
  Plane();
  void init(std::string callsign, float lat, float lon, float alt);
  void kill();
  void start();

protected:
  rclcpp::TimerBase::SharedPtr timer_;
  rclcpp::Publisher<tcas_sim::msg::Pose>::SharedPtr publisher_;
  std::string broadcast_topic = "/airspace_broadcast";
  AircraftState aircraft_state{};
  bool sleep = false; // Initialize here!
  std::string icao_hex;
  std::string flight_number;

  virtual void update_position();
  void broadcast_position();
  void timer_callback();
};