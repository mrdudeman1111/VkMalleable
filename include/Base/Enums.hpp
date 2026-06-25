#pragma once

#include "vulkan/vulkan.h"

namespace VkMall
{
    namespace Enums
    {
        enum FrameBufferTypeBits
        {
            eColor = 0x01,
            eDepth = 0x02,
            eInput = 0x04,
            eSampled = 0x08
        };

        enum eMemoryType
        {
            eHost,
            eLocal
        };

        enum class eResourceType
        {
            eScene,
            eObject
        };

        enum class eShaderStage
        {
            eVertex = VK_SHADER_STAGE_VERTEX_BIT,
            eFragment = VK_SHADER_STAGE_FRAGMENT_BIT,
            eGeometry = VK_SHADER_STAGE_GEOMETRY_BIT
        };

        enum eInputRate
        {
            ePerVertex,
            ePerInstance
        };

        enum eAttributeType
        {
            fVec1 = VK_FORMAT_R32_SFLOAT,
            iVec1 = VK_FORMAT_R32_SINT,
            uVec1 = VK_FORMAT_R32_UINT,
            fVec2 = VK_FORMAT_R32G32_SFLOAT,
            iVec2 = VK_FORMAT_R32G32_SINT,
            uVec2 = VK_FORMAT_R32G32_UINT,
            fVec3 = VK_FORMAT_R32G32B32_SFLOAT,
            iVec3 = VK_FORMAT_R32G32B32_SINT,
            uVec3 = VK_FORMAT_R32G32B32_UINT,
            fVec4 = VK_FORMAT_R32G32B32A32_SFLOAT,
            iVec4 = VK_FORMAT_R32G32B32A32_SINT,
            uVec4 = VK_FORMAT_R32G32B32A32_UINT
        };

        enum class AttachmentType
        {
            eColorAtt = 1,
            eDepthAtt = 2,
            eInputAtt = 3,
            ePreserveAtt = 4,
            eResolveAtt = 5
        };


        enum eBufferUsage
        {

        };

        enum CommandType
        {
            eGraphics,
            eCompute,
            eTransfer
        };

        enum CommandLevel
        {
            ePrimary,
            eSecondary
        };

    }
}
