#include <chrono>
#include <memory>
#include <string>
#include <cmath>

#include "rclcpp/rclcpp.hpp"
#include "tcas_sim/msg/pose.hpp"
#include "interfaces/msg/adsb.hpp"
#include "tcas_sim/plane_node.hpp"

using namespace std::chrono_literals;


Plane::Plane() : Node("plane_X") {

  this->declare_parameter<std::string>("flight_number", "unset");
  this->get_parameter("flight_number", flight_number);

  // plane will both publish and subscribe to this topic
  publisher_ = this->create_publisher<tcas_sim::msg::Pose>(broadcast_topic, 10);

  timer_ = this->create_wall_timer(
    500ms, std::bind(&Plane::timer_callback, this)
  );

  RCLCPP_INFO(this->get_logger(), "Node %s [flight_number=%s] initialized and listening...", this->get_name(), flight_number.c_str());
}

void Plane::init(std::string callsign, float lat, float lon, float alt) { // etc...
  flight_number = callsign; // is this intended like this ?
  aircraft_state.latitude = lat;
  aircraft_state.longitude = lon;
  aircraft_state.altitude = alt;
  start();
}

void Plane::kill() {
  sleep = true;
  // TODO maybe unsubscribe from broadcasts
}

void Plane::start() {
  sleep = false;
}


void Plane::update_position() {
  aircraft_state.latitude += 0.01;
  aircraft_state.longitude += 0.01;
}

void Plane::broadcast_position() {
  auto message = tcas_sim::msg::Pose();
  message.icao_hex = icao_hex;
  message.flight_number = flight_number;
  message.latitude = aircraft_state.latitude;
  message.longitude = aircraft_state.longitude;
  message.altitude = aircraft_state.altitude;
  message.heading = aircraft_state.heading;
  message.velocity = aircraft_state.velocity;

  publisher_->publish(message);
}

void Plane::timer_callback() {
      update_position();
      broadcast_position();
}
