#include "Wrappers/Instance.hpp"

#include "Wrappers/Device.hpp"

namespace VkMall
{
    namespace Wrappers
    {

        Instance::Instance()
        {
            VkResult Err;
            
            glfwInit();
            
            uint32_t ExtCount;
            const char** Exts = glfwGetRequiredInstanceExtensions(&ExtCount);
            
            for(uint32_t i = 0; i < ExtCount; i++)
            {
                Extensions.push_back(Exts[i]);
            }
            
            Layers.push_back("VK_LAYER_KHRONOS_validation");
            
            Extensions.push_back(VK_KHR_GET_PHYSICAL_DEVICE_PROPERTIES_2_EXTENSION_NAME);
            
        #ifdef __APPLE__
            Extensions.push_back(VK_KHR_PORTABILITY_ENUMERATION_EXTENSION_NAME);
        #endif
            
            VkInstanceCreateInfo InstCI{};
            InstCI.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
        #ifdef __APPLE__
            InstCI.flags = VK_INSTANCE_CREATE_ENUMERATE_PORTABILITY_BIT_KHR;
        #endif
            InstCI.enabledLayerCount = (uint32_t)Layers.size();
            InstCI.ppEnabledLayerNames = Layers.data();
            InstCI.enabledExtensionCount = (uint32_t)Extensions.size();
            InstCI.ppEnabledExtensionNames = Extensions.data();
            
            if((Err = vkCreateInstance(&InstCI, nullptr, &vkInstance)) != VK_SUCCESS)
            {
                throw std::runtime_error("Wrappers: Failed to create instance");
            }
            
            Window = nullptr;
        }

        Instance::~Instance()
        {
            vkDestroySurfaceKHR(vkInstance, Surface, nullptr);
            glfwDestroyWindow(Window);
            glfwTerminate();
            
            vkDestroyInstance(vkInstance, nullptr);
        }

        Device* Instance::CreateDevice(uint32_t ExtensionCount, const char** pExtensions)
        {
            VkPhysicalDevice PhysDev = VK_NULL_HANDLE;
            VkDevice Dev = VK_NULL_HANDLE;
            
            uint32_t PhysDevCount;
            vkEnumeratePhysicalDevices(vkInstance, &PhysDevCount, nullptr);
            std::vector<VkPhysicalDevice> PhysicalDevices(PhysDevCount);
            vkEnumeratePhysicalDevices(vkInstance, &PhysDevCount, PhysicalDevices.data());
            
            for(uint32_t i = 0; i < PhysDevCount; i++)
            {
                VkPhysicalDeviceProperties DevProps;
                vkGetPhysicalDeviceProperties(PhysicalDevices[i], &DevProps);
                
                if(DevProps.deviceType == VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU)
                {
                    PhysDev = PhysicalDevices[i];
                }
            }
            
            if(PhysDev == VK_NULL_HANDLE)
            {
                for(uint32_t i = 0; i < PhysicalDevices.size(); i++)
                {
                    VkPhysicalDeviceProperties DevProps;
                    vkGetPhysicalDeviceProperties(PhysicalDevices[i], &DevProps);
                    
                    if(DevProps.deviceType & VK_PHYSICAL_DEVICE_TYPE_INTEGRATED_GPU | VK_PHYSICAL_DEVICE_TYPE_CPU)
                    {
                        PhysDev = PhysicalDevices[i];
                    }
                }
            }
            
            VkPhysicalDeviceFeatures DevFeat;
            vkGetPhysicalDeviceFeatures(PhysDev, &DevFeat);
            
            Device* Ret = new Device(PhysDev, Surface, ExtensionCount, pExtensions, DevFeat);
            
            return Ret;
        }

        void Instance::CreateWindow(uint32_t Width, uint32_t Height, const char* WindowName)
        {
            if(Window != nullptr)
            {
                return;
            }
            
            glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);
            glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
            Window = glfwCreateWindow(Width, Height, WindowName, NULL, NULL);
            
            glfwCreateWindowSurface(vkInstance, Window, nullptr, &Surface);

        }
    }
}