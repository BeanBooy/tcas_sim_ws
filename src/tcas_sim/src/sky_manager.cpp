#include <chrono>
#include <memory>
#include <string>

#include "rclcpp/rclcpp.hpp"
#include "tcas_sim/msg/pose.hpp"

using namespace std::chrono_literals;


class SkyManager : public rclcpp::Node
{
public:
  SkyManager() : Node("sky_manager") {
    

    publisher_ = this->create_publisher<tcas_sim::msg::Pose>("plane_cmd", 10);
    auto timer_callback =
      [this]() -> void {
        auto message = tcas_sim::msg::Pose();
        //message.latitude = this->lat++;
        //message.longitude = this->lon++;
        //RCLCPP_INFO(this->get_logger(), "Publishing Position -> Lat: %.2f, Lon: %.2f", message.latitude, message.longitude);
        //this->publisher_->publish(message);
      };
    //timer_ = this->create_wall_timer(500ms, timer_callback);
  }

private:
  rclcpp::TimerBase::SharedPtr timer_;
  rclcpp::Publisher<tcas_sim::msg::Pose>::SharedPtr publisher_;

};

int main(int argc, char * argv[])
{
  rclcpp::init(argc, argv);
  rclcpp::spin(std::make_shared<SkyManager>());
  rclcpp::shutdown();
  return 0;
}
