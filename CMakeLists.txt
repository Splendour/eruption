cmake_minimum_required(VERSION 3.14)
project(eruption)
set(BINARY ${CMAKE_PROJECT_NAME})

file(GLOB_RECURSE CPP_SRC src/cpp/*.cpp src/cpp/*.h src/cpp/*.hpp)

add_executable(eruption WIN32 ${CPP_SRC})
target_compile_definitions(eruption PRIVATE SHADERS_DIR="${CMAKE_CURRENT_SOURCE_DIR}/src/shaders/")
add_compile_definitions(_HAS_EXCEPTIONS=0)

set_property(TARGET eruption PROPERTY CXX_STANDARD 17)
target_include_directories(eruption PRIVATE src/cpp)

target_include_directories(eruption PRIVATE extern/glfw/include)
add_subdirectory(extern/glfw)
target_link_libraries(eruption PRIVATE glfw)

target_include_directories(eruption PRIVATE extern/spdlog/include)

find_package(Vulkan REQUIRED)
include_directories(${Vulkan_INCLUDE_DIRS})
target_link_libraries(eruption PRIVATE ${Vulkan_LIBRARIES})

# Warnings
if(MSVC)
    target_compile_options(${BINARY} PRIVATE /W4 /WX)
else()
    target_compile_options(${BINARY} PRIVATE -Wall -Wextra -pedantic -Werror)
endif()


# Disable dynamic_cast
if(MSVC)
    target_compile_options(${BINARY} PRIVATE /GR-)
else()
    target_compile_options(${BINARY} PRIVATE -fno-rtti)
endif()


# Disable exceptions
if(MSVC)
    if(CMAKE_CXX_FLAGS MATCHES "/EHsc ")
      string(REPLACE "/EHsc" "/EHs-c-" CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS}")
      message(STATUS "CMAKE_CXX_FLAGS matches /EHsc before end of string
    -- replaced...")
      message(STATUS "")
    endif()
    
    if(CMAKE_CXX_FLAGS MATCHES "/EHsc$")
      string(REPLACE "/EHsc" "/EHs-c-" CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS}")
      message(STATUS "CMAKE_CXX_FLAGS matches /EHsc at end of string --
    replaced...")
      message(STATUS "")
    endif()
endif()

# Disable security check
if(MSVC)
	target_compile_options(${BINARY} PRIVATE /GS-)
endif()

if(MSVC)
    add_definitions(-D_CRT_SECURE_NO_WARNINGS)
endif()

add_definitions(-DDEV_PATH=\"${PROJECT_SOURCE_DIR}\")