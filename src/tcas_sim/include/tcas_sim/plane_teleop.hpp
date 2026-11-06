#ifndef TCAS_SIM__PLANE_TELEOP_HPP_
#define TCAS_SIM__PLANE_TELEOP_HPP_

#include <chrono>
#include <string>
#include <termios.h>

#include "geometry_msgs/msg/twist.hpp"
#include "rclcpp/rclcpp.hpp"

class PlaneTeleop : public rclcpp::Node
{
public:
  PlaneTeleop();
  ~PlaneTeleop() override;

private:
  void setup_terminal();
  void restore_terminal();
  void read_keyboard();
  void handle_key(char key);
  void timer_callback();

  rclcpp::Publisher<geometry_msgs::msg::Twist>::SharedPtr publisher_;
  std::string icao_hex_;
  bool connected_ = false;
  rclcpp::TimerBase::SharedPtr timer_;

  termios original_terminal_{};
  int original_flags_ = 0;
  bool terminal_ready_ = false;
  bool quit_requested_ = false;
  int escape_state_ = 0;  

  double pitch_rate_ = 0.0;
  double yaw_rate_ = 0.0;
  double roll_rate_ = 0.0;
  double speed_ = 0.0;
  double target_speed_ = 0.0;

  using Clock = std::chrono::steady_clock;
  Clock::time_point last_pitch_key_{};
  Clock::time_point last_yaw_key_{};
  Clock::time_point last_roll_key_{};
  Clock::time_point escape_started_{};
  Clock::time_point last_tick_{};
};

#endif  // TCAS_SIM__PLANE_TELEOP_HPP_
