### VkMall
 VkMalleable (VkMall) is a wrapper library for vulkan that abstracts away most of Vulkan's boilerplate code, like creating instances, picking a physical device, creating a device, etc. Currently, there are wrappers for Instances, Devices, Queues, Command pools/buffers, memory allocation, framebuffers, renderpasses, subpasses, pipelines, and synchronization primitives like fences and semaphores. This library is inteded to work with core vulkan 1.0, but will in the future support version switching via profiles.

 I wrote this library while working on my [VkUI](https://github.com/mrdudeman1111/VkUI) library so I could quickly make test applications. At first, this library was meant to be a small quick collection of wrappers for some of the API features I used frequently, but in the end I wrote much more than I intended. So now I have separated this library from my VkUI library and intend to refactor it as a collection of smaller header-only libraries. I will separate VkMall into smaller header libraries meant for specific tasks like memory allocation, command pools/buffers, Meshes, pipelines, etc.

## Building
 This project uses [Conan2]() and [CMake]() for building.

```
> git clone https://github.com/mrdudeman1111/VkMalleable.git
> cd VkMalleable
> mkdir build && cd build
> conan install .. --build=missing --profile=(Conan Profile)
> cd ..
> cmake --preset=conan-(ConanProfile) -G "(Your preferred build system, I use Unix Makefiles)"

 Your build/project files will be in VkMalleable/build/(ConanProfile)
```

Where "(ConanProfile)" is your conan profile.

## Using

 This library was made using...
***
 [![Vulkan](https://www.vulkan.org/user/themes/vulkan/images/logo/vulkan-logo.svg)](https://www.vulkan.org/)


 [![GLFW](https://www.glfw.org/img/favicon/favicon-196x196.png)](https://www.glfw.org/)

 and [TinyGLTF](https://github.com/syoyo/tinygltf)