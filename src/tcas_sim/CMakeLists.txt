cmake_minimum_required(VERSION 3.8)
project(tcas_sim)

if(CMAKE_COMPILER_IS_GNUCXX OR CMAKE_CXX_COMPILER_ID MATCHES "Clang")
  add_compile_options(-Wall -Wextra -Wpedantic)
endif()

# Pakete finden
find_package(ament_cmake REQUIRED)
find_package(rclcpp REQUIRED)
find_package(interfaces REQUIRED)

# --- Executable: static_plane ---
add_executable(static_plane src/static_plane.cpp src/plane.cpp)
target_include_directories(static_plane PRIVATE include)
target_compile_features(static_plane PRIVATE cxx_std_17)
target_link_libraries(static_plane PUBLIC
  ${interfaces_TARGETS}
  rclcpp::rclcpp
)

# --- Executable: user_plane ---
add_executable(user_plane src/user_plane.cpp src/plane.cpp)
target_include_directories(user_plane PRIVATE include)
target_compile_features(user_plane PRIVATE cxx_std_17)
target_link_libraries(user_plane PUBLIC
  ${interfaces_TARGETS}
  rclcpp::rclcpp
)

# --- Executable: plane_teleop ---
add_executable(plane_teleop src/plane_teleop.cpp)
target_include_directories(plane_teleop PRIVATE include)
target_compile_features(plane_teleop PRIVATE cxx_std_17)
target_link_libraries(plane_teleop PUBLIC
  ${interfaces_TARGETS}
  rclcpp::rclcpp
)

# --- Installation ---

# Launch-Dateien installieren
install(DIRECTORY launch DESTINATION share/${PROJECT_NAME})

# Header-Dateien installieren (falls extern benötigt)
install(
  DIRECTORY include/
  DESTINATION include
)

# Executables installieren
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