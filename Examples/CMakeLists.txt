CMAKE_MINIMUM_REQUIRED(VERSION 3.18.1)

file(GLOB SceneTestSrc
    ${CMAKE_CURRENT_SOURCE_DIR}/SceneTest/*.cpp)

add_executable(SceneTest ${SceneTestSrc})

target_link_libraries(SceneTest PUBLIC VkMall)
target_compile_definitions(SceneTest PUBLIC RESDIR="${CMAKE_CURRENT_BINARY_DIR}/")

file(MAKE_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/Shaders")

file(GLOB Resources
    "${CMAKE_CURRENT_SOURCE_DIR}/SceneTest/*.glb"
    "${CMAKE_CURRENT_SOURCE_DIR}/SceneTest/*.png")

file(COPY ${Resources} DESTINATION ${CMAKE_CURRENT_BINARY_DIR})

# Shader compilation 
    file(GLOB_RECURSE ShaderSources
        ${CMAKE_CURRENT_SOURCE_DIR}/Shaders/*.glsl)

    foreach(shader IN LISTS ShaderSources)
        get_filename_component(shaderName ${shader} NAME_WLE)

        set(output "${CMAKE_CURRENT_BINARY_DIR}/Shaders/${shaderName}.spv")
        message("Vulkan::glslc ${shader} -o ${output}")

        add_custom_command(
            COMMAND Vulkan::glslc ${shader} -o ${output}
            OUTPUT ${output}
            COMMENT "Compiling ${shader} to ${output}"
            DEPENDS ${shader}
            )

        list(APPEND shaderBin ${output})

    endforeach()

    add_custom_target(Shaders ALL DEPENDS ${shaderBin})

target_compile_definitions(SceneTest PRIVATE SHADER_DIR="${CMAKE_CURRENT_BINARY_DIR}/Shaders/")