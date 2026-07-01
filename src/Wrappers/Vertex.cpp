#include "Wrappers/Vertex.hpp"

#include <stdexcept>

namespace VkMall
{
	namespace Wrappers
	{
		void VertexDescription::AddAttribute(uint32_t AttribBinding, uint32_t AttribLocation, size_t AttribOffset, Enums::eAttributeType Type)
        {
            VkVertexInputAttributeDescription AttribDesc{};
            AttribDesc.binding = AttribBinding;
            AttribDesc.location = AttribLocation;
            AttribDesc.format = (VkFormat)Type;
			AttribDesc.offset = AttribOffset;

        /*
            switch(Type)
            {
            case Enums::fVec1:
            case Enums::iVec1:
            case Enums::uVec1:
                Offset += sizeof(float)*1;
                break;
            case Enums::fVec2:
            case Enums::iVec2:
            case Enums::uVec2:
                Offset += sizeof(float)*2;
                break;
            case Enums::fVec3:
            case Enums::iVec3:
            case Enums::uVec3:
                Offset += sizeof(float)*3;
                break;
            case Enums::fVec4:
            case Enums::iVec4:
            case Enums::uVec4:
                Offset += sizeof(float)*4;
                break;
            default:
                throw std::runtime_error("Wrappers : failed to add an attribute to a VertexDescription, Tried to add attribute of uknown type\n");
            }
        */

            Attributes.push_back(AttribDesc);
        }
		
		/*! \brief Loads the vertex data into pDst. The preferred method for storing vertices is by using a SoA approach and simply copying each array into it's own contiguous block, this is best practice for SIMD/GPU programming. */
		size_t DefaultVertex::Load(void *pDst)
		{
			if(PositionIter != Vertices.size() || NormalIter != Vertices.size() || UVIter != Vertices.size())
			{
				throw std::runtime_error("failed to upload vertex list. The Position, Normal, or UVs did not load in full (!= Vertices::size()+1)\n");
			}

			memcpy(pDst, Vertices.data(), sizeof(Vertex)*Vertices.size());
		
			return sizeof(Vertex)*Vertices.size();
		}
		
		std::string* DefaultVertex::GetElementList(uint32_t &ElementCount)
		{
			static std::string Ret[3] = {"POSITION", "NORMAL", "TEXCOORD_0"}; // This doesn't really change at all, and we need the array to stay persisten, so I just made it static for now. Maybe I'll change this later, probably not.
			
			ElementCount = 3;

			return Ret;
		}
		
		size_t DefaultVertex::Size()
		{
			return sizeof(Vertex)*Vertices.size();
		}

		VertexBinding DefaultVertex::GetBinding()
		{
			VertexBinding Binding;
			Binding.SetStride(0, sizeof(Vertex));
			
			return Binding;
		}
		
		VertexDescription DefaultVertex::GetDescription()
		{
			size_t Offset = 0; // The offset iterator

			VertexDescription Ret;

			/* Describe the attributes in our vertices*/
				Ret.AddAttribute(0, 0, Offset, Enums::fVec3);
				Offset += sizeof(glm::vec3);

				Ret.AddAttribute(0, 1, Offset, Enums::fVec3);
				Offset += sizeof(glm::vec3);

				Ret.AddAttribute(0, 2, Offset, Enums::fVec2);
				Offset += sizeof(glm::vec2);

			return Ret;
		}

		/*! \brief Loads the vertex data into pDst. The preferred method for storing vertices is by using a SoA approach and simply copying each array into it's own contiguous block, this is best practice for SIMD/GPU programming. */
//		size_t DefaultVertex::Load(void *pDst)
//		{
//			if(PositionIter != Vertices.size()+1 || NormalIter != Vertices.size()+1 || UVIter != Vertices.size()+1)
//			{
//				throw std::runtime_error("failed to upload vertex list. The Position, Normal, or UVs did not load in full (!= Vertices::size()+1)\n");
//			}
//
//			char* pCharDst = (char*)pDst; // A pointer to the Dst (Cast to a usable/walkable format).
//			size_t Ret = 0; // The offset iterator (shows the tail end of the volume).
//			
//			memcpy(pCharDst+Ret, Position.data(), Position.size()*sizeof(glm::vec3)); // Copy the positions into the buffer.
//			Ret += Position.size()*sizeof(glm::vec3); // Walk the iterator forward.
//			
//			memcpy(pCharDst+Ret, Normal.data(), Normal.size()*sizeof(glm::vec3)); // Copy the normals into the buffer.
//			Ret += Normal.size()*sizeof(glm::vec3); // Walk the iterator forward
//			
//			memcpy(pCharDst+Ret, UV.data(), UV.size()*sizeof(glm::vec2)); // Copy the UVs into the buffer.
//			Ret += UV.size()*sizeof(glm::vec2); // Walk the iterator forward
//			
//			return Ret;
//		}
//		
//		size_t DefaultVertex::GetStride()
//		{
//			size_t Ret;
//			
//			Ret = Position.size()*sizeof(glm::vec3);
//			Ret += Normal.size()*sizeof(glm::vec3);
//			Ret += UV.size()*sizeof(glm::vec2);
//			
//			return Ret;
//		}
//		
//		VertexDescription DefaultVertex::GetDescription()
//		{
//			size_t Offset = 0; // The offset iterator
//
//			VertexDescription Ret;
//
//			/* Describe the attributes in our vertices*/
//				Ret.AddAttribute(0, 0, Offset, Enums::fVec3);
//				Offset += Position.size()*sizeof(glm::vec3);
//
//				Ret.AddAttribute(0, 1, Offset, Enums::fVec3);
//				Offset += Normal.size()*sizeof(glm::vec3);
//
//				Ret.AddAttribute(0, 2, Offset, Enums::fVec3);
//				Offset += UV.size()*sizeof(glm::vec2);
//
//			return Ret;
//		}
	}
}
