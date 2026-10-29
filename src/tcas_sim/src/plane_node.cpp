#include <chrono>
#include <memory>
#include <string>

#include "rclcpp/rclcpp.hpp"
#include "tcas_sim/msg/pose.hpp"

using namespace std::chrono_literals;


class Plane : public rclcpp::Node
{
public:
  Plane() : Node("plane") {

    this->declare_parameter<std::string>("plane_id", "unset");
    this->get_parameter("plane_id", plane_id);

    // plane will both publish and subscribe to this topic
    std::string broadcast_topic = "/airspace_broadcast";

    publisher_ = this->create_publisher<tcas_sim::msg::Pose>(broadcast_topic, 10);

    subscription_ = this->create_subscription<tcas_sim::msg::Pose>(
      broadcast_topic, 10,
      std::bind(&Plane::broadcast_callback, this, std::placeholders::_1)
    );

    timer_ = this->create_wall_timer(
      500ms, std::bind(&Plane::timer_callback, this)
    );

    RCLCPP_INFO(this->get_logger(), "Plane [%s] initialized and listening on %s", plane_id.c_str(), broadcast_topic.c_str());
  }

private:
  rclcpp::TimerBase::SharedPtr timer_;
  rclcpp::Publisher<tcas_sim::msg::Pose>::SharedPtr publisher_;
  rclcpp::Subscription<tcas_sim::msg::Pose>::SharedPtr subscription_;

  std::string plane_id;
  float lat = 10.0;
  float lon = 48.3;
  float alt;
  float heading;
  float velocity;

  void timer_callback() {
    // simulate movement
    lat += 0.01;
    lon += 0.01;

    auto message = tcas_sim::msg::Pose();
    message.id = plane_id;
    message.latitude = lat;
    message.longitude = lon;

    publisher_->publish(message);
  }

  void broadcast_callback(const tcas_sim::msg::Pose::SharedPtr msg) {
    // self-echo filter
    if (msg->id == plane_id) {
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
    RCLCPP_INFO(this->get_logger(), "--> [%s] detected neighbor [%s] at distance %.4f", 
                plane_id.c_str(), msg->id.c_str(), distance);
  }

};

int main(int argc, char * argv[])
{
  rclcpp::init(argc, argv);
  rclcpp::spin(std::make_shared<Plane>());
  rclcpp::shutdown();
  return 0;
}
