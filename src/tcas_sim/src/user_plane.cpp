#include <algorithm>
#include <chrono>
#include <memory>
#include <cctype>
#include <stdexcept>
#include <string>
#include <cmath>
#include <functional>
#include <map>

#include "rclcpp/rclcpp.hpp"
#include "interfaces/msg/pose.hpp"
#include "tcas_sim/plane.hpp"
#include "interfaces/srv/tele_op.hpp"


using namespace std::chrono_literals;

typedef struct TeleOpCommand {
    double speed;
    double roll;
    double pitch;
    double yaw;
} key_input_t;

class UserPlane : public Plane {
public:
    UserPlane() : Plane() {
        // read start parameters
        aircraft_state.latitude = this->declare_parameter<double>("start_latitude", 48.0);
        aircraft_state.longitude = this->declare_parameter<double>("start_longitude", 11.0);
        aircraft_state.altitude = this->declare_parameter<double>("start_altitude", 1000.0);
        aircraft_state.heading = this->declare_parameter<double>("start_heading", 0.0);
        aircraft_state.velocity = this->declare_parameter<double>("start_velocity", 0.0);
        icao_hex = this->declare_parameter<std::string>("icao_hex", "");
        // make icao upper case
        std::transform(icao_hex.begin(), icao_hex.end(), icao_hex.begin(), ::toupper);
        // subscribe to airspace broadcast
        subscription_ = this->create_subscription<interfaces::msg::Pose>(
            broadcast_topic, 10,
            std::bind(&UserPlane::broadcast_callback, this, std::placeholders::_1)
        );
        // setup service function for receiving keyboard inputs 
        server_ = this->create_service<interfaces::srv::TeleOp>("send_new_keys/" + icao_hex,
            std::bind(&UserPlane::handle_new_keys, this, std::placeholders::_1, std::placeholders::_2)
        );
        // done :)
        RCLCPP_INFO(this->get_logger(), "Plane %s listening for control...", icao_hex.c_str());
        this->is_active = true;
    }
private:
    rclcpp::Service<interfaces::srv::TeleOp>::SharedPtr server_;
    rclcpp::Subscription<interfaces::msg::Pose>::SharedPtr subscription_;
    key_input_t command_{};
    std::chrono::steady_clock::time_point last_update_{};
    std::chrono::steady_clock::time_point last_command_{};

    enum class Advisory { CLEAR, TA, RA };

    struct TrafficTrack {
        Advisory advisory = Advisory::CLEAR;
        std::chrono::steady_clock::time_point last_seen;
    };
    std::map<std::string, TrafficTrack> traffic_;

