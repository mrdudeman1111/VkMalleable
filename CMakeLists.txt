CMAKE_MINIMUM_REQUIRED(VERSION 3.18.1)
project("VkMalleable")

find_package(Vulkan REQUIRED)
find_package(glm REQUIRED)
find_package(glfw3 REQUIRED)
find_package(TinyGLTF REQUIRED)

file(GLOB_RECURSE VKMALLSRC
    ${CMAKE_SOURCE_DIR}/src/*.cpp
    ${CMAKE_SOURCE_DIR}/include/*.hpp) # Include the headers for VS and XCode users.

add_library(VkMall SHARED ${VKMALLSRC})

target_include_directories(VkMall PRIVATE ${CMAKE_SOURCE_DIR}/include Vulkan::Headers)
target_link_libraries(VkMall Vulkan::Vulkan glfw glm::glm TinyGLTF::TinyGLTF)
