cmake_minimum_required(VERSION 3.1)

get_filename_component(PROJECT_NAME ${CMAKE_CURRENT_SOURCE_DIR} NAME)
project(${PROJECT_NAME})


set(${PROJECT_NAME}_HEADERS
        ./plugin_polyfit.h
    )

set(${PROJECT_NAME}_SOURCES
    ./main.cpp
        ./plugin_polyfit.cpp
    )

add_executable(${PROJECT_NAME} ${${PROJECT_NAME}_SOURCES} ${${PROJECT_NAME}_HEADERS})

set_target_properties(${PROJECT_NAME} PROPERTIES FOLDER "applications")

target_link_libraries(${PROJECT_NAME} 3rd_imgui easy3d_core easy3d_algo easy3d_viewer window)
