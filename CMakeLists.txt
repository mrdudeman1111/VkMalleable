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

target_include_directories(VkMall PUBLIC
    $<BUILD_INTERFACE:${CMAKE_SOURCE_DIR}/include>
    $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}/include>
    Vulkan::Headers)

target_link_libraries(VkMall Vulkan::Vulkan glfw glm::glm TinyGLTF::TinyGLTF)

install(TARGETS VkMall EXPORT VkMallTargets
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
    INCLUDES DESTINATION ${CMAKE_INSTALL_INCLUDEDIR})

install(DIRECTORY include/ DESTINATION ${CMAKE_INSTALL_INCLUDEDIR} FILES_MATCHING PATTERN "*.hpp")

add_subdirectory(${CMAKE_SOURCE_DIR}/Examples)
