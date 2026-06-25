#include "Pipeline.hpp"

#include "glm/glm.hpp"

#define TINYGLTF_NO_STB_IMAGE
#define TINYGLTF_NO_STB_IMAGE_WRITE
#include "tiny_gltf.h"

/*
 #include "opencollada/COLLADASaxFrameworkLoader/COLLADASaxFWLLoader.h"
 #include "opencollada/COLLADAFramework/COLLADAFWIWriter.h"
 #include <opencollada/COLLADAFramework/COLLADAFWGeometry.h>
 #include <opencollada/COLLADAFramework/COLLADAFWMeshVertexData.h>
 */
#include <vulkan/vulkan_core.h>

namespace Wrappers
{

struct Vertex; //! < Default vertex for 3D rendering.

namespace Mesh
{
struct MeshBuffer; //! < struct containing Vertex information

class MeshInstance; //! < Instance of a mesh (can contain unique textures and uniforms)
class Mesh; //! < Manages Meshbuffer(vertices and indices), Vertex stride, textures, and other mesh specific info.
class MeshReader; //! < Reads Collada files (.dae) into Mesh Objects
}
}



namespace Wrappers
{
struct Vertex
{
    glm::vec3 Position; //! < 3-component float vector containing 3d position
    glm::vec3 Normal; //! < 3-component float vector containing the normal of this vertex (the direction the vertex is facing)
    glm::vec2 UV; //! < 2-component float vector containing the UV coordinate in texture space.
};

namespace Mesh
{
class MeshInstance
{};

class Mesh : VulkanObject
{
    friend MeshReader;
    
public:
    Mesh(VkDevice* Device, Memory::Allocator* pAllocator, std::string MeshName = "Mesh");
    ~Mesh();
    
    inline static uint32_t MeshCount = 0;
    inline static tinygltf::TinyGLTF* pLoader = nullptr;
    
    /*! \brief returns the vertex information.
     *  returns a structure of type VertexBinding that specifies the binding and stride of the This Mesh's vertices.
     *  This structure does need to be passed to pipeline creation.
     */
    inline VertexBinding GetVertexBinding()
    {
        return *vtxBindings;
    }
    
    /* \brief Binds this mesh for drawing.
     *
     *  Binds this mesh's vertices, indices, and descriptor set (defaults to binding 3 usually).
     */
    inline void Bind(VkCommandBuffer* cmdBuffer)
    {
        VkDeviceSize VtxOffset = 0;
        vkCmdBindVertexBuffers(*cmdBuffer, 0, 1, &gpuMeshBuffer, &VtxOffset);
        VkDeviceSize IdxOffset = sizeof(Vertex)*Vertices.size();
        vkCmdBindIndexBuffer(*cmdBuffer, gpuMeshBuffer, IdxOffset, VK_INDEX_TYPE_UINT32);
    }
    
    /* \brief Draws this mesh (call Bind() first).
     *
     *
     */
    inline void Draw(VkCommandBuffer* cmdBuffer) { vkCmdDrawIndexed(*cmdBuffer, (uint32_t)Indices.size(), 1, 0, 0, 0); }
    
    bool Load(std::string MeshPath);
    void Update();
    
    bool isLoading() { return bLoading; }
    
    glm::mat4 Transform;
    
    std::vector<Vertex> Vertices;
    std::vector<uint32_t> Indices;
    
private:
    
    Memory::Allocator* pAllocator;
    
    // gpu buffer
    VkBuffer gpuMeshBuffer;
    Memory::Allocation* MeshBufferAlloc;
    
    VertexBinding* vtxBindings = nullptr;
    
    bool bLoading;
    
    bool bHasUV;
    bool bHasNormals;
};
}
}

