cmake_minimum_required(VERSION 3.15)
project(WhotGame VERSION 0.0.1 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)

include_directories(${PROJECT_SOURCE_DIR}/include)

add_library(whot_lib STATIC
    src/AI/AIPlayer.cpp
    src/AI/DifficultyLevel.cpp
    src/AI/Strategy.cpp
    src/Application.cpp
    src/Core/Card.cpp
    src/Core/Deck.cpp
    src/Core/GameConstants.cpp
    src/Core/Hand.cpp
    src/Core/Player.cpp
    src/Game/GameEngine.cpp
    src/Game/GameState.cpp
    src/Game/RuleEngine.cpp
    src/Game/ScoreCalculator.cpp
    src/Game/TurnManager.cpp
    src/Network/HTTPServer.cpp
    src/Network/MessageProtocol.cpp
    src/Network/SessionManager.cpp
    src/Network/WebSocketServer.cpp
    src/Persistence/Database.cpp
    src/Persistence/GameRepository.cpp
    src/Persistence/PlayerRepository.cpp
    src/Rules/NigerianRules.cpp
    src/Utils/JSONSerializer.cpp
    src/Utils/Logger.cpp
    src/Utils/Random.cpp
    src/Utils/Validation.cpp
)
set_source_files_properties(src/Network/WebSocketServer.cpp PROPERTIES
    COMPILE_OPTIONS "$<$<CXX_COMPILER_ID:GNU,Clang,AppleClang>:-std=c++17>"
)

add_executable(whot_server src/main.cpp)
target_link_libraries(whot_server whot_lib)

if(MSVC)
    target_compile_options(whot_lib PRIVATE /W4)
    target_compile_options(whot_server PRIVATE /W4)
else()
    target_compile_options(whot_lib PRIVATE -Wall -Wextra -Wpedantic)
    target_compile_options(whot_server PRIVATE -Wall -Wextra -Wpedantic)
endif()

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
    if(MSVC)
        target_compile_options(whot_tests PRIVATE /W4)
    else()
        target_compile_options(whot_tests PRIVATE -Wall -Wextra -Wpedantic)
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
