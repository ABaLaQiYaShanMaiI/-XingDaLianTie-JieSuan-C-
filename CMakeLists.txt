cmake_minimum_required(VERSION 3.10)
project(XingDaLianTieJieSuan VERSION 3.0.0 LANGUAGES CXX)

# C++17 standard for filesystem support
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Windows specific: target Win7 SDK compatibility
if(WIN32)
    add_definitions(-DWINVER=0x0601 -D_WIN32_WINNT=0x0601)
    set(CMAKE_WINDOWS_EXPORT_ALL_SYMBOLS ON)
endif()

# Find required packages
find_package(wxWidgets 3.0 REQUIRED COMPONENTS core base)
include(${wxWidgets_USE_FILE})

find_package(PkgConfig)
if(PKG_CONFIG_FOUND)
    pkg_check_modules(POPPLER REQUIRED poppler-cpp)
endif()

# Source files
set(SOURCES
    src/main.cpp
    src/parser.cpp
    src/classifier.cpp
    src/validator.cpp
    src/excel_writer.cpp
    src/config.cpp
    src/cli.cpp
    src/gui/app.cpp
)

set(HEADERS
    src/models.h
    src/parser.h
    src/classifier.h
    src/validator.h
    src/excel_writer.h
    src/config.h
    src/cli.h
    src/exceptions.h
    src/gui/app.h
)

# Main executable
add_executable(${PROJECT_NAME} WIN32 ${SOURCES} ${HEADERS})

# Include directories
target_include_directories(${PROJECT_NAME} PRIVATE
    ${CMAKE_SOURCE_DIR}/src
    ${POPPLER_INCLUDE_DIRS}
)

# Link libraries
target_link_libraries(${PROJECT_NAME}
    ${wxWidgets_LIBRARIES}
    ${POPPLER_LIBRARIES}
)

# Copy classify_rules.yaml next to the executable
configure_file(
    ${CMAKE_SOURCE_DIR}/classify_rules.yaml
    ${CMAKE_BINARY_DIR}/classify_rules.yaml
    COPYONLY
)

# Compiler flags
if(MSVC)
    target_compile_options(${PROJECT_NAME} PRIVATE /W3 /utf-8)
else()
    target_compile_options(${PROJECT_NAME} PRIVATE -Wall -Wextra)
endif()

# Install rules
install(TARGETS ${PROJECT_NAME} DESTINATION bin)
install(FILES classify_rules.yaml DESTINATION bin)