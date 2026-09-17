cmake_minimum_required(VERSION 3.14)
project(chat)


set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

file(GLOB_RECURSE SOURCES "src/*.cpp")

add_executable(chat ${SOURCES})
target_include_directories(chat PRIVATE include)

option(BUILD_TESTING "Build tests" ON)
if(BUILD_TESTING)
    include(FetchContent)

    FetchContent_Declare(
        googletest
        GIT_REPOSITORY https://github.com/google/googletest.git
        GIT_TAG v1.17.0
    )

    FetchContent_MakeAvailable(googletest)

    enable_testing()
    file(GLOB_RECURSE TEST_SOURCES "test/*.cpp" "src/Core/*.cpp")
    add_executable(chat_test ${TEST_SOURCES})
    target_include_directories(chat_test PRIVATE include)
    target_link_libraries(chat_test PRIVATE GTest::gtest_main)
    include(GoogleTest)
    gtest_discover_tests(chat_test)
endif()