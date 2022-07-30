cmake_minimum_required(VERSION 2.8)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin") 
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/lib")
SET(OpenCV_DIR ~/Documents/courses/Embsys/Class/OpenCV-20220613/installation/OpenCV-master/lib/cmake/opencv4)
project(hw6)
find_package(OpenCV REQUIRED)
find_package(PahoMqttCpp REQUIRED)
find_package(Boost REQUIRED)
set (CMAKE_CXX_STANDARD 11)


include_directories(${OpenCV_INCLUDE_DIRS})
include_directories(${PahoMqttC_INCLUDE_DIRS})
include_directories(${PahoMqttCpp_INCLUDE_DIRS})
include_directories(include/)
include_directories(/usr/include/mysql)
# add_executable(main src/main.cpp src/face_detect.cpp)
# add_executable(sub_main src/main_sub.cpp)
add_library(boost_beast INTERFACE)
target_include_directories(boost_beast 
                            SYSTEM
                            INTERFACE
                            "${Boost_INCLUDE_DIRS}/beast/include")


set(MY_LIBS_1 mysqlclient pthread dl z ssl crypto resolv m rt paho-mqtt3c paho-mqttpp3 boost_beast)

add_executable(main src/tmp_main.cpp src/face_detect.cpp src/http_server.cpp src/mqtt_server.cpp)
target_link_libraries(main ${OpenCV_LIBS} ${MY_LIBS_1})
# target_link_libraries(main ${OpenCV_LIBS} paho-mqtt3c paho-mqttpp3)
# target_link_libraries(sub_main paho-mqtt3c paho-mqttpp3 ${MY_LIBS_1})
