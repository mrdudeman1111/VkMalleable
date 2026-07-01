#define TINYGLTF_IMPLEMENTATION
#define STB_IMAGE_IMPLEMENTATION
#define STB_IMAGE_WRITE_IMPLEMENTATION

#include "Wrappers/Mesh.hpp"

namespace VkMall
{
	namespace Wrappers
	{
		// For now, we only handle 32-bit types, so no short types.
		bool ExtractVtxComp(uint8_t& CompCount, tinygltf::Model& Model, tinygltf::Primitive& Prim, std::string CompName, std::vector<float>& OutArr)
		{
			auto Iter = Prim.attributes.find(CompName);
			
			if(Iter == Prim.attributes.end()) return false;
			
			const tinygltf::Accessor& Accessor = Model.accessors[Iter->second]; // the component can be found in the accessor, so change compcount to an output variable later.
			const tinygltf::BufferView& BuffView = Model.bufferViews[Accessor.bufferView];
			const tinygltf::Buffer& Buff = Model.buffers[BuffView.buffer];
			
			// uint32_t Count = Accessor.count;
			const uint8_t* pData = &Buff.data[BuffView.byteOffset + Accessor.byteOffset];
			int Type = Accessor.type;
			
			switch(Type)
			{
				case TINYGLTF_TYPE_SCALAR:
					CompCount = 1;
					break;
				case TINYGLTF_TYPE_VEC2:
					CompCount = 2;
					break;
				case TINYGLTF_TYPE_VEC3:
					CompCount = 3;
					break;
				case TINYGLTF_TYPE_VEC4:
					CompCount = 4;
					break;
				default:
					throw std::runtime_error("Failed to load attributes due to unknown data type.\n");
					break;
			}
			
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
			
			delete pVertices;
		}
		
		bool Mesh::Load(std::string Path)
		{
			tinygltf::Model tmpModel;
			std::string Err, Warn;
			
			// This string used to prepend a path to whatever was passed, but now we don't do that (You can import meshes from anywhere, as long as you supply the full OS path in Path variable)
			std::string FullPath = Path;
			
			uint8_t CompCount;
			
			// load the mesh file
			// if(!MeshLoader.LoadBinaryFromFile(&tmpModel, &Err, &Warn, Path))
			if (!pLoader->LoadBinaryFromFile(&tmpModel, &Err, &Warn, FullPath))
			{
				std::cout << "Error : " << Err << ", Warn : " << Warn << std::endl;
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
				
				/* This is where we check what types of vertex elements we'll be loading. */
				uint32_t ElementCount;
				std::string* Elements = pVertices->GetElementList(ElementCount);
				
				// process the mesh primitives' vertices.
				for (tinygltf::Primitive &Prim : Mesh.primitives)
				{
					std::vector<float> tmpData;
					
					for(uint32_t i = 0; i < ElementCount; i++)
					{
						if(!ExtractVtxComp(CompCount, tmpModel, Prim, Elements[i], tmpData))
						{
							// We only hit this branch if the element at 'i' is not a GLTF 2.0 attribute name, or it's just not part of the stored vertices, (i.e. not all meshes will have TEXCOORD_2).
							continue;
						}
						
						std::cout << "Mesh Loader : Loaded attribute " << Elements[i] << '\n';
						
						for(uint32_t x = 0; x < tmpData.size()/CompCount; x++)
						{
							uint32_t Idx = x*CompCount;
							
							pVertices->AddElement(Elements[i], &tmpData[Idx]);
						}
					}
					
					
					//                    std::vector<float> tmpPos;
					//                    std::vector<float> tmpNorm;
					//                    std::vector<float> tmpUV;
					//
					//                    if (!ExtractVtxComp(tmpModel, Prim, "POSITION", 3, tmpPos))
					//                    {
					//                        throw std::runtime_error("Failed to extract position data from mesh\n");
					//                    }
					//
					//                    if (!ExtractVtxComp(tmpModel, Prim, "NORMAL", 3, tmpNorm))
					//                    {
					//                        throw std::runtime_error("Failed to extract normal data from mesh\n");
					//                    }
					//
					//                    if (!ExtractVtxComp(tmpModel, Prim, "TEXCOORD_0", 2, tmpUV))
					//                    {
					//                        throw std::runtime_error("Failed to extract UV data from mesh\n");
					//                    }
					//
					//                    for (uint32_t i = 0; i < tmpPos.size() / 3; i++)
					//                    {
					//                        uint32_t Vec3Idx = i * 3; // 3, 6, 9, 12
					//						uint32_t Vec2Idx = i * 2; // 2, 4, 6, 8
					//
					//						glm::vec2 UV = {tmpUV[Vec2Idx], tmpUV[Vec2Idx+1]};
					//                        glm::vec4 Pos = {tmpPos[Vec3Idx], tmpPos[Vec3Idx + 1], tmpPos[Vec3Idx + 2], UV.x};
					//                        glm::vec4 Norm = {tmpNorm[Vec3Idx], tmpNorm[Vec3Idx + 1], tmpNorm[Vec3Idx + 2], UV.y};
					//
					//                        Vertices.push_back({Pos, Norm});
					//                    }
					
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
			if (MeshBufferAlloc == nullptr)
			{
				VkBufferCreateInfo BufferCI{};
				BufferCI.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
				BufferCI.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
				BufferCI.size = (pVertices->Size()) + (sizeof(uint32_t) * Indices.size());
				BufferCI.usage = VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_INDEX_BUFFER_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT;
				
				if (vkCreateBuffer(*pDevice, &BufferCI, nullptr, &gpuMeshBuffer))
				{
					throw std::runtime_error("Wrappers : " + ObjectName + "::Update() failed to create mesh buffer");
				}
				
				MeshBufferAlloc = pAllocator->Allocate(gpuMeshBuffer, Enums::eLocal);
			}
			
			void* pVertexData = malloc(pVertices->Size());
			
			size_t pPos = pVertices->Load(pVertexData);
			
			// Commit(through transfer) the Mesh data to GPU side buffer
			pAllocator->Copy(&gpuMeshBuffer, pVertexData, pVertices->Size(), 0);
			pAllocator->Copy(&gpuMeshBuffer, Indices.data(), sizeof(uint32_t) * Indices.size(), pPos);
			
			free(pVertexData);
		}
	}
}
