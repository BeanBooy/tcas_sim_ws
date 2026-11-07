#include <chrono>
#include <memory>
#include <string>
#include <cmath>

#include "rclcpp/rclcpp.hpp"
#include "interfaces/msg/adsb.hpp"
#include "tcas_sim/plane_node.hpp"

using namespace std::chrono_literals;

class StaticPlane : public Plane {
public:
    StaticPlane() : Plane() {
        RCLCPP_INFO(this->get_logger(), "Static Plane");
        this->is_active = false;
        // subscribe to adsb command topic for live adsb overwrites
        adsb_subscription = this->create_subscription<interfaces::msg::Adsb>(
            "/adsb_cmd", 10,
            std::bind(&StaticPlane::adsb_callback, this, std::placeholders::_1)
        );
    }

private:
    // struct filled up by adsb data
    StampedAircraftState stamped_aircraft_state{};
    // subscription to adsb topic
    rclcpp::Subscription<interfaces::msg::Adsb>::SharedPtr adsb_subscription;

    aircraft_state_t get_current_pos_state() override {
        // perform very basic linear extrapolation
        aircraft_state_t extrap;
        // copy basis values
        extrap.heading = stamped_aircraft_state.heading.value;
        extrap.velocity = stamped_aircraft_state.velocity.value;
        extrap.vertical_speed = stamped_aircraft_state.vertical_speed.value;
        // height extrapolation (m + m/s * s = m)
        double dt_alt = stamped_aircraft_state.altitude.age_sec();
        extrap.altitude = stamped_aircraft_state.altitude.value + (stamped_aircraft_state.vertical_speed.value * dt_alt);
        // position extrapolation (dead reckoning)
        double dt_pos = stamped_aircraft_state.latitude.age_sec();
        double distance = stamped_aircraft_state.velocity.value * dt_pos;  // m/s * s = m
        double hdg_rad = stamped_aircraft_state.heading.value * M_PI / 180.0;
        // north east delta in meters
        double delta_north = distance * std::cos(hdg_rad);
        double delta_east = distance * std::sin(hdg_rad);
        // transform meter to degree
        double meters_per_deg_lat = 111320.0;
        double lat_rad = stamped_aircraft_state.latitude.value * M_PI / 180.0;
        double meters_per_deg_lon = 111320.0 * std::cos(lat_rad);
        extrap.latitude = stamped_aircraft_state.latitude.value + (delta_north / meters_per_deg_lat);
        // zero division on the poles
        if (std::abs(meters_per_deg_lon) > .00001) {
            extrap.longitude = stamped_aircraft_state.longitude.value + (delta_east / meters_per_deg_lon);
        } else {
            extrap.longitude = stamped_aircraft_state.longitude.value;
        }
        return extrap;
    }

    void adsb_callback(const interfaces::msg::Adsb::SharedPtr msg) {
        if (msg->node_name != this->get_name()) {
            // if message isn't for me, ignore
            return;
        }

        is_active = msg->is_active;
        icao_hex = msg->icao;
        if (msg->altitude_valid) stamped_aircraft_state.altitude.set(msg->altitude);
        if (msg->ground_speed_valid) stamped_aircraft_state.velocity.set(msg->ground_speed);
        if (msg->track_valid) stamped_aircraft_state.heading.set(msg->track);
        if (msg->coords_valid) {
            stamped_aircraft_state.latitude.set(msg->latitude);
            stamped_aircraft_state.longitude.set(msg->longitude);
        }
        if (msg->vertical_speed_valid) stamped_aircraft_state.vertical_speed.set(msg->vertical_speed);
    }

};

int main(int argc, char * argv[])
{
  rclcpp::init(argc, argv);
  rclcpp::spin(std::make_shared<StaticPlane>());
  rclcpp::shutdown();
  return 0;
}