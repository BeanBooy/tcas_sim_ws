#include <chrono>
#include <memory>
#include <string>
#include <cmath>

#include "rclcpp/rclcpp.hpp"
#include "tcas_sim/plane_node.hpp"

using namespace std::chrono_literals;


Plane::Plane() : Node("plane_X") {

  // every plane will publish to this topic (user planes will subscribe)
  publisher_ = this->create_publisher<interfaces::msg::Pose>(broadcast_topic, 10);

  timer_ = this->create_wall_timer(
    500ms, std::bind(&Plane::timer_callback, this)
  );

  RCLCPP_INFO(this->get_logger(), "Node %s initialized...", this->get_name());
}


void Plane::update_position() {
  aircraft_state.latitude += 0.01;
  aircraft_state.longitude += 0.01;
}

void Plane::broadcast_position() {
  auto message = interfaces::msg::Pose();
  message.icao_hex = icao_hex;
  message.latitude = aircraft_state.latitude;
  message.longitude = aircraft_state.longitude;
  message.altitude = aircraft_state.altitude;
  message.heading = aircraft_state.heading;
  message.velocity = aircraft_state.velocity;
  publisher_->publish(message);
}

void Plane::timer_callback() {
  if (is_active) {
    //update_position();
    broadcast_position();
  }
}
