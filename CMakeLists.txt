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

if(MSVC)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /Zc:__cplusplus")
    add_compile_options("$<$<C_COMPILER_ID:MSVC>:/utf-8>")
    add_compile_options("$<$<CXX_COMPILER_ID:MSVC>:/utf-8>")
endif()

if (CMAKE_BUILD_TYPE MATCHES "Release")
    include(CheckCXXCompilerFlag)
    CHECK_CXX_COMPILER_FLAG("-march=native" COMPILER_SUPPORT_NATIVE_ARCH)
    if (COMPILER_SUPPORT_NATIVE_ARCH)
        message("Add -march=native flag")
        set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -march=native")
        set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -march=native")
    endif()
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

# jansson
set(JANSSON_EXAMPLES OFF CACHE INTERNAL "")
set(JANSSON_BUILD_DOCS OFF CACHE INTERNAL "")
set(JANSSON_INSTALL OFF CACHE INTERNAL "")
set(JANSSON_WITHOUT_TESTS ON CACHE INTERNAL "")
add_subdirectory(vendor/jansson)
target_link_libraries(run_benchmark jansson)
include_directories("${jansson_BINARY_DIR}/include")

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

# winrt_json
if(MSVC AND ("${CMAKE_SIZEOF_VOID_P}" EQUAL "8"))
    include_external_msproject(winrt_json ${CMAKE_CURRENT_LIST_DIR}/src/winrt_json/winrt_json.vcxproj)
    set(BENCHMARK_HAS_WINRT ON) 
    set_property(TARGET winrt_json PROPERTY VS_PACKAGE_REFERENCES "Microsoft.Windows.CppWinRT.2.0.220531.1")
    set_property(TARGET winrt_json PROPERTY Configuration "Release")

    # These does not work
    # set(CMAKE_VS_NUGET_PACKAGE_RESTORE ON CACHE BOOL "")
    # set(CMAKE_VS_NUGET_PACKAGE_RESTORE ON)
    message("\"${CMAKE_VS_MSBUILD_COMMAND}\" \"${CMAKE_CURRENT_LIST_DIR}/src/winrt_json/winrt_json.vcxproj\" -t:restore -p:configuration=debug -p:platform=x64 -p:RestorePackagesConfig=true")
    add_custom_target(restore_nuget ${CMAKE_VS_MSBUILD_COMMAND} "${CMAKE_CURRENT_LIST_DIR}/src/winrt_json/winrt_json.vcxproj" -t:restore -p:platform=x64 -p:RestorePackagesConfig=true)
    add_custom_command(TARGET restore_nuget COMMAND 
        ${CMAKE_COMMAND} -E copy
        ${CMAKE_CURRENT_LIST_DIR}/src/winrt_json/packages/Microsoft.Windows.CppWinRT.2.0.220531.1/bin/cppwinrt.exe
        ${CMAKE_CURRENT_LIST_DIR}/src/winrt_json/packages/Microsoft.Windows.CppWinRT.2.0.220531.1/build/native/cppwinrt.exe
    )
    add_dependencies(winrt_json restore_nuget)
    add_definitions(-DBENCHMARK_HAS_WINRT=1)
    message(${CMAKE_BUILD_TYPE})
    if(${CMAKE_BUILD_TYPE} MATCHES Debug)
        message(debugbuild)
        target_link_libraries(run_benchmark ${CMAKE_CURRENT_LIST_DIR}/src/winrt_json/x64/Debug/winrt_json.lib)
    elseif(${CMAKE_BUILD_TYPE} MATCHES Release)
        message(releasebuild)
        target_link_libraries(run_benchmark ${CMAKE_CURRENT_LIST_DIR}/src/winrt_json/x64/Release/winrt_json.lib)
    endif()
    add_dependencies(run_benchmark winrt_json)
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
endif()

if(XCODE)
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
endif()

if(NOT XCODE)
    if(BENCHMARK_HAS_SIMDJSON)
        add_definitions(-DBENCHMARK_HAS_SIMDJSON=1)
    endif()
    add_definitions(-DBENCHMARK_DATA_PATH="${CMAKE_CURRENT_BINARY_DIR}")
endif()

