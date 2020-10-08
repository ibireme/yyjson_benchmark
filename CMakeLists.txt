cmake_minimum_required (VERSION 3.5)
project (yyjson_benchmark)


# ------------------------------------------------------------------------------
# build type
if(XCODE OR MSVC)
    set(CMAKE_CONFIGURATION_TYPES "Debug;Release" CACHE STRING "" FORCE)
endif()
if(NOT CMAKE_BUILD_TYPE)
    message(STATUS "No build type selected, default to: Release")
    set(CMAKE_BUILD_TYPE Release)
endif()

if(NOT XCODE)
    set(CMAKE_C_STANDARD 11)
    set(CMAKE_CXX_STANDARD 17)
endif()

# ------------------------------------------------------------------------------
# executable
add_executable(run_benchmark "")

file(GLOB SOURCES 
    "src/*.h"
    "src/*.c" 
    "src/*.cpp"
    "src/utils/*.h"
    "src/utils/*.c"
)
if(IOS)
    list(APPEND SOURCES
        "src/main/ios/main.m"    
        "src/main/ios/LaunchScreen.storyboard"
        "src/main/ios/Info.plist"
        "${CMAKE_CURRENT_BINARY_DIR}/data"
    )
    find_library(libUIKit UIKit)
    find_library(libWebKit WebKit)
    target_link_libraries(run_benchmark ${libUIKit} ${libWebKit})
    set_target_properties(run_benchmark PROPERTIES MACOSX_BUNDLE_INFO_PLIST "${CMAKE_CURRENT_SOURCE_DIR}/src/main/ios/Info.plist")
    set_target_properties(run_benchmark PROPERTIES RESOURCE "${CMAKE_CURRENT_BINARY_DIR}/data")
else()
    list(APPEND SOURCES 
        "src/main/console/main.c"
    )
endif()
target_sources(run_benchmark PRIVATE ${SOURCES})
target_include_directories(run_benchmark PRIVATE "src/" "src/utils/")


# ------------------------------------------------------------------------------
# vendor (submodule)

# yyjson
set(YYJSON_DISABLE_COMMENT_READER ON CACHE INTERNAL "")
set(YYJSON_DISABLE_INF_AND_NAN_READER ON CACHE INTERNAL "")
add_subdirectory(vendor/yyjson)
target_link_libraries(run_benchmark yyjson)

# cjson
add_library(cjson STATIC 
    vendor/cJSON/cJSON.h 
    vendor/cJSON/cJSON.c
)
target_include_directories(cjson PUBLIC vendor/cJSON/)
target_link_libraries(run_benchmark cjson)

# rapidjson (header only)
include_directories(vendor/rapidjson/include)

# sajson (header only)
include_directories(vendor/sajson/include)

# simdjson (64bit only, require C++17)
if(("${CMAKE_SIZEOF_VOID_P}" EQUAL "8"))
    set(SIMDJSON_JUST_LIBRARY ON CACHE INTERNAL "")
    set(SIMDJSON_BUILD_STATIC ON CACHE INTERNAL "")
    add_subdirectory(vendor/simdjson)
    target_link_libraries(run_benchmark simdjson)
    set(BENCHMARK_HAS_SIMDJSON ON)
endif()



# ------------------------------------------------------------------------------
# copy test data, config project
file(COPY ${CMAKE_CURRENT_SOURCE_DIR}/data DESTINATION ${CMAKE_CURRENT_BINARY_DIR})

if(MSVC)
    if(CMAKE_CXX_FLAGS MATCHES "/W[0-4]")
        string(REGEX REPLACE "/W[0-4]" "/W4" CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS}")
    else()
        set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /W4")
    endif()
    add_definitions(-D_CRT_SECURE_NO_WARNINGS)
    add_definitions(-DBENCHMARK_DATA_PATH="${CMAKE_CURRENT_BINARY_DIR}")
    if(BENCHMARK_HAS_SIMDJSON)
        add_definitions(-DBENCHMARK_HAS_SIMDJSON=1)
    endif()

elseif(XCODE)
    list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake")
    include(XcodeProperty)

    set(XCODE_TARGET_LIST run_benchmark yyjson cjson)
    
    foreach(ONE_TARGET ${XCODE_TARGET_LIST})
        set_default_xcode_property(${ONE_TARGET})
        set_xcode_deployment_version(${ONE_TARGET} "10.11" "9.0" "9.0" "2.0")
        set_xcode_language_standard(${ONE_TARGET} "gnu11" "gnu++17")
        set_xcode_property(${ONE_TARGET} ONLY_ACTIVE_ARCH[variant=Debug] "YES")
        set_xcode_property(${ONE_TARGET} ONLY_ACTIVE_ARCH[variant=Release] "YES")
        if (IOS)
            set_xcode_property(${ONE_TARGET} SDKROOT "iphoneos")
            set_xcode_property(${ONE_TARGET} IPHONEOS_DEPLOYMENT_TARGET "9.0")
        else()
            set_xcode_property(${ONE_TARGET} SDKROOT "macosx")
        endif()
    endforeach()

    set(XCODE_CFLAGS "")
    if(BENCHMARK_HAS_SIMDJSON)
        set(XCODE_CFLAGS "${XCODE_CFLAGS} -DBENCHMARK_HAS_SIMDJSON=1")
    endif()
    if (NOT IOS)
        set(XCODE_CFLAGS "${XCODE_CFLAGS} -DBENCHMARK_DATA_PATH=\"\\\\\"${CMAKE_CURRENT_BINARY_DIR}\\\\\"\"")
    endif()
    set_xcode_property(run_benchmark OTHER_CFLAGS[variant=Debug] ${XCODE_CFLAGS})
    set_xcode_property(run_benchmark OTHER_CFLAGS[variant=Release] ${XCODE_CFLAGS})
    set_xcode_property(run_benchmark OTHER_CPLUSPLUSFLAGS[variant=Debug] "$(OTHER_CFLAGS)")
    set_xcode_property(run_benchmark OTHER_CPLUSPLUSFLAGS[variant=Release] "$(OTHER_CFLAGS)")

else()
    if(BENCHMARK_HAS_SIMDJSON)
        add_definitions(-DBENCHMARK_HAS_SIMDJSON=1)
    endif()
    add_definitions(-DBENCHMARK_DATA_PATH="${CMAKE_CURRENT_BINARY_DIR}")
endif()

