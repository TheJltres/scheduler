cmake_minimum_required(VERSION 3.14)
project(scheduler VERSION 0.1 LANGUAGES CXX)
message(STATUS "${PROJECT_NAME} version: ${PROJECT_VERSION}")

set(EXECUTABLE_OUTPUT_PATH bin)
set(ALLOW_DUPLICATE_CUSTOM_TARGETS ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_MODULES ON)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

include(GNUInstallDirs)
include(CTest)
include(FetchContent)

option(BUILD_SHARED_LIBS "Build shared libraries (.so or .dylib)." ON)
set(CMAKE_BUILD_WITH_INSTALL_RPATH TRUE)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/${CMAKE_INSTALL_LIBDIR})
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/${CMAKE_INSTALL_LIBDIR})
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/${CMAKE_INSTALL_BINDIR})

set(CMAKE_C_FLAGS_RELEASE "-Wall -O3")
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -fvisibility=hidden")

if(UNIX)
    option(BUILD_SHARED_LIBS "Build shared libraries(.so)." ON)
else()
    # only support for static build on Windows.
    set(BUILD_SHARED_LIBS OFF)
endif()

if(BUILD_SHARED_LIBS)
    set(CMAKE_POSITION_INDEPENDENT_CODE ON)
endif()

# Disable CTest targets
set_property(GLOBAL PROPERTY CTEST_TARGETS_ADDED 1)

# Include FetchContent
set(FETCHCONTENT_QUIET off)

# Fetch Argh
FetchContent_Declare(
    argh
    GIT_REPOSITORY https://github.com/adishavit/argh.git
    GIT_TAG        v1.3.2
)
FetchContent_MakeAvailable(argh)

# Fetch mINI
FetchContent_Declare(
  mINI
  GIT_REPOSITORY https://github.com/metayeti/mINI
  GIT_TAG        0.9.15
)
FetchContent_MakeAvailable(mINI)

# Fetch OR-tools library and create the alias ortools::ortools.
FetchContent_Declare(
    ortools
    GIT_REPOSITORY https://github.com/google/or-tools.git
    GIT_TAG        v9.9
)

# Build OR-tools dependencies.
set(BUILD_DEPS ON)
set(BUILD_DOC OFF)
set(BUILD_SAMPLES OFF)
set(BUILD_EXAMPLES OFF)
FetchContent_MakeAvailable(ortools)

# Disable CTest targets
set_property(GLOBAL PROPERTY CTEST_TARGETS_ADDED 1)

# Create a main
add_executable(${PROJECT_NAME} "")
configure_file(SchedulerConfig.h.in SchedulerConfig.h)
target_include_directories(${PROJECT_NAME} PUBLIC "${PROJECT_BINARY_DIR}")

target_sources(${PROJECT_NAME} PRIVATE "src/main.cc")

target_include_directories(${PROJECT_NAME} BEFORE PRIVATE ${PROJECT_SOURCE_DIR}/google/src)
target_compile_features(${PROJECT_NAME} PUBLIC cxx_std_11)
set_target_properties(${PROJECT_NAME} PROPERTIES VERSION ${PROJECT_VERSION})

# Include Argh
target_include_directories(${PROJECT_NAME} PUBLIC "${CMAKE_BINARY_DIR}/_deps/argh-src")

# Include mINI
target_include_directories(${PROJECT_NAME} PUBLIC "${CMAKE_BINARY_DIR}/_deps/mini-src/src/mini")

# Include libastro
target_link_libraries(${PROJECT_NAME} PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/libastro/libastro.a")

# Include ortools
target_link_libraries(${PROJECT_NAME} PUBLIC ortools::ortools)

if(APPLE)
  set_target_properties(${PROJECT_NAME} PROPERTIES INSTALL_RPATH
    "@loader_path/../${CMAKE_INSTALL_LIBDIR};@loader_path")
elseif(UNIX)
  cmake_path(RELATIVE_PATH CMAKE_INSTALL_FULL_LIBDIR
    BASE_DIRECTORY ${CMAKE_INSTALL_FULL_BINDIR}
    OUTPUT_VARIABLE libdir_relative_path)
  set_target_properties(${PROJECT_NAME} PROPERTIES
    INSTALL_RPATH "$ORIGIN/${libdir_relative_path}")
endif()

add_executable(Scheduler::${PROJECT_NAME} ALIAS ${PROJECT_NAME})

# Install
install(
    TARGETS ${PROJECT_NAME}
    EXPORT ${PROJECT_NAME}Targets
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)

message("Scheduler build!")
