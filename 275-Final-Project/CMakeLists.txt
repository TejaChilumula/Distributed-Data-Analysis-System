cmake_minimum_required(VERSION 3.5)

project(IngestionNode-Original)

set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(Qt6 COMPONENTS Core Network REQUIRED)


set(SOURCES
    main.cpp
    ingestion_main.cpp
)


set(HEADERS
    ingestion_main.h
)


add_executable(${PROJECT_NAME} ${SOURCES} ${HEADERS})


target_link_libraries(${PROJECT_NAME} Qt6::Core Qt6::Network)


set(CMAKE_AUTOMOC ON)


include_directories(${CMAKE_CURRENT_SOURCE_DIR})


find_package(OpenMP REQUIRED)
if(OpenMP_CXX_FOUND)
    target_link_libraries(${PROJECT_NAME} OpenMP::OpenMP_CXX)
endif()
