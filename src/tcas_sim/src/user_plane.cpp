//
// Created by calvinbia on 10/31/26.
//
#include <chrono>
#include <memory>
#include <string>
#include <cmath>

#include "rclcpp/rclcpp.hpp"
#include "tcas_sim/msg/pose.hpp"
#include "interfaces/msg/adsb.hpp"
#include "tcas_sim/plane_node.hpp"

using namespace std::chrono_literals;

class UserPlane : public Plane {
public:
    UserPlane()
    : Plane()
    {
        RCLCPP_INFO(this->get_logger(), "User Plane.");

        subscription_ = this->create_subscription<tcas_sim::msg::Pose>(
                        broadcast_topic, 10,
                        std::bind(&UserPlane::broadcast_callback, this, std::placeholders::_1)
                        );
    }
private:
    rclcpp::Subscription<tcas_sim::msg::Pose>::SharedPtr subscription_;

    void broadcast_callback(const tcas_sim::msg::Pose::SharedPtr msg) {
        // self-echo filter
        if (msg->icao_hex == icao_hex) {
            return;
        }

        // calculate distance to the other aircraft
        float dx = msg->latitude - aircraft_state.latitude;
        float dy = msg->longitude - aircraft_state.longitude;
        float distance = std::sqrt(dx * dx + dy * dy);

        // filter out messages out of our radar range (simulates pysical radar range limitations)
        float radar_range = 0.5;
        if (distance > radar_range) {
            return;
        }

        // if we reach here, other plane is within our radar range
        //RCLCPP_INFO(this->get_logger(), "--> detected neighbor [%s] at distance %.4f",  msg->icao_hex.c_str(), distance);
    }

};

int main(int argc, char * argv[])
{
  rclcpp::init(argc, argv);
  rclcpp::spin(std::make_shared<UserPlane>());
  rclcpp::shutdown();
  return 0;
}