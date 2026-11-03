cmake_minimum_required(VERSION 3.20)
project(tcas_sim)

if(CMAKE_COMPILER_IS_GNUCXX OR CMAKE_CXX_COMPILER_ID MATCHES "Clang")
  add_compile_options(-Wall -Wextra -Wpedantic)
endif()

# Find dependencies (removed rosidl and std_msgs since messages live in 'interfaces' now)
find_package(ament_cmake REQUIRED)
find_package(rclcpp REQUIRED)
find_package(geometry_msgs REQUIRED)
find_package(interfaces REQUIRED)

include_directories(include)

# Build executables (bundled with base plane_node)
add_executable(static_plane src/static_plane.cpp src/plane_node.cpp)
add_executable(user_plane src/user_plane.cpp src/plane_node.cpp)
add_executable(plane_teleop src/plane_teleop.cpp)

foreach(target static_plane user_plane)
  target_include_directories(${target} PRIVATE include)
  target_link_libraries(${target} PUBLIC
          rclcpp::rclcpp
          ${interfaces_TARGETS}
          "${cpp_typesupport_target}"
  )
endforeach()

# Install launch files

target_link_libraries(user_plane PRIVATE ${geometry_msgs_TARGETS})
target_compile_features(user_plane PRIVATE cxx_std_17)
target_include_directories(plane_teleop PRIVATE include)
target_compile_features(plane_teleop PRIVATE cxx_std_17)
target_link_libraries(plane_teleop PRIVATE
        rclcpp::rclcpp
        ${geometry_msgs_TARGETS}
)

# for launch files
install(DIRECTORY launch DESTINATION share/${PROJECT_NAME})

# Install header files
install(
  DIRECTORY include/
  DESTINATION include
)

# Install executables
install(TARGETS
        static_plane
        user_plane
        plane_teleop
        DESTINATION lib/${PROJECT_NAME}
)

if(BUILD_TESTING)
  find_package(ament_lint_auto REQUIRED)
  set(ament_cmake_copyright_FOUND TRUE)
  set(ament_cmake_cpplint_FOUND TRUE)
  ament_lint_auto_find_test_dependencies()
endif()

ament_package()