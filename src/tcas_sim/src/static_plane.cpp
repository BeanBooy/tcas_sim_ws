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
#include "plane_node.cpp"

using namespace std::chrono_literals;

class StaticPlane : public Plane {
public:
    StaticPlane()
    : Plane()
    {
        RCLCPP_INFO(this->get_logger(), "Static Plane");

        subscription_adsb = this->create_subscription<interfaces::msg::Adsb>(
        "/adsb_cmd", 10,
        std::bind(&StaticPlane::adsb_callback, this, std::placeholders::_1)
        );
    }

private:
    void update_position() override {}

    rclcpp::Subscription<interfaces::msg::Adsb>::SharedPtr subscription_adsb;

    void adsb_callback(const interfaces::msg::Adsb::SharedPtr msg) {
        // if message not for me, ignore
        if (msg->node_name != this->get_name()) {
            return;
        }

        // TODO my adsb command receive here!!
        RCLCPP_INFO(this->get_logger(), "--> received adsb message!");

    }

};
