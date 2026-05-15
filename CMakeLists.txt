cmake_minimum_required(VERSION 3.10)
project(tatu_web CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

find_package(Drogon CONFIG REQUIRED)

aux_source_directory(controllers CTL_SRC)
aux_source_directory(filters     FILTER_SRC)

add_executable(${PROJECT_NAME} main.cc)

drogon_create_views(${PROJECT_NAME}
                    ${CMAKE_CURRENT_SOURCE_DIR}/views
                    ${CMAKE_CURRENT_BINARY_DIR}
                    FALSE
                    VIEW_SRC)

target_sources(${PROJECT_NAME}
               PRIVATE
               ${CTL_SRC}
               ${FILTER_SRC}
               ${VIEW_SRC})

target_link_libraries(${PROJECT_NAME} PRIVATE Drogon::Drogon)

target_include_directories(${PROJECT_NAME}
                           PRIVATE
                           ${CMAKE_CURRENT_SOURCE_DIR}
                           ${CMAKE_CURRENT_SOURCE_DIR}/controllers
                           ${CMAKE_CURRENT_SOURCE_DIR}/filters
                           ${CMAKE_CURRENT_SOURCE_DIR}/utils
                           ${CMAKE_CURRENT_BINARY_DIR})

if(MSVC)
    target_compile_options(${PROJECT_NAME} PRIVATE /wd26819)
endif()

file(COPY config.json DESTINATION ${CMAKE_CURRENT_BINARY_DIR})
