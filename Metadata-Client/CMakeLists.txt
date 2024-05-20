cmake_minimum_required(VERSION 3.5)

# Set the project name
project(MetadataClientProject)

# Set the C++ standard
set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Find the Qt libraries
find_package(Qt6 COMPONENTS Core Network REQUIRED)

# Add the source files
set(SOURCES
    main.cpp
    metadata_client.cpp
)

# Add the header files
set(HEADERS
    metadata_client.h
)

# Add the executable
add_executable(${PROJECT_NAME} ${SOURCES} ${HEADERS})

# Link the Qt libraries
target_link_libraries(${PROJECT_NAME} Qt6::Core Qt6::Network)

# Find and link the OpenMP library
find_package(OpenMP REQUIRED)
if(OpenMP_CXX_FOUND)
    target_link_libraries(${PROJECT_NAME} OpenMP::OpenMP_CXX)
endif()

# Enable automoc for Qt
set(CMAKE_AUTOMOC ON)

# Ensure MOC files are generated
set(CMAKE_INCLUDE_CURRENT_DIR ON)

# Include directories
include_directories(${CMAKE_CURRENT_SOURCE_DIR})
