//
// Created by calvinbia on 10/31/26.
//
#include <algorithm>
#include <chrono>
#include <memory>
#include <string>
#include <cmath>
#include <functional>

#include "rclcpp/rclcpp.hpp"
#include "tcas_sim/msg/pose.hpp"
#include "interfaces/msg/adsb.hpp"
#include "geometry_msgs/msg/twist.hpp"

// Reuse the existing Plane class, without its standalone main().
#define TCAS_SIM_CHILD_PLANE
#include "plane_node.cpp"
#undef TCAS_SIM_CHILD_PLANE

using namespace std::chrono_literals;

class UserPlane : public Plane {
public:
    UserPlane()
    : Plane()
    {
        RCLCPP_INFO(this->get_logger(), "User Plane.");

        aircraft_state.latitude = this->declare_parameter<double>("start_latitude", 48.0);
        aircraft_state.longitude = this->declare_parameter<double>("start_longitude", 11.0);
        aircraft_state.altitude = this->declare_parameter<double>("start_altitude", 1000.0);
        icao_hex = "USER001";

        subscription_ = this->create_subscription<tcas_sim::msg::Pose>(
                        broadcast_topic, 10,
                        std::bind(&UserPlane::broadcast_callback, this, std::placeholders::_1)
                        );
        command_subscription_ = this->create_subscription<geometry_msgs::msg::Twist>(
                        "/user_plane/cmd_vel", 10,
                        std::bind(&UserPlane::command_callback, this, std::placeholders::_1)
                        );


        timer_->cancel();
        timer_ = this->create_wall_timer(50ms, std::bind(&UserPlane::timer_callback, this));
        last_update_ = std::chrono::steady_clock::now();
    }
private:
    rclcpp::Subscription<tcas_sim::msg::Pose>::SharedPtr subscription_;
    rclcpp::Subscription<geometry_msgs::msg::Twist>::SharedPtr command_subscription_;
    geometry_msgs::msg::Twist command_{};
    std::chrono::steady_clock::time_point last_update_{};
    std::chrono::steady_clock::time_point last_command_{};

    void command_callback(const geometry_msgs::msg::Twist::SharedPtr msg) {
        command_ = *msg;
        last_command_ = std::chrono::steady_clock::now();
    }

    void update_position() override {
        const auto now = std::chrono::steady_clock::now();
        const double dt = std::min(
            std::chrono::duration<double>(now - last_update_).count(), 0.2);
        last_update_ = now;


        if (now - last_command_ > 500ms) {
            command_.linear.x = 0.0;
            command_.angular.x = 0.0;
            command_.angular.y = 0.0;
            command_.angular.z = 0.0;
        }

        constexpr double pi = 3.141592653589793;
        constexpr double degrees_per_radian = 180.0 / pi;
        constexpr double meters_per_degree = 111320.0;


        aircraft_state.pitch = std::clamp(
            aircraft_state.pitch + command_.angular.y * degrees_per_radian * dt,
            -80.0, 80.0);
        aircraft_state.roll += command_.angular.x * degrees_per_radian * dt;
        if (aircraft_state.roll > 180.0) {aircraft_state.roll -= 360.0;}
        if (aircraft_state.roll < -180.0) {aircraft_state.roll += 360.0;}


        aircraft_state.heading = std::fmod(
            aircraft_state.heading - command_.angular.z * degrees_per_radian * dt + 360.0,
            360.0);
        aircraft_state.velocity = std::max(0.0, command_.linear.x);

        // Very simple kinematics: fly forward in the direction of the nose.
        const double pitch_rad = aircraft_state.pitch * pi / 180.0;
        const double heading_rad = aircraft_state.heading * pi / 180.0;
        const double distance = aircraft_state.velocity * dt;
        const double horizontal_distance = distance * std::cos(pitch_rad);
        aircraft_state.latitude += horizontal_distance * std::cos(heading_rad)
            / meters_per_degree;
        const double longitude_scale = std::max(
            0.01, std::abs(std::cos(aircraft_state.latitude * pi / 180.0)));
        aircraft_state.longitude += horizontal_distance * std::sin(heading_rad)
            / (meters_per_degree * longitude_scale);
        aircraft_state.altitude += distance * std::sin(pitch_rad);
    }

    void broadcast_callback(const tcas_sim::msg::Pose::SharedPtr msg) {

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
