cmake_minimum_required(VERSION 3.10)
project(tuya_espd VERSION 1.0.0 LANGUAGES C)

# Build options
option(ENABLE_DEBUG "Enable debug build" OFF)

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_C_EXTENSIONS ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

set(CMAKE_C_FLAGS_DEBUG "-g -O0 -DDEBUG")
set(CMAKE_C_FLAGS_RELEASE "-O2 -DNDEBUG")

if(NOT CMAKE_BUILD_TYPE)
    if(ENABLE_DEBUG)
        set(CMAKE_BUILD_TYPE Debug)
    else()
        set(CMAKE_BUILD_TYPE Release)
    endif()
    message(STATUS "Build type: ${CMAKE_BUILD_TYPE}")
endif()

# OpenWrt staging directory
set(STAGING_DIR "$ENV{STAGING_DIR}")

# ─── Find Tuya SDK ─────────────────────────────────────────────
find_package(PkgConfig QUIET)
if(PkgConfig_FOUND)
    pkg_check_modules(TUYA_SDK QUIET libtuya)
endif()

if(NOT TUYA_SDK_FOUND)
    if(EXISTS "${STAGING_DIR}/usr/include/tuyalink_core.h")
        message(STATUS "Found Tuya SDK in staging dir: ${STAGING_DIR}")
        set(TUYA_SDK_FOUND TRUE)
        set(TUYA_SDK_INCLUDE_DIRS ${STAGING_DIR}/usr/include)
        set(TUYA_SDK_LIBRARY_DIRS ${STAGING_DIR}/usr/lib)
        set(TUYA_SDK_LIBRARIES link_core middleware_implementation
                               platform_port utils_modules)
    else()
        message(WARNING "Tuya SDK not found; libraries must be provided externally.")
        set(TUYA_SDK_LIBRARIES link_core middleware_implementation
                               platform_port utils_modules)
    endif()
endif()

# ─── Find system dependencies ──────────────────────────────────
find_package(Threads REQUIRED)

# cJSON
find_path(CJSON_INCLUDE_DIR NAMES cjson/cJSON.h cJSON.h
          PATHS "${STAGING_DIR}/usr/include")
find_library(CJSON_LIBRARY NAMES cjson
             PATHS "${STAGING_DIR}/usr/lib")

# ubus / ubox / blobmsg-json
find_path(UBUS_INCLUDE_DIR NAMES libubus.h
          PATHS "${STAGING_DIR}/usr/include")
find_library(UBUS_LIBRARY NAMES ubus
             PATHS "${STAGING_DIR}/usr/lib")

find_path(UBOX_INCLUDE_DIR NAMES libubox/uloop.h
          PATHS "${STAGING_DIR}/usr/include")
find_library(UBOX_LIBRARY NAMES ubox
             PATHS "${STAGING_DIR}/usr/lib")

find_library(BLOBMSG_JSON_LIBRARY NAMES blobmsg_json
             PATHS "${STAGING_DIR}/usr/lib")

# argp-standalone (musl)
find_library(ARGP_LIBRARY NAMES argp
             PATHS "${STAGING_DIR}/usr/lib")

if(NOT ARGP_LIBRARY)
    message(FATAL_ERROR "libargp not found in STAGING_DIR=${STAGING_DIR}")
endif()
if(NOT CJSON_INCLUDE_DIR OR NOT CJSON_LIBRARY)
    message(FATAL_ERROR "cJSON not found in STAGING_DIR=${STAGING_DIR}")
endif()
if(NOT UBUS_INCLUDE_DIR OR NOT UBUS_LIBRARY OR
   NOT UBOX_INCLUDE_DIR OR NOT UBOX_LIBRARY OR
   NOT BLOBMSG_JSON_LIBRARY)
    message(FATAL_ERROR "ubus/ubox/blobmsg-json not found in STAGING_DIR=${STAGING_DIR}")
endif()

# ─── Source files ──────────────────────────────────────────────
file(GLOB SOURCES CONFIGURE_DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/*.c")

# ─── Executable ───────────────────────────────────────────────
add_executable(tuya_espd ${SOURCES})

target_compile_options(tuya_espd PRIVATE -Wall -Wextra -Werror -Wpedantic)

target_compile_definitions(tuya_espd PRIVATE
    _GNU_SOURCE
    VERSION_MAJOR=${PROJECT_VERSION_MAJOR}
    VERSION_MINOR=${PROJECT_VERSION_MINOR}
    VERSION_PATCH=${PROJECT_VERSION_PATCH}
    VERSION_STRING="${PROJECT_VERSION}"
)

target_include_directories(tuya_espd PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${TUYA_SDK_INCLUDE_DIRS}
    ${CJSON_INCLUDE_DIR}
    ${UBUS_INCLUDE_DIR}
    ${UBOX_INCLUDE_DIR}
)

if(DEFINED TUYA_SDK_LIBRARY_DIRS)
    target_link_directories(tuya_espd PRIVATE ${TUYA_SDK_LIBRARY_DIRS})
endif()
target_link_directories(tuya_espd PRIVATE "${STAGING_DIR}/usr/lib")

target_link_libraries(tuya_espd
    ${TUYA_SDK_LIBRARIES}
    ${CJSON_LIBRARY}
    ${UBUS_LIBRARY}
    ${UBOX_LIBRARY}
    ${BLOBMSG_JSON_LIBRARY}
    ${ARGP_LIBRARY}
    Threads::Threads
)

set_target_properties(tuya_espd PROPERTIES OUTPUT_NAME "tuya_espd")

install(TARGETS tuya_espd DESTINATION bin)

# ─── Configuration summary ────────────────────────────────────
message(STATUS "")
message(STATUS "=== ${PROJECT_NAME} v${PROJECT_VERSION} ===")
message(STATUS "Build type:  ${CMAKE_BUILD_TYPE}")
message(STATUS "C Compiler:  ${CMAKE_C_COMPILER}")
message(STATUS "")
