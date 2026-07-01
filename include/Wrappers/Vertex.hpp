#pragma once

#include "Base/Enums.hpp"

#include <vector>

#include "glm/glm.hpp"

namespace VkMall
{
	namespace Wrappers
	{
		struct VertexDescription
		{
		public:
		    void AddAttribute(uint32_t AttribBinding, uint32_t AttribLocation, size_t AttribOffset, Enums::eAttributeType Type);

		    inline VkVertexInputAttributeDescription* GetAttributes(uint32_t& AttribCount) { AttribCount = Attributes.size(); return Attributes.data(); }

		private:
		    std::vector<VkVertexInputAttributeDescription> Attributes;

		    uint32_t Offset = 0;
		};

		struct VertexBinding
		{
		public:
		    inline VkVertexInputBindingDescription* GetBindings(uint32_t& Count)
		    {
				Count = Bindings.size();
				return Bindings.data();
		    }

		    void SetStride(uint32_t Binding, uint32_t Stride)
		    {
				if(Binding >= Bindings.size())
				{
					while(Binding >= Bindings.size())
					{
						Bindings.push_back({});
					}
				}

				Bindings[Binding].stride = Stride;
				Bindings[Binding].binding = 0;
				Bindings[Binding].inputRate = VK_VERTEX_INPUT_RATE_VERTEX;
		    }

		private:
		    std::vector<VkVertexInputBindingDescription> Bindings;
		};
		
		/*! \brief An interface class for implementing your own vertices if you want. Eventually, I will add support for filling out fields in this class during Mesh::Load(string).
		 *
		 * You provide the names of your elements in GetElementList(uint32_t&), then the mesh loader will parse through this list for usable names (i.e. Position or UV) then, using the provided names, will fill out this structure using AddElement(string, void*). Once this vertex list is ready for use, the mesh will use Load(void*) to fill out the GPU-side vertex buffer.
		 * During configuration, pipelines will need to know your stride and description. The stride is the size of one vertex, and the description is a list of all the vertices contained here
		 * If you need to know what names are associated with valid vertex attributes, view the gltf 2.0 spec https://registry.khronos.org/glTF/specs/2.0/glTF-2.0.html#meshes
		 * Other than that, you're free to put as many vectors, arrays, or anything else as long as you're implementing those 5 core functions.
		 *
		 */
		class IVertexList
		{
		public:
			IVertexList() {}
			virtual ~IVertexList() {};
			/*! \brief Loads all the vertex data into pDst. Order is important, standard dictates you place each array of data into pDst in the order the attributes are declared in. (This is an interface for a SoA, this load function allows us to use std::vector for heap/dynamically sized SoAs. Make sure you return the TAIL END of pDst after you've transfered everything, the mesh indices come right after in the GPU buffer
			 *
			 * @param pDst A pointer to the destination you want to load the data into.
			 */
			virtual size_t Load(void* pDst) = 0;
			
			/*! \brief Returns the list of element names in this vertex. These element names must match certain names if you want the mesh loader to actually fill these values. Otherwise, this just returns a list of the element names like Position or Normal */
			virtual std::string* GetElementList(uint32_t& ElementCount) = 0;

			/*! \brief Used by the mesh loading interface to load elements into the vertices. */
			virtual inline void AddElement(std::string Name, void* Data) = 0;
			
			/*! \brief Returns the total size of the vertices in bytes. */
			virtual size_t Size() = 0;
			
			/*! \brief Returns the vertex bindings. */
			virtual VertexBinding GetBinding() = 0;

			/*! \brief Returns the vertex Description. */
			virtual VertexDescription GetDescription() = 0;
		};

		class DefaultVertex : public IVertexList
        {
		public:
			DefaultVertex() : PositionIter(0), NormalIter(0), UVIter(0) {};
			~DefaultVertex() { Vertices.clear(); }
			
			size_t Load(void* pDst) override;

			std::string* GetElementList(uint32_t& ElementCount) override;

			inline void AddElement(std::string Name, void* Data) override
			{
				if(Name == "POSITION")
				{
					glm::vec3* pd = (glm::vec3*)Data;

					if(PositionIter < Vertices.size())
					{
						Vertices[PositionIter].Position = *pd;
					}
					else
					{
						Vertices.push_back({});
						Vertices[PositionIter].Position = *pd;
					}
					
					PositionIter++;
				}
				else if(Name == "NORMAL")
				{
					glm::vec3* pd = (glm::vec3*)Data;

					if(NormalIter < Vertices.size())
					{
						Vertices[NormalIter].Normal = *pd;
					}
					else
					{
						Vertices.push_back({});
						Vertices[NormalIter].Normal = *pd;
					}
					
					NormalIter++;
				}
				else if(Name == "TEXCOORD_0")
				{
					glm::vec2* pd = (glm::vec2*)Data;

					if(UVIter < Vertices.size())
					{
						Vertices[UVIter].UV = *pd;
					}
					else
					{
						Vertices.push_back({});
						Vertices[UVIter].UV = *pd;
					}
					
					UVIter++;
				}
			}

			size_t Size() override;
			VertexBinding GetBinding() override;
			VertexDescription GetDescription() override;
			
			struct Vertex
			{
				glm::vec3 Position;
				glm::vec3 Normal;
				glm::vec2 UV;
			};

			std::vector<Vertex> Vertices;
			size_t PositionIter, NormalIter, UVIter;
        };

		/*! \brief A default vertex implementation. If you do not provide your own IVertex type to Mesh during initiation, this is the vertex that will be used. P.S. This implementation is an SoA Vertex type. */
//        class DefaultVertex : public IVertex
//        {
//		public:
//			DefaultVertex() {};
//			~DefaultVertex() { Position.clear(); Normal.clear(); UV.clear(); }
//			
//			size_t Load(void* pDst) override;
//
//			std::string* GetElementList(uint32_t& ElementCount) override;
//
//			inline void AddElement(std::string Name, void* Data) override
//			{
//				if(Name == "Position")
//				{
//					glm::vec3* pd = (glm::vec3*)Data;
//					Position.push_back(*pd);
//				}
//				
//				if(Name == "Normal")
//				{
//					glm::vec3* pd = (glm::vec3*)Data;
//					Normal.push_back(*pd);
//				}
//				
//				if(Name == "UV")
//				{
//					glm::vec2* pd = (glm::vec2*)Data;
//					UV.push_back(*pd);
//				}
//			}
//			
//			size_t GetStride() override;
//			VertexDescription GetDescription() override;
//
//            std::vector<glm::vec3> Position; //! < 3-component float vector containing 3d position
//            std::vector<glm::vec3> Normal; //! < 3-component float vector containing the normal of this vertex (the direction the vertex is facing)
//			  std::vector<glm::vec2> UV; //! < 2-component float vector indicating where vertices/faces place on the Textures we map on this mesh.
//        };
	}
}