    Advisory predict_conflict(const interfaces::msg::Pose & other) const {

        double ta_horizon = 45.0;
        double ra_horizon = 25.0;
        double ta_horizontal = 1000.0;
        double ra_horizontal = 500.0;
        double ta_vertical = 300.0;
        double ra_vertical = 150.0;
        double pi = 3.141592653589793;
        double meters_per_degree = 111320.0;


        const double lat_rad = aircraft_state.latitude * pi / 180.0;
        const double north = (other.latitude - aircraft_state.latitude) * meters_per_degree;
        const double east = (other.longitude - aircraft_state.longitude) *
            meters_per_degree * std::cos(lat_rad);
        const double up = other.altitude - aircraft_state.altitude;


        double own_heading = aircraft_state.heading * pi / 180.0;
        double other_heading = other.heading * pi / 180.0;
        double own_horizontal_speed = aircraft_state.velocity *
            std::cos(aircraft_state.pitch * pi / 180.0);
        double other_horizontal_speed = other.velocity *
            std::cos(other.pitch * pi / 180.0);
        double north_speed = other_horizontal_speed * std::cos(other_heading) -
            own_horizontal_speed * std::cos(own_heading);
        double east_speed = other_horizontal_speed * std::sin(other_heading) -
            own_horizontal_speed * std::sin(own_heading);
        double up_speed = other.vertical_speed - aircraft_state.vertical_speed;

        bool ta_detected = false;

        for (double t = 0.0; t <= ta_horizon; t += 0.5) {
            const double horizontal = std::hypot(north + north_speed * t,
                                                  east + east_speed * t);
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
                it = traffic_.erase(it);
            } else {
                ++it;
            }
        }
    }

    void handle_new_keys(const std::shared_ptr<interfaces::srv::TeleOp::Request> request, 
        std::shared_ptr<interfaces::srv::TeleOp::Response> response) {
        // copy values to command
        command_.speed = request->speed;
        command_.roll = request->roll;
        command_.pitch = request->pitch;
        command_.yaw = request->yaw;
        response->ret = 420;
        last_command_ = std::chrono::steady_clock::now();
    }

    aircraft_state_t get_current_pos_state() override {
        const auto now = std::chrono::steady_clock::now();
        const double dt = std::min(
            std::chrono::duration<double>(now - last_update_).count(), 0.2);
        last_update_ = now;
        remove_old_traffic(now);

        if (now - last_command_ > 500ms) {
            command_.speed = 0.0;
            command_.roll = 0.0;
            command_.pitch = 0.0;
            command_.yaw = 0.0;
        }

        double pi = 3.141592653589793;
        double degrees_per_radian = 180.0 / pi;
        double meters_per_degree = 111320.0;


        aircraft_state.pitch = std::clamp(
            aircraft_state.pitch + command_.pitch * degrees_per_radian * dt,
            -80.0, 80.0);
        aircraft_state.roll += command_.roll * degrees_per_radian * dt;
        if (aircraft_state.roll > 180.0) {aircraft_state.roll -= 360.0;}
        if (aircraft_state.roll < -180.0) {aircraft_state.roll += 360.0;}


        aircraft_state.heading = std::fmod(
            aircraft_state.heading - command_.yaw * degrees_per_radian * dt + 360.0,
            360.0);
        aircraft_state.velocity = std::max(0.0, command_.speed);

        double pitch_rad = aircraft_state.pitch * pi / 180.0;
        double heading_rad = aircraft_state.heading * pi / 180.0;
        double distance = aircraft_state.velocity * dt;
        double horizontal_distance = distance * std::cos(pitch_rad);
        aircraft_state.latitude += horizontal_distance * std::cos(heading_rad)
            / meters_per_degree;
        double longitude_scale = std::max(
            0.01, std::abs(std::cos(aircraft_state.latitude * pi / 180.0)));
        aircraft_state.longitude += horizontal_distance * std::sin(heading_rad)
            / (meters_per_degree * longitude_scale);
        aircraft_state.altitude += distance * std::sin(pitch_rad);
        aircraft_state.vertical_speed = aircraft_state.velocity * std::sin(pitch_rad);

        return aircraft_state;
    }

    void broadcast_callback(const interfaces::msg::Pose::SharedPtr msg) {
        // Do not assess our own broadcast or unidentified traffic.
        if (msg->icao_hex.empty() || msg->icao_hex == icao_hex) {
            return;
        }

        const auto now = std::chrono::steady_clock::now();
        const Advisory new_advisory = predict_conflict(*msg);
        auto & track = traffic_[msg->icao_hex];
        track.last_seen = now;


        if (new_advisory != track.advisory) {
            if (new_advisory == Advisory::RA) {
                RCLCPP_WARN(this->get_logger(),
                    "TCAS [%s]: RA - predicted conflict with %s",
                    icao_hex.c_str(), msg->icao_hex.c_str());
            } else if (new_advisory == Advisory::TA) {
                RCLCPP_WARN(this->get_logger(),
                    "TCAS [%s]: TA - traffic %s approaching",
                    icao_hex.c_str(), msg->icao_hex.c_str());
            } else {
                RCLCPP_INFO(this->get_logger(),
                    "TCAS [%s]: CLEAR of %s",
                    icao_hex.c_str(), msg->icao_hex.c_str());
            }
            track.advisory = new_advisory;
        }
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
