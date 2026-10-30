#include <chrono>
#include <memory>
#include <string>
#include <vector>

#include "rclcpp/rclcpp.hpp"
#include "tcas_sim/msg/pose.hpp"

using namespace std::chrono_literals;


class SkyManager : public rclcpp::Node
{
public:
  SkyManager() : Node("sky_manager") {
    
    this->declare_parameter<std::string>("data_stream", "sim");
    this->get_parameter("data_stream", data_stream);

    if (data_stream != "live") {
      return;
    }

    // TODO connect to ads-b socket

    publisher_ = this->create_publisher<tcas_sim::msg::Pose>("position_overwrite", 10);

    plane_discovery_timer = this->create_wall_timer(
      std::chrono::seconds(1), 
      std::bind(&SkyManager::update_available_planes_registry, this)
    );
  }

private:
  rclcpp::Publisher<tcas_sim::msg::Pose>::SharedPtr publisher_;
  rclcpp::TimerBase::SharedPtr plane_discovery_timer;

  std::string data_stream;
  std::vector<std::string> plane_node_registry;

  void update_available_planes_registry() {
    // fetch all nodes
    auto node_names = this->get_node_names();
    
    std::vector<std::string> current_plane_node_registry;
    std::vector<std::string> new_plane_nodes;  // list of newly discovered nodes
    std::vector<std::string> del_plane_nodes;  // list of prevous available but now deleted nodes

    // filter out plane nodes
    for (const auto& node_name : node_names) {
      if (node_name.rfind("plane_", 0) == 0) {  // names that start with plane_
        current_plane_node_registry.push_back(node_name);

        if(std::find(plane_node_registry.begin(), plane_node_registry.end(), node_name) != plane_node_registry.end()) {
          
        }

      }

      // TODO whatever
      RCLCPP_INFO(this->get_logger(), "Found node: %s", name.c_str());
    }
  }

};

int main(int argc, char * argv[])
{
  rclcpp::init(argc, argv);
  rclcpp::spin(std::make_shared<SkyManager>());
  rclcpp::shutdown();
  return 0;
}
