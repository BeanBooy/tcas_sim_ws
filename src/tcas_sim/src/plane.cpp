#include <chrono>
#include <memory>
#include <string>
#include <cmath>

#include "rclcpp/rclcpp.hpp"
#include "tcas_sim/plane.hpp"

using namespace std::chrono_literals;


Plane::Plane() : Node("plane_X") {
  // every plane broadcasts its position (user planes will subscribe)
  broadcast_publisher = this->create_publisher<interfaces::msg::Pose>(broadcast_topic, 10);
  broadcast_timer = this->create_wall_timer(500ms, std::bind(&Plane::broadcast_timer_callback, this));
}

void Plane::broadcast_position(aircraft_state_t& aircraft_state) const {
  auto message = interfaces::msg::Pose();
  message.icao_hex = icao_hex;
  message.latitude = aircraft_state.latitude;
  message.longitude = aircraft_state.longitude;
  message.altitude = aircraft_state.altitude;
  message.heading = aircraft_state.heading;
  message.velocity = aircraft_state.velocity;
  message.pitch = aircraft_state.pitch;
  message.roll = aircraft_state.roll;
  broadcast_publisher->publish(message);
}

void Plane::broadcast_timer_callback() {
  if (is_active) {
    aircraft_state_t current_pos_state = get_current_pos_state();
    broadcast_position(current_pos_state);
  }
}
