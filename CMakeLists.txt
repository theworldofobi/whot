cmake_minimum_required(VERSION 3.15)
project(WhotGame VERSION 0.0.1 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

if(MSVC)
    add_compile_options(/W4 /WX)
else()
    add_compile_options(-Wall -Wextra -Wpedantic -Werror)
endif()

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)

# Fix RPATH to prioritize system libraries over anaconda3
if(UNIX AND NOT APPLE)
    # Completely disable RPATH handling to suppress warnings about anaconda3 conflicts
    # Libraries will be resolved at runtime via system default paths
    set(CMAKE_SKIP_RPATH TRUE)
    set(CMAKE_SKIP_BUILD_RPATH TRUE)
    set(CMAKE_BUILD_WITH_INSTALL_RPATH FALSE)
    set(CMAKE_INSTALL_RPATH_USE_LINK_PATH FALSE)
    set(CMAKE_INSTALL_RPATH "")
    # Set BUILD_RPATH to empty to prevent conflict detection
    set(CMAKE_BUILD_RPATH "")
endif()

include_directories(${PROJECT_SOURCE_DIR}/include)

file(GLOB_RECURSE CORE_SOURCES "src/Core/*.cpp")
file(GLOB_RECURSE GAME_SOURCES "src/Game/*.cpp")
file(GLOB_RECURSE RULES_SOURCES "src/Rules/*.cpp")
file(GLOB_RECURSE NETWORK_SOURCES "src/Network/*.cpp")
file(GLOB_RECURSE AI_SOURCES "src/AI/*.cpp")
file(GLOB_RECURSE PERSISTENCE_SOURCES "src/Persistence/*.cpp")
file(GLOB_RECURSE UTILS_SOURCES "src/Utils/*.cpp")

set(ALL_SOURCES
    ${CORE_SOURCES}
    ${GAME_SOURCES}
    ${RULES_SOURCES}
    ${NETWORK_SOURCES}
    ${AI_SOURCES}
    ${PERSISTENCE_SOURCES}
    ${UTILS_SOURCES}
    src/Application.cpp
)

add_library(whot_lib STATIC ${ALL_SOURCES})

add_executable(whot_server src/main.cpp)
target_link_libraries(whot_server whot_lib)

find_package(Threads REQUIRED)
target_link_libraries(whot_lib PUBLIC Threads::Threads)

set(WEBSOCKETPP_INCLUDE_DIR "" CACHE PATH "Path to websocketpp headers (must contain config/asio_no_tls.hpp)")

# Prefer a path that has the full websocketpp (including asio_no_tls config)
if(DEFINED ENV{CONDA_PREFIX})
    file(GLOB _ws_pkgs "$ENV{CONDA_PREFIX}/pkgs/websocketpp-*/include")
    if(_ws_pkgs)
        list(GET _ws_pkgs 0 WEBSOCKETPP_INCLUDE_DIR)
    endif()
endif()
if(WEBSOCKETPP_INCLUDE_DIR STREQUAL "")
    find_path(WEBSOCKETPP_INCLUDE_DIR
        NAMES websocketpp/config/asio_no_tls.hpp
        PATHS /usr /usr/local
        PATH_SUFFIXES include
        NO_DEFAULT_PATH
    )
endif()
if(WEBSOCKETPP_INCLUDE_DIR STREQUAL "")
    find_path(WEBSOCKETPP_INCLUDE_DIR
        NAMES websocketpp/config/asio_no_tls.hpp
        PATH_SUFFIXES include
    )
endif()
if(WEBSOCKETPP_INCLUDE_DIR STREQUAL "")
    find_path(WEBSOCKETPP_INCLUDE_DIR
        NAMES websocketpp/config.hpp
        PATHS /usr /usr/local
        PATH_SUFFIXES include
        NO_DEFAULT_PATH
    )
