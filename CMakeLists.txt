cmake_minimum_required(VERSION 3.20)

project(BlackHole LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(glfw3 CONFIG REQUIRED)
find_package(glm CONFIG REQUIRED)

add_executable(BlackHole
    main.cpp
    glad/src/gl.c
)
if(MSVC)
    target_compile_options(BlackHole PRIVATE /EHsc)
endif()
target_include_directories(BlackHole PRIVATE
    glad/include
)

target_link_libraries(BlackHole PRIVATE
    glfw
    glm::glm
)