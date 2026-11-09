//
// Created by calvinbia on 10/31/26.
//
#include <algorithm>
#include <chrono>
#include <memory>
#include <cctype>
#include <stdexcept>
#include <string>
#include <cmath>
#include <functional>
#include <map>

#include "../include/tcas_sim/plane_node.hpp"
#include "rclcpp/rclcpp.hpp"
#include "interfaces/msg/pose.hpp"
#include "interfaces/msg/resolution.hpp"
#include "tcas_sim/plane_node.hpp"
#include "geometry_msgs/msg/twist.hpp"



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
        aircraft_state.heading = this->declare_parameter<double>("start_heading", 0.0);
        aircraft_state.velocity = this->declare_parameter<double>("start_velocity", 30.0);

        if (aircraft_state.velocity > 0.0) {
            this->has_init_velocity = true;
        }

        icao_hex = this->declare_parameter<std::string>("icao_hex", "");
        std::transform(icao_hex.begin(), icao_hex.end(), icao_hex.begin(),
            [](unsigned char c) { return static_cast<char>(std::toupper(c)); });

        const std::string command_topic = "/user_plane/" + icao_hex + "/cmd_vel";

        resolution_publisher_ = this->create_publisher<interfaces::msg::Resolution>(
            "/tcas/resolutions", 10);

        resolution_subscription_ = this->create_subscription<interfaces::msg::Resolution>("/tcas/resolutions",
                                    10,
                                    std::bind(&UserPlane::resolution_callback,this,std::placeholders::_1)
                                    );

        subscription_ = this->create_subscription<interfaces::msg::Pose>(
                        broadcast_topic, 10,
                        std::bind(&UserPlane::broadcast_callback, this, std::placeholders::_1)
                        );
        this->is_active = true;
        command_subscription_ = this->create_subscription<geometry_msgs::msg::Twist>(
                        command_topic, 10,
                        std::bind(&UserPlane::command_callback, this, std::placeholders::_1)
                        );


        timer_->cancel();
        timer_ = this->create_wall_timer(50ms, std::bind(&UserPlane::timer_callback, this));
        last_update_ = std::chrono::steady_clock::now();
        RCLCPP_INFO(this->get_logger(), "Plane %s listening for control on %s",
            icao_hex.c_str(), command_topic.c_str());
        RCLCPP_INFO(
            this->get_logger(),
            "Plane vel: %f with init vel: %s",
            aircraft_state.velocity,
            this->has_init_velocity ? "true" : "false"
        );
    }
