#pragma once

#include "vulkan/vulkan.h"

#include <GLFW/glfw3.h>

#include <vector>

namespace VkMall
{
    namespace Wrappers
    {
        class Instance
        {
            public:
            Instance();
            ~Instance();

            operator VkInstance() {
                return vkInstance;
            }

            operator VkInstance*() {
                return &vkInstance;
            }

            inline GLFWwindow* GetWindow() { return Window; }

            class Device* CreateDevice(uint32_t ExtensionCount, const char** pExtensions);
            void CreateWindow(uint32_t Width, uint32_t Height, const char* WindowName);

            inline bool ShouldClose() { return glfwWindowShouldClose(Window); }
            inline void PollWindow() { glfwPollEvents(); }

            private:
            VkInstance vkInstance;

            std::vector<const char*> Layers;
            std::vector<const char*> Extensions;

            GLFWwindow* Window;
            VkSurfaceKHR Surface;
        };

    }
}