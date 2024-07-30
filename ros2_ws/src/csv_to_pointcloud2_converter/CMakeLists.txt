cmake_minimum_required(VERSION 3.8)
project(csv_to_pointcloud2_converter)

if(CMAKE_COMPILER_IS_GNUCXX OR CMAKE_CXX_COMPILER_ID MATCHES "Clang")
  add_compile_options(-Wall -Wextra -Wpedantic)
endif()

# Find dependencies
find_package(ament_cmake REQUIRED)
find_package(rclcpp REQUIRED)
find_package(sensor_msgs REQUIRED)

# Add executable
add_executable(csv_to_pointcloud2_node src/csv_to_pointcloud2_node.cpp)
target_include_directories(csv_to_pointcloud2_node PUBLIC
  $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
  $<INSTALL_INTERFACE:include>)
target_compile_features(csv_to_pointcloud2_node PUBLIC c_std_99 cxx_std_17)  # Require C++17
ament_target_dependencies(
  csv_to_pointcloud2_node
  "rclcpp"
  "sensor_msgs"
)

# Install
install(TARGETS csv_to_pointcloud2_node
  DESTINATION lib/${PROJECT_NAME})

if(BUILD_TESTING)
  find_package(ament_lint_auto REQUIRED)
  ament_lint_auto_find_test_dependencies()
endif()

ament_package()
