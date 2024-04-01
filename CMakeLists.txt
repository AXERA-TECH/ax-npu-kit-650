cmake_minimum_required(VERSION 3.13 FATAL_ERROR)
project(ax_skel)

set(CMAKE_CXX_STANDARD 11)

if (CHIP_AX650)
    add_definitions(-DCHIP_AX650)
endif()

if (CMAKE_BUILD_TYPE MATCHES Debug)
    set(CMAKE_CXX_FLAGS "-fvisibility=hidden -g -O0")
    add_definitions(-D__AX_SKEL_DEBUG__)
elseif (CMAKE_BUILD_TYPE MATCHES Release)
    set(CMAKE_CXX_FLAGS "-fvisibility=hidden -O2 -fdata-sections -ffunction-sections")
endif()

include(cmake/msp_dependencies.cmake)

include_directories(${MSP_INC_DIR})
link_directories(${MSP_LIB_DIR})

include_directories(src/3rdparty/eigen3)

include_directories(inc)
include_directories(src)

aux_source_directory(src/api SRCS)
aux_source_directory(src/mgr SRCS)
aux_source_directory(src/utils SRCS)
aux_source_directory(src/inference SRCS)
aux_source_directory(src/pipeline SRCS)
aux_source_directory(src/pipeline/hvcfp SRCS)
aux_source_directory(src/tracker SRCS)

file(STRINGS "${CMAKE_SOURCE_DIR}/VERSION" FRAMEWORK_VERSION)
configure_file(${CMAKE_SOURCE_DIR}/src/api/ax_skel_version.h.in ${CMAKE_SOURCE_DIR}/src/api/ax_skel_version.h @ONLY)

add_library(ax_skel SHARED ${SRCS})
target_link_libraries(ax_skel ${MSP_LIBS})
set_target_properties(ax_skel
        PROPERTIES
        PUBLIC_HEADER
        "inc/ax_skel_api.h;inc/ax_skel_type.h;inc/ax_skel_err.h")

if (BUILD_DEMO)
    find_package(OpenCV REQUIRED)
    include_directories(${OpenCV_INCLUDE_DIRS})

    add_executable(ax_skel_version demo/ax_skel_version.cpp)
    target_link_libraries(ax_skel_version ax_skel ${MSP_LIBS})

    add_executable(ax_skel_getcap demo/ax_skel_getcap.cpp)
    target_link_libraries(ax_skel_getcap ax_skel ${MSP_LIBS})

    add_executable(hvcfp_demo demo/hvcfp_demo.cpp)
    target_link_libraries(hvcfp_demo ax_skel ${MSP_LIBS} ${OpenCV_LIBS})

    list(APPEND TEST_PROGRAMS
            ax_skel_version
            ax_skel_getcap
            hvcfp_demo)
endif()

install(TARGETS ax_skel ${TEST_PROGRAMS}
        LIBRARY
            DESTINATION lib
        PUBLIC_HEADER
            DESTINATION include
        RUNTIME
            DESTINATION bin)