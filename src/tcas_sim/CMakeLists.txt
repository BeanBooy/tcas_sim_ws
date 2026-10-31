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
find_package(interfaces REQUIRED)

# Define custom messages
set(msg_files
  msg/Pose.msg
)

rosidl_generate_interfaces(${PROJECT_NAME}
  ${msg_files}
  DEPENDENCIES builtin_interfaces
  ADD_LINTER_TESTS
)

# Modern typesupport linkage for custom messages
rosidl_get_typesupport_target(cpp_typesupport_target "${PROJECT_NAME}" "rosidl_typesupport_cpp")

add_executable(plane_node src/plane_node.cpp)
add_executable(static_plane src/static_plane.cpp)
add_executable(user_plane src/user_plane.cpp)

foreach(target plane_node static_plane user_plane)
  target_include_directories(${target} PRIVATE include)
  target_link_libraries(${target} PUBLIC
    rclcpp::rclcpp
    ${interfaces_TARGETS}
    "${cpp_typesupport_target}"
  )
  add_dependencies(${target} ${PROJECT_NAME}__rosidl_generator_cpp)
endforeach()

# for launch files
install(DIRECTORY launch DESTINATION share/${PROJECT_NAME})

install(TARGETS
  plane_node
  static_plane
  user_plane
  DESTINATION lib/${PROJECT_NAME}
)

if(BUILD_TESTING)
  find_package(ament_lint_auto REQUIRED)
  set(ament_cmake_copyright_FOUND TRUE)
  set(ament_cmake_cpplint_FOUND TRUE)
  ament_lint_auto_find_test_dependencies()
endif()

ament_package()