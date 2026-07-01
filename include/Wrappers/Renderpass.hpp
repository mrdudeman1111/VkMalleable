#pragma once

#include "Wrappers/Framebuffer.hpp"

namespace VkMall
{
    namespace Wrappers
    {
        struct PassInfo
        {
        public:
            std::vector<std::pair<Enums::AttachmentType, VkImageLayout>> Attachments;
        };

        class Pass
        {
            public:
            Pass(PassInfo AttachmentInfos, VkPipelineBindPoint BindPoint);

            void AddDependency(VkSubpassDependency Dependency);

            inline VkSubpassDescription GetSubpass() { return vkSubpass; }
            inline VkSubpassDependency* GetDependencies(uint32_t& pDependencyCount) { pDependencyCount = Dependencies.size(); return Dependencies.data(); }

            private:

            bool bDepthBuffer; //! < Used to determine if the framebuffer contains a depth stencil attachment

            /* Vulkan */
                VkSubpassDescription vkSubpass; //! < Vulkan handle to the subpass description. Used during renderpass creation.

                std::vector<VkSubpassDependency> Dependencies = {}; //! < Contains all subpass dependency objects, these are used to define resource dependencies between subpasses within a renderpass.

                VkAttachmentReference DepthAttachment; //! < The VkAttachmentReference Object used for the depth stencil attachment if it exists, which is determined using the bDepthBuffer attribute.
                std::vector<VkAttachmentReference> ColorAttachments = {}; //! < Contains VkAttachmentReference Objects for each color attachment contained in the frame buffer (only the ones we use in this pass).
                std::vector<VkAttachmentReference> InputAttachments = {}; //! < Contains VkAttachmentReference Objects for each input attachment contained in the frame buffer (only the ones we use in this pass).
                std::vector<VkAttachmentReference> ResolveAttachments = {}; //! < Contains VkAttachmentReference Objects for each resolve attachment contained in the frame buffer (only the ones we use in this pass).
                std::vector<uint32_t> PreserveAttachments = {}; //! < Contains VkAttachmentReference Objects for each preserve attachment contained in the frame buffer (only the ones we use in this pass).
        };

        class Renderpass : VulkanObject
        {
            public:
            Renderpass(VkDevice& Device, uint32_t PassCount, Pass* Passes, Wrappers::FrameBufferDescription FBDesc, VkSampleCountFlagBits);

            operator VkRenderPass() {
                return vkRenderpass;
            }

            operator VkRenderPass*() {
                return &vkRenderpass;
            }

            void Begin(VkCommandBuffer* cmdBuffer, Framebuffer* FB, VkExtent2D RenderSize);
            void End(VkCommandBuffer* cmdBuffer);

            private:
            std::vector<VkSubpassDescription> Subpasses;
            VkRenderPass vkRenderpass;

            FrameBufferDescription FrameBufferDesc;
        };
    }
}