cmake_minimum_required(VERSION 3.5.1)
project(openauto CXX)

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTORCC ON)
set(base_directory ${CMAKE_CURRENT_SOURCE_DIR})
set(resources_directory ${base_directory}/assets)
set(sources_directory ${base_directory}/src)
set(include_directory ${base_directory}/include)

set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${base_directory}/lib)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${base_directory}/lib)

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${base_directory}/bin)
set(EXECUTABLE_OUTPUT_PATH ${base_directory}/bin)

set(Boost_USE_STATIC_LIBS OFF)
set(Boost_USE_MULTITHREADED ON)
set(Boost_USE_STATIC_RUNTIME OFF)

set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${CMAKE_SOURCE_DIR}/cmake_modules/")
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS_INIT} -Wall -pedantic -fPIC")
set(CMAKE_CXX_FLAGS_DEBUG "-g -O0")
set(CMAKE_CXX_FLAGS_RELEASE "-g -O3")

add_definitions(-DBOOST_ALL_DYN_LINK)

find_package(Boost REQUIRED COMPONENTS system log OPTIONAL_COMPONENTS unit_test_framework)
find_package(libusb-1.0 REQUIRED)
find_package(Qt5 COMPONENTS Multimedia MultimediaWidgets Bluetooth Svg)
find_package(Protobuf REQUIRED)
# Legacy FindProtobuf module only yields -lprotobuf, but protobuf 36 needs
# its Abseil closure at link time ("DSO missing from command line").
# pkg-config exposes the full, correct link line: use it.
find_package(PkgConfig REQUIRED)
pkg_check_modules(PROTOBUF_PC REQUIRED protobuf)
set(PROTOBUF_LIBRARIES ${PROTOBUF_PC_LIBRARIES})
find_package(OpenSSL REQUIRED)
find_package(rtaudio REQUIRED)

# aasdk has no install() rules, it is consumed directly from its source tree.
# Export AASDK_PATH (e.g. $HOME/dev/aa-headunit/aasdk) before running cmake.
if(DEFINED ENV{AASDK_PATH})
    set(AASDK_INCLUDE_DIRS $ENV{AASDK_PATH}/include)
    set(AASDK_PROTO_INCLUDE_DIRS $ENV{AASDK_PATH}/build)
    set(AASDK_LIBRARIES $ENV{AASDK_PATH}/lib/libaasdk.so)
    set(AASDK_PROTO_LIBRARIES $ENV{AASDK_PATH}/lib/libaasdk_proto.so)
else()
    message(FATAL_ERROR "AASDK_PATH env var is not set. Export AASDK_PATH pointing to the built aasdk tree (headers + lib/) before running cmake.")
endif()

if(WIN32)
    set(WINSOCK2_LIBRARIES "ws2_32")
endif(WIN32)

if(RPI3_BUILD)
    add_definitions(-DUSE_OMX -DOMX_SKIP64BIT -DRASPBERRYPI3)
    set(BCM_HOST_LIBRARIES "/opt/vc/lib/libbcm_host.so")
    set(BCM_HOST_INCLUDE_DIRS "/opt/vc/include")
    set(ILCLIENT_INCLUDE_DIRS "/opt/vc/src/hello_pi/libs/ilclient")
    set(ILCLIENT_LIBRARIES "/opt/vc/src/hello_pi/libs/ilclient/libilclient.a;/opt/vc/lib/libvcos.so;/opt/vc/lib/libvcilcs.a;/opt/vc/lib/libvchiq_arm.so")
endif(RPI3_BUILD)

# Bluetooth (Qt5Bluetooth + btservice + local pairing) is optional and OFF by
# default: qt5-connectivity is not in the official Arch repos and BT is not
# needed for wired Android Auto. Re-enable with -DOPENAUTO_ENABLE_BLUETOOTH=ON.
option(OPENAUTO_ENABLE_BLUETOOTH "Build with Bluetooth support (requires Qt5Bluetooth)" OFF)
if(OPENAUTO_ENABLE_BLUETOOTH)
    add_definitions(-DUSE_BLUETOOTH)
endif()