endif()
if(WEBSOCKETPP_INCLUDE_DIR STREQUAL "" AND DEFINED ENV{CONDA_PREFIX})
    set(WEBSOCKETPP_INCLUDE_DIR "$ENV{CONDA_PREFIX}/include")
endif()

# Fallback: fetch websocketpp via FetchContent (e.g. when libwebsocketpp-dev is missing or layout differs)
if(NOT WEBSOCKETPP_INCLUDE_DIR)
    include(FetchContent)
    FetchContent_Declare(
        websocketpp
        GIT_REPOSITORY https://github.com/zaphoyd/websocketpp.git
        GIT_TAG 0.8.2
    )
    FetchContent_GetProperties(websocketpp)
    if(NOT websocketpp_POPULATED)
        FetchContent_Populate(websocketpp)
    endif()
    set(WEBSOCKETPP_INCLUDE_DIR ${websocketpp_SOURCE_DIR})
    message(STATUS "Using websocketpp from FetchContent: ${WEBSOCKETPP_INCLUDE_DIR}")
endif()

if(NOT WEBSOCKETPP_INCLUDE_DIR)
    message(FATAL_ERROR
        "websocketpp headers not found."
    )
else()
    message(STATUS "Using websocketpp headers from: ${WEBSOCKETPP_INCLUDE_DIR}")
    target_include_directories(whot_lib PUBLIC ${WEBSOCKETPP_INCLUDE_DIR})
endif()

find_package(Boost REQUIRED COMPONENTS system)
target_link_libraries(whot_lib PUBLIC Boost::system)

find_package(nlohmann_json REQUIRED)
target_link_libraries(whot_lib PUBLIC nlohmann_json::nlohmann_json)

find_package(SQLite3 REQUIRED)
target_link_libraries(whot_lib PUBLIC SQLite::SQLite3)

option(BUILD_TESTS "Build tests" ON)
if(BUILD_TESTS)
    enable_testing()
    
    # Prefer FetchContent to avoid ABI mismatch with system/conda GTest (e.g. GLIBCXX_3.4.29)
    include(FetchContent)
    FetchContent_Declare(
        googletest
        GIT_REPOSITORY https://github.com/google/googletest.git
        GIT_TAG release-1.11.0
    )
    set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
    FetchContent_MakeAvailable(googletest)
    
    file(GLOB_RECURSE TEST_SOURCES "tests/*.cpp")
    
    add_executable(whot_tests ${TEST_SOURCES})
    target_include_directories(whot_tests PRIVATE ${PROJECT_SOURCE_DIR}/tests)
    # Disable RPATH for test executable to suppress warnings
    if(UNIX AND NOT APPLE)
        set_target_properties(whot_tests PROPERTIES
            SKIP_BUILD_RPATH TRUE
            BUILD_WITH_INSTALL_RPATH FALSE
            INSTALL_RPATH ""
        )
    endif()
    target_link_libraries(whot_tests 
        whot_lib 
        gtest
        gtest_main
    )
    
    include(GoogleTest)
    gtest_discover_tests(whot_tests)
endif()

install(TARGETS whot_server
    RUNTIME DESTINATION bin
)

install(DIRECTORY web/
    DESTINATION share/whot/web
)

option(BUILD_DOCS "Build documentation" OFF)
if(BUILD_DOCS)
    find_package(Doxygen)
    if(DOXYGEN_FOUND)
        set(DOXYGEN_IN ${CMAKE_CURRENT_SOURCE_DIR}/Doxyfile.in)
        set(DOXYGEN_OUT ${CMAKE_CURRENT_BINARY_DIR}/Doxyfile)
        
        configure_file(${DOXYGEN_IN} ${DOXYGEN_OUT} @ONLY)
        
        add_custom_target(docs
            COMMAND ${DOXYGEN_EXECUTABLE} ${DOXYGEN_OUT}
            WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
            COMMENT "Generating API documentation with Doxygen"
            VERBATIM
        )
    endif()
endif()
