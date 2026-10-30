#include <chrono>
#include <memory>
#include <string>
#include <cmath>

#include "rclcpp/rclcpp.hpp"
#include "tcas_sim/msg/pose.hpp"
#include "interfaces/msg/adsb.hpp"

using namespace std::chrono_literals;


class Plane : public rclcpp::Node
{
public:
  Plane() : Node("plane_X") {

    this->declare_parameter<std::string>("flight_number", "unset");
    this->get_parameter("flight_number", flight_number);

    // plane will both publish and subscribe to this topic
    std::string broadcast_topic = "/airspace_broadcast";

    publisher_ = this->create_publisher<tcas_sim::msg::Pose>(broadcast_topic, 10);

    subscription_ = this->create_subscription<tcas_sim::msg::Pose>(
      broadcast_topic, 10,
      std::bind(&Plane::broadcast_callback, this, std::placeholders::_1)
    );

    subscription_adsb = this->create_subscription<interfaces::msg::Adsb>(
      "/adsb_cmd", 10,
      std::bind(&Plane::adsb_callback, this, std::placeholders::_1)
    );

    timer_ = this->create_wall_timer(
      500ms, std::bind(&Plane::timer_callback, this)
    );

    RCLCPP_INFO(this->get_logger(), "Node %s [flight_number=%s] initialized and listening...", this->get_name(), flight_number.c_str());
  }

  void init(std::string callsign, float lat, float lon, float alt) { // etc...
    start();
    // TODO set all parameters
  }

  void kill() {
    sleep = true;
    // TODO maybe unsubscribe from broadcase
  }

  void start() {
    sleep = false;
  }

private:
  rclcpp::TimerBase::SharedPtr timer_;
  rclcpp::Publisher<tcas_sim::msg::Pose>::SharedPtr publisher_;
  rclcpp::Subscription<tcas_sim::msg::Pose>::SharedPtr subscription_;

  rclcpp::Subscription<interfaces::msg::Adsb>::SharedPtr subscription_adsb;

  bool sleep;  // turns plane on or off because starting and killing plane nodes sucks!
  // plane data
  std::string icao_hex;
  std::string flight_number;
  float lat;
  float lon;
  float alt;
  float heading;
  float velocity;

  void timer_callback() {
    // simulate movement
    lat += 0.01;
    lon += 0.01;

    auto message = tcas_sim::msg::Pose();
    message.icao_hex = icao_hex;
    message.flight_number = flight_number;
    message.latitude = lat;
    message.longitude = lon;

    publisher_->publish(message);
  }

  void broadcast_callback(const tcas_sim::msg::Pose::SharedPtr msg) {
    // self-echo filter
    if (msg->icao_hex == icao_hex) {
      return; 
    }

    // calculate distance to the other aircraft
    float dx = msg->latitude - lat;
    float dy = msg->longitude - lon;
    float distance = std::sqrt(dx * dx + dy * dy);

    // filter out messages out of our radar range (simulates pysical radar range limitations)
    float radar_range = 0.5; 
    if (distance > radar_range) {
      return;
    }

    // if we reach here, other plane is within our radar range
    //RCLCPP_INFO(this->get_logger(), "--> detected neighbor [%s] at distance %.4f",  msg->icao_hex.c_str(), distance);
  }

  void adsb_callback(const interfaces::msg::Adsb::SharedPtr msg) {
    // if message not for me, ignore
    if (msg->node_name != this->get_name()) {
      return; 
    }
    
    // TODO my adsb command receive here!!
    RCLCPP_INFO(this->get_logger(), "--> received adsb message!");

  }

};

int main(int argc, char * argv[])
{
  rclcpp::init(argc, argv);
  rclcpp::spin(std::make_shared<Plane>());
  rclcpp::shutdown();
  return 0;
}
