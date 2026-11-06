#include "tcas_sim/plane_teleop.hpp"

#include <algorithm>
#include <cerrno>
#include <chrono>
#include <cctype>
#include <fcntl.h>
#include <functional>
#include <memory>
#include <stdexcept>
#include <thread>
#include <unistd.h>

using namespace std::chrono_literals;

namespace
{
constexpr double kPitchRate = 0.20;   // rad/s: nose up is positive
constexpr double kYawRate = 0.25;     // rad/s: left turn is positive
constexpr double kRollRate = 0.30;    // rad/s: right roll is positive
constexpr double kSpeedStep = 5.0;    // m/s per Space or B press
constexpr double kMaxSpeed = 80.0;    // m/s
constexpr double kAcceleration = 10.0;  // m/s^2, gradual speed changes
constexpr auto kKeyTimeout = 200ms;
}  // namespace

PlaneTeleop::PlaneTeleop()
: Node("plane_teleop")
{
  icao_hex_ = this->declare_parameter<std::string>("icao_hex", "");
  std::transform(icao_hex_.begin(), icao_hex_.end(), icao_hex_.begin(),
    [](unsigned char c) { return static_cast<char>(std::toupper(c)); });
  if (icao_hex_.size() != 6 ||
      !std::all_of(icao_hex_.begin(), icao_hex_.end(),
        [](unsigned char c) { return std::isxdigit(c) != 0; })) {
    throw std::invalid_argument("icao_hex must be 6 hexadecimal characters (0-9, A-F)");
  }

  const std::string command_topic = "/user_plane/" + icao_hex_ + "/cmd_vel";
  publisher_ = this->create_publisher<geometry_msgs::msg::Twist>(command_topic, 10);

  setup_terminal();
  last_tick_ = Clock::now();
  timer_ = this->create_wall_timer(20ms, std::bind(&PlaneTeleop::timer_callback, this));

  RCLCPP_INFO(this->get_logger(), "Teleop for ICAO %s on %s (waiting for subscriber)",
    icao_hex_.c_str(), command_topic.c_str());
  RCLCPP_INFO(this->get_logger(),
    "Controls: W/S pitch | A/D yaw | LEFT/RIGHT roll | SPACE faster | B slower | Q quit");
  RCLCPP_INFO(this->get_logger(),
    "Hold or repeat a direction key to keep turning. Speed stays set until changed.");
}

PlaneTeleop::~PlaneTeleop()
{
  restore_terminal();
}

void PlaneTeleop::setup_terminal()
{
  if (!isatty(STDIN_FILENO)) {
    throw std::runtime_error("Keyboard teleop needs an interactive Linux terminal");
  }
  if (tcgetattr(STDIN_FILENO, &original_terminal_) != 0) {
    throw std::runtime_error("Cannot read terminal settings");
  }
  original_flags_ = fcntl(STDIN_FILENO, F_GETFL);
  if (original_flags_ == -1) {
    throw std::runtime_error("Cannot read terminal flags");
  }

  termios raw = original_terminal_;
  raw.c_lflag &= ~(ICANON | ECHO);
  raw.c_cc[VMIN] = 0;
  raw.c_cc[VTIME] = 0;
  if (tcsetattr(STDIN_FILENO, TCSANOW, &raw) != 0) {
    throw std::runtime_error("Cannot enable keyboard input");
  }
  terminal_ready_ = true;
  if (fcntl(STDIN_FILENO, F_SETFL, original_flags_ | O_NONBLOCK) == -1) {
    restore_terminal();
    throw std::runtime_error("Cannot enable nonblocking keyboard input");
  }
}

void PlaneTeleop::restore_terminal()
{
  if (terminal_ready_) {
    tcsetattr(STDIN_FILENO, TCSANOW, &original_terminal_);
    fcntl(STDIN_FILENO, F_SETFL, original_flags_);
    terminal_ready_ = false;
  }
}

void PlaneTeleop::handle_key(char key)
{
  const auto now = Clock::now();
  switch (std::tolower(static_cast<unsigned char>(key))) {
    case 'w': pitch_rate_ = kPitchRate; last_pitch_key_ = now; break;
    case 's': pitch_rate_ = -kPitchRate; last_pitch_key_ = now; break;
    case 'a': yaw_rate_ = kYawRate; last_yaw_key_ = now; break;
    case 'd': yaw_rate_ = -kYawRate; last_yaw_key_ = now; break;
    case ' ': target_speed_ = std::min(kMaxSpeed, target_speed_ + kSpeedStep); break;
    case 'b': target_speed_ = std::max(0.0, std::min(target_speed_, speed_) - kSpeedStep); break;
    case 'q': quit_requested_ = true; break;
    default: break;
  }
}

void PlaneTeleop::read_keyboard()
{
  char key;
  while (!quit_requested_ && read(STDIN_FILENO, &key, 1) == 1) {
    const auto now = Clock::now();
    if (escape_state_ == 1) {
      escape_state_ = (key == '[') ? 2 : 0;
      if (escape_state_ == 0) {
        handle_key(key);
      }
    } else if (escape_state_ == 2) {

      if (key == 'D') {
        roll_rate_ = -kRollRate;
        last_roll_key_ = now;
      } else if (key == 'C') {
        roll_rate_ = kRollRate;
        last_roll_key_ = now;
      }
      escape_state_ = 0;
    } else if (key == '\x1b') {
      escape_state_ = 1;
      escape_started_ = now;
    } else {
      handle_key(key);
    }
  }
}

void PlaneTeleop::timer_callback()
{

  const bool connected = publisher_->get_subscription_count() > 0;
  if (connected != connected_) {
    connected_ = connected;
    if (connected_) {
      RCLCPP_INFO(this->get_logger(), "A command subscriber is available for %s", icao_hex_.c_str());
    } else {
      RCLCPP_WARN(this->get_logger(), "No command subscribers for %s", icao_hex_.c_str());
    }
  }

  read_keyboard();

  if (quit_requested_) {
    publisher_->publish(geometry_msgs::msg::Twist());
    std::this_thread::sleep_for(100ms);
    restore_terminal();
    rclcpp::shutdown();
    return;
  }

  const auto now = Clock::now();
  if (escape_state_ != 0 && now - escape_started_ > 200ms) {
    escape_state_ = 0;
  }
  if (now - last_pitch_key_ > kKeyTimeout) {pitch_rate_ = 0.0;}
  if (now - last_yaw_key_ > kKeyTimeout) {yaw_rate_ = 0.0;}
  if (now - last_roll_key_ > kKeyTimeout) {roll_rate_ = 0.0;}


  const double dt = std::min(std::chrono::duration<double>(now - last_tick_).count(), 0.1);
  last_tick_ = now;
  const double change = kAcceleration * dt;
  if (speed_ < target_speed_) {
    speed_ = std::min(target_speed_, speed_ + change);
  } else {
    speed_ = std::max(target_speed_, speed_ - change);
  }

  geometry_msgs::msg::Twist command;
  command.linear.x = speed_;
  command.angular.x = roll_rate_;
  command.angular.y = pitch_rate_;
  command.angular.z = yaw_rate_;
  publisher_->publish(command);
}

int main(int argc, char ** argv)
{
  rclcpp::init(argc, argv);
  try {
    auto node = std::make_shared<PlaneTeleop>();
    rclcpp::spin(node);
  } catch (const std::exception & error) {
    RCLCPP_ERROR(rclcpp::get_logger("plane_teleop"), "%s", error.what());
    rclcpp::shutdown();
    return 1;
  }
  rclcpp::shutdown();
  return 0;
}
