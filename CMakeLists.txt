cmake_minimum_required(VERSION 3.10)
project(semantic_seg LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
if(CMAKE_COMPILER_IS_GNUCXX OR CMAKE_CXX_COMPILER_ID MATCHES "Clang")
  add_compile_options(-Wall -Wextra -Wpedantic)
endif()

find_package(ament_cmake REQUIRED)
find_package(ament_index_cpp REQUIRED)
find_package(rclcpp REQUIRED)
find_package(sensor_msgs REQUIRED)
find_package(std_msgs REQUIRED)
set(_opencv5_dir "/opt/opencv-spacemit/lib/cmake/opencv5")
set(_opencv4_dir "/opt/opencv-spacemit/lib/cmake/opencv4")
if(NOT DEFINED OpenCV_DIR)
  if(EXISTS "${_opencv5_dir}/OpenCVConfig.cmake")
    set(OpenCV_DIR "${_opencv5_dir}")
  else()
    set(OpenCV_DIR "${_opencv4_dir}")
  endif()
endif()
find_package(OpenCV 5 QUIET CONFIG COMPONENTS core imgproc videoio NO_DEFAULT_PATH)
if(NOT OpenCV_FOUND)
  unset(OpenCV_DIR)
  unset(OpenCV_DIR CACHE)
  set(OpenCV_DIR "${_opencv4_dir}")
  find_package(OpenCV 4 REQUIRED CONFIG COMPONENTS core imgproc videoio NO_DEFAULT_PATH)
endif()

set(SPACEMIT_ROBOT_ROOT "${CMAKE_CURRENT_SOURCE_DIR}/../../../..")
set(VISION_SERVICE_INCLUDE_DIR "${SPACEMIT_ROBOT_ROOT}/output/staging/include")
set(VISION_SERVICE_LIBRARY "${SPACEMIT_ROBOT_ROOT}/output/staging/lib/libvision.so")
if(NOT EXISTS "${VISION_SERVICE_INCLUDE_DIR}/vision_service.h")
  message(FATAL_ERROR "vision_service.h not found")
endif()
if(NOT EXISTS "${VISION_SERVICE_LIBRARY}")
  message(FATAL_ERROR "vision library not found. Ensure output/staging is prepared first.")
endif()

add_executable(semantic_seg_node
  src/semantic_seg_node.cpp
  src/image_utils.cpp
  src/segmentation_utils.cpp)
target_include_directories(semantic_seg_node PRIVATE
  ${CMAKE_CURRENT_SOURCE_DIR}/include
  ${VISION_SERVICE_INCLUDE_DIR})
ament_target_dependencies(semantic_seg_node ament_index_cpp rclcpp)
target_link_libraries(semantic_seg_node
  sensor_msgs::sensor_msgs__rosidl_typesupport_cpp
  std_msgs::std_msgs__rosidl_typesupport_cpp
  ${VISION_SERVICE_LIBRARY}
  ${OpenCV_LIBS})

get_filename_component(_opencv_lib_dir "${OpenCV_DIR}/../.." REALPATH)
get_filename_component(_vision_lib_dir "${VISION_SERVICE_LIBRARY}" DIRECTORY)
set_target_properties(semantic_seg_node PROPERTIES
  INSTALL_RPATH "${_opencv_lib_dir};${_vision_lib_dir}"
  BUILD_RPATH "${_opencv_lib_dir};${_vision_lib_dir}")
install(TARGETS semantic_seg_node DESTINATION lib/${PROJECT_NAME})
install(DIRECTORY launch config DESTINATION share/${PROJECT_NAME})
ament_package()
