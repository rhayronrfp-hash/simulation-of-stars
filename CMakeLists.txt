cmake_minimum_required(VERSION 3.20)

project(stars LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(glfw3 CONFIG REQUIRED)
find_package(glm CONFIG REQUIRED)

add_executable(stars
    main.cpp
    glad/src/gl.c
)
if(MSVC)
    target_compile_options(stars PRIVATE /EHsc)
endif()
target_include_directories(stars PRIVATE
    glad/include
)

target_link_libraries(stars PRIVATE
    glfw
    glm::glm
)