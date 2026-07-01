#pragma once

#include "Wrappers/Descriptors.hpp"
#include "Wrappers/Renderpass.hpp"

#include "Wrappers/Vertex.hpp"

namespace VkMall
{
    namespace Wrappers
    {
        struct Shader : VulkanObject
        {
        public:
            Shader(VkDevice* Device, Enums::eShaderStage Stage, std::string ShaderPath, std::string ShaderName = "Shader");
            ~Shader();

            inline VkShaderModule GetShaderModule() { return vkShader; }
            inline VkPipelineShaderStageCreateInfo GetShaderStageInfo() { return StageInfo; }

            Enums::eShaderStage Stage;

        private:
            VkShaderModule vkShader;
            VkPipelineShaderStageCreateInfo StageInfo;
        };

        struct PipelineDescription
        {
            std::string VertexShaderPath = "";
            std::string GeometryShaderPath = "";
            std::string FragmentShaderPath = "";

            VertexBinding vtxBinding;
            VertexDescription vtxDesc;

            Renderpass* pRenderpass;
            uint32_t Subpass;

            VkSampleCountFlagBits SampleCount;

            VkPrimitiveTopology Topology;

            VkExtent2D PipelineResolution;

            std::vector<DescriptorSet*> Descriptors;
        };

        class Pipeline : VulkanObject
        {
            public:
            Pipeline(VkDevice* Device, PipelineDescription PipeDesc, std::string PipelineName = "Pipeline");
            ~Pipeline();

            operator VkPipeline() {
                return vkPipe;
            }

            operator VkPipeline*() {
                return &vkPipe;
            }

            operator VkPipelineLayout() {
                return Layout;
            }

            inline void Bind(VkCommandBuffer* cmdBuffer) { vkCmdBindPipeline(*cmdBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, vkPipe); }
            inline VkPipeline GetPipeline() { return vkPipe; }
            inline VkPipelineLayout GetLayout() { return Layout; }

            private:
            VkPipelineLayout Layout;
            VkPipeline vkPipe;

            Renderpass* RP;

            bool bUseGeometry;

            Shader* VertexShader;
            Shader* GeometryShader;

            Shader* FragmentShader;
        };
    }
}