# CAN bridge (socketcan -> AA buttons via InputService, events log-only).
# OFF by default: needs a can interface (vcan0 on PC, can0 in-car) and is
# useless without a car. Re-enable with -DOPENAUTO_ENABLE_CAN=ON.
option(OPENAUTO_ENABLE_CAN "Build with CAN bridge support (socketcan)" OFF)
if(OPENAUTO_ENABLE_CAN)
    add_definitions(-DUSE_CAN)
endif()

include_directories(${CMAKE_CURRENT_BINARY_DIR}
                    ${Qt5Multimedia_INCLUDE_DIRS}
                    ${Qt5MultimediaWidgets_INCLUDE_DIRS}
                    ${Qt5Widgets_INCLUDE_DIRS}
                    ${Qt5Bluetooth_INCLUDE_DIRS}
                    ${Qt5Svg_INCLUDE_DIRS}
                    ${Boost_INCLUDE_DIRS}
                    ${LIBUSB_1_INCLUDE_DIRS}
                    ${PROTOBUF_INCLUDE_DIR}
                    ${OPENSSL_INCLUDE_DIR}
                    ${RTAUDIO_INCLUDE_DIRS}
                    ${AASDK_PROTO_INCLUDE_DIRS}
                    ${AASDK_INCLUDE_DIRS}
                    ${BCM_HOST_INCLUDE_DIRS}
                    ${ILCLIENT_INCLUDE_DIRS}
                    ${include_directory})
								
link_directories(${CMAKE_LIBRARY_OUTPUT_DIRECTORY})

set(common_include_directory ${include_directory}/f1x/openauto/Common)

set(autoapp_sources_directory ${sources_directory}/autoapp)
set(autoapp_include_directory ${include_directory}/f1x/openauto/autoapp)
file(GLOB_RECURSE autoapp_source_files ${autoapp_sources_directory}/*.ui ${autoapp_sources_directory}/*.cpp ${autoapp_include_directory}/*.hpp ${common_include_directory}/*.hpp ${resources_directory}/*.qrc)

if(NOT OPENAUTO_ENABLE_BLUETOOTH)
    list(REMOVE_ITEM autoapp_source_files
        ${autoapp_sources_directory}/Projection/LocalBluetoothDevice.cpp
        ${autoapp_include_directory}/Projection/LocalBluetoothDevice.hpp)
endif()

if(NOT OPENAUTO_ENABLE_CAN)
    list(REMOVE_ITEM autoapp_source_files
        ${autoapp_sources_directory}/Projection/CanBridge.cpp
        ${autoapp_include_directory}/Projection/CanBridge.hpp)
endif()

add_executable(autoapp ${autoapp_source_files})

target_link_libraries(autoapp
                        ${Boost_LIBRARIES}
                        ${Qt5Multimedia_LIBRARIES}
                        ${Qt5MultimediaWidgets_LIBRARIES}
                        ${Qt5Bluetooth_LIBRARIES}
                        ${Qt5Svg_LIBRARIES}
                        ${LIBUSB_1_LIBRARIES}
                        ${PROTOBUF_LIBRARIES}
                        ${BCM_HOST_LIBRARIES}
                        ${ILCLIENT_LIBRARIES}
                        ${WINSOCK2_LIBRARIES}
                        ${RTAUDIO_LIBRARIES}
                        ${AASDK_PROTO_LIBRARIES}
                        ${AASDK_LIBRARIES})

if(OPENAUTO_ENABLE_BLUETOOTH)
set(btservice_sources_directory ${sources_directory}/btservice)
set(btservice_include_directory ${include_directory}/f1x/openauto/btservice)
file(GLOB_RECURSE btservice_source_files ${btservice_sources_directory}/*.cpp ${btservice_include_directory}/*.hpp ${common_include_directory}/*.hpp)

add_executable(btservice ${btservice_source_files})

target_link_libraries(btservice
                        ${Boost_LIBRARIES}
                        ${Qt5Bluetooth_LIBRARIES}
                        ${Qt5MultimediaWidgets_LIBRARIES}
                        ${PROTOBUF_LIBRARIES}
                        ${AASDK_PROTO_LIBRARIES})
endif()
