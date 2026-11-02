//
// Created by calvinbia on 10/31/26.
//
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
    // update time parameter also needed for extrapolation maybe ?!?!
    void update_position() override {}
    StampedAircraftState stamped_aircraft_state{};
     

    rclcpp::Subscription<interfaces::msg::Adsb>::SharedPtr adsb_subscription;

    void adsb_callback(const interfaces::msg::Adsb::SharedPtr msg) {
        // if message not for me, ignore
        if (msg->node_name != this->get_name()) {
            return;
        }
        
        RCLCPP_INFO(this->get_logger(), "--> received adsb message!");

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

        


        if (std::string(this->get_name()) == "plane_0") {
            RCLCPP_INFO(this->get_logger(), "\nalt: [age: %d] %.2f\nlat: [age: %d] %.4f\nlon: [age: %d] %.4f\ngs: [age: %d] %.2f\nheading: [age: %d] %.2f\nvs: [age: %d] %.2f\n", 
                stamped_aircraft_state.altitude.age_ms(),
                stamped_aircraft_state.altitude.value,
                stamped_aircraft_state.latitude.age_ms(),
                stamped_aircraft_state.latitude.value, 
                stamped_aircraft_state.longitude.age_ms(),
                stamped_aircraft_state.longitude.value, 
                stamped_aircraft_state.velocity.age_ms(), 
                stamped_aircraft_state.velocity.value, 
                stamped_aircraft_state.heading.age_ms(), 
                stamped_aircraft_state.heading.value, 
                stamped_aircraft_state.vertical_speed.age_ms(),
                stamped_aircraft_state.vertical_speed.value
            );
        }
    }

};

int main(int argc, char * argv[])
{
  rclcpp::init(argc, argv);
  rclcpp::spin(std::make_shared<StaticPlane>());
  rclcpp::shutdown();
  return 0;
}