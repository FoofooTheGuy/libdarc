cmake_minimum_required(VERSION 3.20)
project(libdarc-test)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

if (NOT MSVC)
	set(CMAKE_CXX_FLAGS "-Wall")
endif()
set(CMAKE_CXX_FLAGS_DEBUG "-g")
set(CMAKE_CXX_FLAGS_RELEASE "-O2")

add_subdirectory(lib EXCLUDE_FROM_ALL)

link_libraries(darc)

file(GLOB MY_SRC
	"${CMAKE_CURRENT_SOURCE_DIR}/sources/*.cpp"
	"${CMAKE_CURRENT_SOURCE_DIR}/include/*.hpp"
	"${CMAKE_CURRENT_SOURCE_DIR}/include/*.hpp"
)

include_directories("${CMAKE_CURRENT_SOURCE_DIR}/include")

add_executable(${PROJECT_NAME} ${MY_SRC})

set_target_properties(${PROJECT_NAME} PROPERTIES FOLDER "Application")
