cmake_minimum_required(VERSION 3.1)

get_filename_component(PROJECT_NAME ${CMAKE_CURRENT_SOURCE_DIR} NAME)
project(${PROJECT_NAME})


set(${PROJECT_NAME}_HEADERS
    viewer.h
    panel.h
    plugin.h
    )

set(${PROJECT_NAME}_SOURCES
    viewer.cpp
    panel.cpp
    plugin.cpp
    )


add_library(${PROJECT_NAME} STATIC ${${PROJECT_NAME}_SOURCES} ${${PROJECT_NAME}_HEADERS})

set_target_properties(${PROJECT_NAME} PROPERTIES FOLDER "applications")

target_link_libraries(${PROJECT_NAME} 3rd_glew 3rd_glfw 3rd_imgui easy3d_core)
