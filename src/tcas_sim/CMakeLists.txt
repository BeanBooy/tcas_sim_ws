cmake_minimum_required(VERSION 3.20)
project(tcas_sim)

if(CMAKE_COMPILER_IS_GNUCXX OR CMAKE_CXX_COMPILER_ID MATCHES "Clang")
  add_compile_options(-Wall -Wextra -Wpedantic)
endif()

# Find dependencies
find_package(ament_cmake REQUIRED)
find_package(rclcpp REQUIRED)
find_package(std_msgs REQUIRED)
find_package(rosidl_default_generators REQUIRED)

# Define custom messages
set(msg_files
  msg/Pose.msg
)

rosidl_generate_interfaces(${PROJECT_NAME}
  ${msg_files}
  DEPENDENCIES builtin_interfaces
  ADD_LINTER_TESTS
)


add_executable(plane_node src/plane_node.cpp)
target_link_libraries(plane_node PUBLIC rclcpp::rclcpp)
# Modern typesupport linkage for custom messages
rosidl_get_typesupport_target(cpp_typesupport_target "${PROJECT_NAME}" "rosidl_typesupport_cpp")
target_link_libraries(plane_node PUBLIC "${cpp_typesupport_target}")


add_executable(sky_manager src/sky_manager.cpp)
target_link_libraries(sky_manager PUBLIC rclcpp::rclcpp)
target_link_libraries(sky_manager PUBLIC "${cpp_typesupport_target}")



add_dependencies(plane_node ${PROJECT_NAME}__rosidl_generator_cpp)
add_dependencies(sky_manager ${PROJECT_NAME}__rosidl_generator_cpp)


install(TARGETS
  plane_node
  sky_manager
  DESTINATION lib/${PROJECT_NAME}
)

if(BUILD_TESTING)
  find_package(ament_lint_auto REQUIRED)
  set(ament_cmake_copyright_FOUND TRUE)
  set(ament_cmake_cpplint_FOUND TRUE)
  ament_lint_auto_find_test_dependencies()
endif()

ament_package()