    cmake_minimum_required(VERSION 3.16)

    include($ENV{IDF_PATH}/tools/cmake/project.cmake)

    # Set the component directory for the project
    set(EXTRA_COMPONENT_DIRS ${CMAKE_CURRENT_LIST_DIR}/my_lib)
    project(project_webserver)
    target_add_binary_data(${CMAKE_PROJECT_NAME}.elf "html/webserver.html" TEXT)
    target_add_binary_data(${CMAKE_PROJECT_NAME}.elf "html/dashboard.html" TEXT)

