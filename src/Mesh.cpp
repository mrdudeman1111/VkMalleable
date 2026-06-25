#define TINYGLTF_IMPLEMENTATION
#include "Mesh.hpp"

/*
 #include <opencollada/COLLADAFramework/COLLADAFWGeometry.h>
 #include <opencollada/COLLADAFramework/COLLADAFWMesh.h>
 #include <opencollada/COLLADAFramework/COLLADAFWMeshPrimitive.h>
 #include <opencollada/COLLADAFramework/COLLADAFWTriangles.h>
 #include <opencollada/COLLADAFramework/COLLADAFWTypes.h>
 #include <opencollada/COLLADASaxFrameworkLoader/COLLADASaxFWLLoader.h>
 */

namespace Wrappers
{
namespace Mesh
{
bool ExtractVtxComp(tinygltf::Model& Model, tinygltf::Primitive& Prim, std::string CompName, int CompCount, std::vector<float>& OutArr)
{
    auto Iter = Prim.attributes.find(CompName);
    
    if(Iter == Prim.attributes.end()) return false;
    
    const tinygltf::Accessor& Accessor = Model.accessors[Iter->second]; // the component can be found in the accessor, so change compcount to an output variable later.
    const tinygltf::BufferView& BuffView = Model.bufferViews[Accessor.bufferView];
    const tinygltf::Buffer& Buff = Model.buffers[BuffView.buffer];
    
    // uint32_t Count = Accessor.count;
    const uint8_t* pData = &Buff.data[BuffView.byteOffset + Accessor.byteOffset];
    int Type = Accessor.type;
    
#ifdef DEBUG_MODE
    bool ScalarCheck = (Type == TINYGLTF_TYPE_SCALAR && CompCount != 1);
    bool Vec2Check = (Type == TINYGLTF_TYPE_VEC2 && CompCount != 2);
    bool Vec3Check = (Type == TINYGLTF_TYPE_VEC3 && CompCount != 3);
    bool Vec4Check = (Type == TINYGLTF_TYPE_VEC4 && CompCount != 4);
    
    if(ScalarCheck || Vec2Check || Vec3Check || Vec4Check)
    {
        throw std::runtime_error("Comp count does does not match reported component count of the requested component type");
    }
#endif
    
    OutArr.clear();
    OutArr.resize(Accessor.count*CompCount);
    
    for(uint32_t i = 0; i < Accessor.count; i++)
    {
        for(uint32_t x = 0; x < CompCount; x++)
        {
            uint32_t Idx = (i*CompCount)+x;
            float* Comp = (float*)pData;
            Comp += Idx;
            OutArr[Idx] = *Comp;
        }
    }
    
    return true;
}

bool ExtractIdx(tinygltf::Model &Model, tinygltf::Primitive &Prim, std::vector<uint32_t> &Indices)
{
    if (Prim.indices < 0)
        return false;
    
    const tinygltf::Accessor &Accessor = Model.accessors[Prim.indices];
    const tinygltf::BufferView &BuffView = Model.bufferViews[Accessor.bufferView];
    const tinygltf::Buffer &Buffer = Model.buffers[BuffView.buffer];
    
    uint32_t IdxCount = static_cast<uint32_t>(Accessor.count);
    
    const void *pData = &(Buffer.data[BuffView.byteOffset + Accessor.byteOffset]);
    
    Indices.resize(IdxCount);
    
    uint16_t tmp[IdxCount];
    
    if (Accessor.componentType == TINYGLTF_COMPONENT_TYPE_UNSIGNED_SHORT)
    {
        const uint16_t *Dat = reinterpret_cast<const uint16_t *>(pData);
        
        for (uint32_t i = 0; i < IdxCount; i++)
        {
            Indices[i] = (Dat[i]);
        }
    }
    else
    {
        const uint32_t *Dat = reinterpret_cast<const uint32_t *>(pData);
        
        for (uint32_t i = 0; i < IdxCount; i++)
        {
            Indices[i] = Dat[i];
        }
    }
    
    return true;
}

Mesh::Mesh(VkDevice *Device, Memory::Allocator *pAllocator, std::string MeshName) : pAllocator(pAllocator), VulkanObject(Device, MeshName)
{
    gpuMeshBuffer = VK_NULL_HANDLE;
    MeshBufferAlloc = nullptr;
    
    MeshCount++;
    
    if(pLoader == nullptr)
    {
        pLoader = new tinygltf::TinyGLTF();
    }
}

Mesh::~Mesh()
{
    if (gpuMeshBuffer != VK_NULL_HANDLE)
        vkDestroyBuffer(*pDevice, gpuMeshBuffer, nullptr);
    if (MeshBufferAlloc != nullptr)
        delete MeshBufferAlloc;
    
    MeshCount--;
    
    if(MeshCount == 0)
    {
        delete pLoader;
        pLoader = nullptr;
    }
}

bool Mesh::Load(std::string Path)
{
    tinygltf::Model tmpModel;
    std::string Err, Warn;
    
    // This string used to prepend a path to whatever was passed, but now we don't do that (You can import meshes from anywhere, as long as you supply the full OS path in Path variable)
    std::string FullPath = Path;
    
    // load the mesh file
    // if(!MeshLoader.LoadBinaryFromFile(&tmpModel, &Err, &Warn, Path))
    if (!pLoader->LoadBinaryFromFile(&tmpModel, &Err, &Warn, FullPath))
    {
        throw std::runtime_error("Failed to load mesh file " + Path);
    }

    if (tmpModel.scenes.size() == 0)
    {
        std::cout << "Tried to create drawable mesh, but couldn't due to an empty scene.";
        return false;
    }
    
    // process the File from the MeshScene variable provided from tinygltf.
    tinygltf::Scene &MeshScene = tmpModel.scenes[(tmpModel.defaultScene > -1) ? tmpModel.defaultScene : 0];
    
    for (int NodeIdx : MeshScene.nodes)
    {
        // skip the node if there is no mesh
        if (tmpModel.nodes[NodeIdx].mesh < 0)
            continue;

        tinygltf::Mesh &Mesh = tmpModel.meshes[tmpModel.nodes[NodeIdx].mesh];
        
        if (Mesh.primitives.size() == 0)
        {
            continue;
        }
        
        // process the mesh primitives' vertices.
        for (tinygltf::Primitive &Prim : Mesh.primitives)
        {
            std::vector<float> tmpPos;
            std::vector<float> tmpNorm;
            std::vector<float> tmpUV;
            
            if (!ExtractVtxComp(tmpModel, Prim, "POSITION", 3, tmpPos))
            {
                throw std::runtime_error("Failed to extract position data from mesh\n");
            }
            
            if (!ExtractVtxComp(tmpModel, Prim, "NORMAL", 3, tmpNorm))
            {
                throw std::runtime_error("Failed to extract normal data from mesh\n");
            }
            
            if (!ExtractVtxComp(tmpModel, Prim, "TEXCOORD_0", 2, tmpUV))
            {
                throw std::runtime_error("Failed to extract UV data from mesh\n");
            }
            
            for (uint32_t i = 0; i < tmpPos.size() / 3; i++)
            {
                uint32_t Vec3Idx = i * 3; // 3, 6, 9, 12
                uint32_t Vec2Idx = i * 2; // 2, 4, 6, 8
                
                glm::vec3 Pos = {tmpPos[Vec3Idx], tmpPos[Vec3Idx + 1], tmpPos[Vec3Idx + 2]};
                glm::vec3 Norm = {tmpNorm[Vec3Idx], tmpNorm[Vec3Idx + 1], tmpNorm[Vec3Idx + 2]};
                glm::vec2 UV = {tmpUV[Vec2Idx], tmpUV[Vec2Idx+1]};
                
                Vertices.push_back({Pos, Norm, UV});
            }
            
            std::vector<uint32_t> tmpIdx;
            
            if (!ExtractIdx(tmpModel, Prim, tmpIdx))
            {
                throw std::runtime_error("Failed to extract index data from mesh\n");
            }
            
            for (uint32_t i = 0; i < tmpIdx.size(); i++)
            {
                Indices.push_back(tmpIdx[i]);
            }
            
            return true;
        }
    }
}

void Mesh::Update()
{
    if (vtxBindings == nullptr)
    {
        vtxBindings = new VertexBinding();
    }
    if (MeshBufferAlloc == nullptr)
    {
        VkBufferCreateInfo BufferCI{};
        BufferCI.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
        BufferCI.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
        BufferCI.size = (sizeof(Vertex) * Vertices.size()) + (sizeof(uint32_t) * Indices.size());
        BufferCI.usage = VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_INDEX_BUFFER_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT;
        
        if (vkCreateBuffer(*pDevice, &BufferCI, nullptr, &gpuMeshBuffer))
        {
            throw std::runtime_error("Wrappers : " + ObjectName + "::Update() failed to create mesh buffer");
        }
        
        MeshBufferAlloc = pAllocator->Allocate(gpuMeshBuffer, Enums::eLocal);
    }
    
    vtxBindings->SetStride(0, sizeof(Vertex));
    
    // Commit(through transfer) the Mesh data to GPU side buffer
    pAllocator->Copy(&gpuMeshBuffer, Vertices.data(), sizeof(Vertex) * Vertices.size(), 0);
    pAllocator->Copy(&gpuMeshBuffer, Indices.data(), sizeof(uint32_t) * Indices.size(), sizeof(Vertex) * Vertices.size());
}

/* DEPRECATED (The opencollada importer implemented through the OpenCollada librark)
 
 MeshReader* GlobReader = nullptr;
 
 MeshReader* GetReader()
 {
 if(GlobReader == nullptr)
 {
 GlobReader = new MeshReader();
 return GlobReader;
 }
 else
 {
 return GlobReader;
 }
 }
 
 
 void MeshReader::ProccessGeom(const COLLADAFW::Geometry* pGeom)
 {
 if(pGeom->getType() != COLLADAFW::Geometry::GEO_TYPE_MESH)
 {
 // return true, not false. Since we don't want to use nonmesh geometries, and such we have correctly handled this geometry without error.
 return;
 }
 
 COLLADAFW::Mesh* pMesh = (COLLADAFW::Mesh*)pGeom;
 
 struct {
 bool Pos = true;
 bool Col = false;
 bool Norm = false;
 bool UV = false;
 } EnabledAttribs;
 
 const COLLADAFW::FloatArray* Positions = pMesh->getPositions().getFloatValues();
 const COLLADAFW::FloatArray* Normals = pMesh->getNormals().getFloatValues();
 const COLLADAFW::FloatArray* UVs = pMesh->getUVCoords().getFloatValues();
 
 if(UVs->empty())
 {
 outMesh->bHasUV = false;
 }
 else
 {
 outMesh->bHasUV = true;
 EnabledAttribs.UV = true;
 }
 
 if(Normals->empty())
 {
 outMesh->bHasNormals = false;
 }
 else
 {
 outMesh->bHasNormals = true;
 EnabledAttribs.Norm = true;
 }
 
 size_t VertexCount = Positions->getCount()/3;
 
 outMesh->Vertices.resize(VertexCount);
 
 for(uint32_t i = 0; i < VertexCount; i++)
 {
 outMesh->Vertices[i].Position.x = (*Positions) [(i*3)];
 outMesh->Vertices[i].Position.y = (*Positions) [(i*3)+1];
 outMesh->Vertices[i].Position.z = (*Positions) [(i*3)+2];
 
 if(outMesh->bHasNormals)
 {
 outMesh->Vertices[i].Normal.x = (*Normals) [(i*3)];
 outMesh->Vertices[i].Normal.y = (*Normals) [(i*3)+1];
 outMesh->Vertices[i].Normal.z = (*Normals) [(i*3)+2];
 }
 
 if(outMesh->bHasUV)
 {
 outMesh->Vertices[i].UV.x = (*UVs) [(i*2)];
 outMesh->Vertices[i].UV.y = (*UVs) [(i*2)+1];
 }
 }
 
 size_t VertexStride = 0;
 
 if(EnabledAttribs.Pos) { VertexStride += (sizeof(float) * 3); }
 if(EnabledAttribs.Col) { VertexStride += (sizeof(float) * 3); }
 if(EnabledAttribs.Norm) { VertexStride += (sizeof(float) * 3); }
 if(EnabledAttribs.UV) { VertexStride += (sizeof(float) * 2); }
 
 COLLADAFW::MeshPrimitiveArray& Primitives = pMesh->getMeshPrimitives();
 
 size_t PrimitiveCount = Primitives.getCount();
 
 // load indices
 for(uint32_t i = 0; i < PrimitiveCount; i++)
 {
 if(Primitives[i]->getPrimitiveType() != COLLADAFW::MeshPrimitive::TRIANGLES)
 {
 throw std::runtime_error("Wrappers : failed to load a mesh, there was a primitive with non-triangle topology\n");
 }
 
 COLLADAFW::UIntValuesArray& PosIndices =  Primitives[i]->getPositionIndices();
 COLLADAFW::UIntValuesArray& NormIndices = Primitives[i]->getNormalIndices();
 COLLADAFW::UIntValuesArray& UVIndices= Primitives[i]->getUVCoordIndices(0)->getIndices(); // for now only store one set of texcoords. We do not want several texture coordinates for different textures, it's inefficient
 
 // Every vertex needs a position, so we assume that the Positions.size() is == to the number of vertices in the mesh.
 size_t VtxCount = PosIndices.getCount();
 
 assert(NormIndices.getCount() == VtxCount);
 assert(UVIndices.getCount() == VtxCount);
 
 for(uint32_t x = 0; x < VtxCount; x++)
 {
 outMesh->Indices.push_back(PosIndices[x]);
 }
 }
 }
 
 void MeshReader::Load(Mesh* pMesh, std::string MeshPath)
 {
 MeshReader* pReader = GetReader();
 
 outMesh = pMesh;
 
 COLLADASaxFWL::Loader* ColladaLoader = new COLLADASaxFWL::Loader();
 
 if(!ColladaLoader->loadDocument(MeshPath, pReader))
 {
 throw std::runtime_error("Wrappers: failed to read mesh into memory\n");
 }
 
 delete ColladaLoader;
 return;
 }
 */

}
}
