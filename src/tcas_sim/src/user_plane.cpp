//
// Created by calvinbia on 10/31/26.
//
#include <chrono>
#include <memory>
#include <string>
#include <cmath>

#include "rclcpp/rclcpp.hpp"
#include "tcas_sim/plane_node.hpp"

using namespace std::chrono_literals;

class UserPlane : public Plane {
public:
    UserPlane()
    : Plane()
    {
        RCLCPP_INFO(this->get_logger(), "User Plane.");

        this->declare_parameter<std::string>("icao_hex", "unset");
        this->get_parameter("icao_hex", icao_hex);

        subscription_ = this->create_subscription<interfaces::msg::Pose>(
                        broadcast_topic, 10,
                        std::bind(&UserPlane::broadcast_callback, this, std::placeholders::_1)
                        );
        this->is_active = true;
    }
private:
    rclcpp::Subscription<interfaces::msg::Pose>::SharedPtr subscription_;

    aircraft_state_t get_current_pos_state() override {
        return AircraftState{};
    }

    void broadcast_callback(const interfaces::msg::Pose::SharedPtr msg) {
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