private:
    rclcpp::Subscription<interfaces::msg::Pose>::SharedPtr subscription_;
    rclcpp::Subscription<geometry_msgs::msg::Twist>::SharedPtr command_subscription_;
    rclcpp::Publisher<interfaces::msg::Resolution>::SharedPtr resolution_publisher_;
    rclcpp::Subscription<interfaces::msg::Resolution>::SharedPtr resolution_subscription_;
    geometry_msgs::msg::Twist command_{};
    std::chrono::steady_clock::time_point last_update_{};
    std::chrono::steady_clock::time_point last_command_{};
    bool has_init_velocity = false;

    enum class Advisory { CLEAR, TA, RA, COLLISION };
    enum class Resolution : uint8_t { NONE = 0, CLIMB = 1, DESCEND = 2 };

    struct TrafficTrack {
        Advisory advisory = Advisory::CLEAR;
        Resolution resolution = Resolution::NONE;
        std::chrono::steady_clock::time_point last_seen;
    };
    std::map<std::string, TrafficTrack> traffic_;

    Resolution choose_resolution(const std::string & other_id) const {
        if (icao_hex > other_id) {
            return Resolution::CLIMB;
        }
        else {
            return Resolution::DESCEND;
        }
    }

    void publish_resolution(const std::string & other_id, Resolution resolution)
    {
        interfaces::msg::Resolution msg;

        msg.source_icao = icao_hex;
        msg.target_icao = other_id;

        msg.resolution = static_cast<uint8_t>(resolution);

        resolution_publisher_->publish(msg);
    }
    void resolution_callback(
    const interfaces::msg::Resolution::SharedPtr msg)
    {
        // Only accept instructions addressed to this airplane.
        if (msg->target_icao != icao_hex) {
            return;
        }

        // Ignore messages sent by ourselves.
        if (msg->source_icao == icao_hex) {
            return;
        }

        // Read the received resolution.
        Resolution received =
            static_cast<Resolution>(msg->resolution);

        // Store the resolution for this specific aircraft.
        auto & track = traffic_[msg->source_icao];
        track.last_seen = std::chrono::steady_clock::now();

        // Ignore repeated identical instructions.
        if (track.resolution == received) {
            return;
        }

        track.resolution = received;

        if (received == Resolution::CLIMB) {
            RCLCPP_WARN(
                this->get_logger(),
                "TCAS [%s]: RA with %s - CLIMB",
                icao_hex.c_str(),
                msg->source_icao.c_str());
        }
        else if (received == Resolution::DESCEND) {
            RCLCPP_WARN(
                this->get_logger(),
                "TCAS [%s]: RA with %s - DESCEND",
                icao_hex.c_str(),
                msg->source_icao.c_str());
        }
        else {
            RCLCPP_INFO(
                this->get_logger(),
                "TCAS [%s]: Resolution from %s ended",
                icao_hex.c_str(),
                msg->source_icao.c_str());
        }
    }


    Advisory predict_conflict(const interfaces::msg::Pose & other) const {

        double ta_horizon = 45.0;
        double ra_horizon = 25.0;
        double ta_horizontal = 1000.0;
        double ra_horizontal = 500.0;
        double ta_vertical = 300.0;
        double ra_vertical = 150.0;
        double pi = 3.141592653589793;
        double collision_horizontal = 150.0;
        double collision_vertical = 100.0;

        double meters_per_degree = 111320.0;




        const double lat_rad = aircraft_state.latitude * pi / 180.0;
        const double north = (other.latitude - aircraft_state.latitude) * meters_per_degree;
        const double east = (other.longitude - aircraft_state.longitude) *
            meters_per_degree * std::cos(lat_rad);
        const double up = other.altitude - aircraft_state.altitude;

        if (std::hypot(north, east) < collision_horizontal && std::abs(up) < collision_vertical) {
            return Advisory::COLLISION;
        }

        double own_heading = aircraft_state.heading * pi / 180.0;
        double other_heading = other.heading * pi / 180.0;
        double own_horizontal_speed = aircraft_state.velocity * std::cos(aircraft_state.pitch * pi / 180.0);
        double other_horizontal_speed = other.velocity * std::cos(other.pitch * pi / 180.0);
        double north_speed = other_horizontal_speed * std::cos(other_heading) - own_horizontal_speed * std::cos(own_heading);
        double east_speed = other_horizontal_speed * std::sin(other_heading) - own_horizontal_speed * std::sin(own_heading);
        double up_speed = other.vertical_speed - aircraft_state.vertical_speed;

        bool ta_detected = false;

        for (double t = 0.0; t <= ta_horizon; t += 0.5) {
            const double horizontal = std::hypot(north + north_speed * t, east + east_speed * t);

            const double vertical = std::abs(up + up_speed * t);
            if (t <= ra_horizon && horizontal < ra_horizontal && vertical < ra_vertical) {
                return Advisory::RA;
            }
            if (horizontal < ta_horizontal && vertical < ta_vertical) {
                ta_detected = true;
            }
        }
        return ta_detected ? Advisory::TA : Advisory::CLEAR;
    }

    void remove_old_traffic(std::chrono::steady_clock::time_point now) {
        for (auto it = traffic_.begin(); it != traffic_.end();) {
            if (now - it->second.last_seen > 3s) {
                if (it->second.advisory != Advisory::CLEAR) {
                    RCLCPP_WARN(this->get_logger(),
                        "TCAS [%s]: lost track of %s (no recent broadcasts)",
                        icao_hex.c_str(), it->first.c_str());
                }
                //Could cause issues here
                if (it->second.advisory == Advisory::RA) {
                    publish_resolution(it->first, Resolution::NONE);
                }
                it = traffic_.erase(it);
            } else {
                ++it;
            }
        }
    }

    void command_callback(const geometry_msgs::msg::Twist::SharedPtr msg) {
        command_ = *msg;
        last_command_ = std::chrono::steady_clock::now();
    }

    aircraft_state_t get_current_pos_state() override {
        const auto now = std::chrono::steady_clock::now();
        const double dt = std::min(
            std::chrono::duration<double>(now - last_update_).count(), 0.2);
        last_update_ = now;
        remove_old_traffic(now);

        if (now - last_command_ > 500ms) {
            command_.linear.x = 0.0;
            command_.angular.x = 0.0;
            command_.angular.y = 0.0;
            command_.angular.z = 0.0;
        }

        double pi = 3.141592653589793;
        double degrees_per_radian = 180.0 / pi;
        double meters_per_degree = 111320.0;


        aircraft_state.pitch = std::clamp(
            aircraft_state.pitch + command_.angular.y * degrees_per_radian * dt,
            -80.0, 80.0);
        aircraft_state.roll += command_.angular.x * degrees_per_radian * dt;
        if (aircraft_state.roll > 180.0) {aircraft_state.roll -= 360.0;}
        if (aircraft_state.roll < -180.0) {aircraft_state.roll += 360.0;}


        aircraft_state.heading = std::fmod(
            aircraft_state.heading - command_.angular.z * degrees_per_radian * dt + 360.0,
            360.0);

        if (!this->has_init_velocity || command_.linear.x > 0.0) {
            aircraft_state.velocity = std::max(0.0, command_.linear.x);
        }

        double pitch_rad = aircraft_state.pitch * pi / 180.0;
        double heading_rad = aircraft_state.heading * pi / 180.0;
        double distance = aircraft_state.velocity * dt;
        double horizontal_distance = distance * std::cos(pitch_rad);
        aircraft_state.latitude += horizontal_distance * std::cos(heading_rad) / meters_per_degree;
        double longitude_scale = std::max(0.01, std::abs(std::cos(aircraft_state.latitude * pi / 180.0)));
        aircraft_state.longitude += horizontal_distance * std::sin(heading_rad) / (meters_per_degree * longitude_scale);
        aircraft_state.altitude += distance * std::sin(pitch_rad);
        aircraft_state.vertical_speed = aircraft_state.velocity * std::sin(pitch_rad);

        return aircraft_state;
    }

    void broadcast_callback(const interfaces::msg::Pose::SharedPtr msg){
        if (msg->icao_hex.empty() || msg->icao_hex == icao_hex) {
            return;
        }

        const auto now = std::chrono::steady_clock::now();
        const Advisory new_advisory = predict_conflict(*msg);

        auto & track = traffic_[msg->icao_hex];
        track.last_seen = now;

        // Only react when the advisory changes.
        if (new_advisory == track.advisory) {
            return;
        }

        if (new_advisory == Advisory::RA) {

            // Determine what our own airplane should do.
            Resolution own_resolution =
                choose_resolution(msg->icao_hex);

            // Tell the OTHER airplane to do the opposite.
            Resolution other_resolution =
                (own_resolution == Resolution::CLIMB)
                    ? Resolution::DESCEND
                    : Resolution::CLIMB;

            publish_resolution(msg->icao_hex, other_resolution);

            RCLCPP_WARN(
                this->get_logger(),
                "TCAS [%s]: RA with %s - sent resolution",
                icao_hex.c_str(),
                msg->icao_hex.c_str());
        }
        else {
            // If a previous RA ended, withdraw our instruction.
            if (track.advisory == Advisory::RA) {
                publish_resolution(msg->icao_hex, Resolution::NONE);
            }

            if (new_advisory == Advisory::COLLISION) {
                RCLCPP_ERROR(
                    this->get_logger(),
                    "TCAS [%s]: COLLISION proximity with %s",
                    icao_hex.c_str(),
                    msg->icao_hex.c_str());
            }
            else if (new_advisory == Advisory::TA) {
                RCLCPP_WARN(
                    this->get_logger(),
                    "TCAS [%s]: TA - traffic %s approaching",
                    icao_hex.c_str(),
                    msg->icao_hex.c_str());
            }
            else {
                RCLCPP_INFO(
                    this->get_logger(),
                    "TCAS [%s]: CLEAR of %s",
                    icao_hex.c_str(),
                    msg->icao_hex.c_str());
            }
        }

        track.advisory = new_advisory;
    }

};

int main(int argc, char * argv[])
{
    rclcpp::init(argc, argv);
    try {
        rclcpp::spin(std::make_shared<UserPlane>());
    } catch (const std::exception & error) {
        RCLCPP_ERROR(rclcpp::get_logger("user_plane"), "%s", error.what());
        rclcpp::shutdown();
        return 1;
    }
    rclcpp::shutdown();
    return 0;
}
