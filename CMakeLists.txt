cmake_minimum_required(VERSION 3.5)
project(rclcpp_executor_testbench)

#################################################
# compiler settings
#################################################

# Default to C11
if(NOT CMAKE_C_STANDARD)
  set(CMAKE_C_STANDARD 11)
endif()

# Default to C++14
if(NOT CMAKE_CXX_STANDARD)
  set(CMAKE_CXX_STANDARD 14)
endif()

if(CMAKE_COMPILER_IS_GNUCXX OR CMAKE_CXX_COMPILER_ID MATCHES "Clang")
  add_compile_options(-Wall -g -Wextra -Wpedantic)
endif()


# for performance tracing 
# see https://gitlab.com/micro-ROS/ros_tracing/ros2_perf_analysis/tree/feature/simple-analysis/scripts
add_compile_options(-finstrument-functions 
-fplugin=/home/martin/instrument-attribute-gcc-plugin/instrument_attribute.so
-fplugin-arg-instrument_attribute-debug)

set(CMAKE_VERBOSE_MAKEFILE ON)

#################################################
# package dependencies
#################################################

find_package(ament_cmake_ros REQUIRED)
find_package(rcl REQUIRED)
find_package(std_msgs REQUIRED)
find_package(static_executor REQUIRED)

#################################################
# include directories
#################################################
include_directories(include)

#################################################
# targets
#################################################

add_executable(rclcpp_executor_testbench src/sste_exec_bm.cpp)
ament_target_dependencies(rclcpp_executor_testbench rclcpp static_executor std_msgs)

install(TARGETS
  rclcpp_executor_testbench
  DESTINATION lib/${PROJECT_NAME}
)

ament_package()
