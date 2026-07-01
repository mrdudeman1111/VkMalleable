#include "Wrappers/Renderpass.hpp"

namespace VkMall
{
    namespace Wrappers
    {
        Pass::Pass(PassInfo AttachmentInfos, VkPipelineBindPoint BindPoint)
        {
            bDepthBuffer = false;

            Dependencies.resize(0);

            for(uint32_t i = 0; i < AttachmentInfos.Attachments.size(); i++)
            {
            switch(AttachmentInfos.Attachments[i].first)
            {
                case Enums::AttachmentType::eColorAtt:
                ColorAttachments.push_back({});
                ColorAttachments.front().layout = AttachmentInfos.Attachments[i].second;
                ColorAttachments.front().attachment = i;
                break;
                case Enums::AttachmentType::eDepthAtt:
                bDepthBuffer = true;
                DepthAttachment.layout = AttachmentInfos.Attachments[i].second;
                DepthAttachment.attachment = i;
                break;
                case Enums::AttachmentType::eInputAtt:
                InputAttachments.push_back({});
                InputAttachments.front().layout = AttachmentInfos.Attachments[i].second;
                InputAttachments.front().attachment = i;
                break;
                case Enums::AttachmentType::ePreserveAtt:
                PreserveAttachments.push_back({});
                PreserveAttachments.front() = i;
                break;
                case Enums::AttachmentType::eResolveAtt:
                ResolveAttachments.push_back({});
                ResolveAttachments.front().layout = AttachmentInfos.Attachments[i].second;
                ResolveAttachments.front().attachment = i;
                break;
            }
            }

            vkSubpass.colorAttachmentCount = ColorAttachments.size();
            vkSubpass.pColorAttachments = (ColorAttachments.size() == 0) ? nullptr : ColorAttachments.data();

            vkSubpass.inputAttachmentCount = InputAttachments.size();
            vkSubpass.pInputAttachments = (InputAttachments.size() == 0) ? nullptr : InputAttachments.data();

            vkSubpass.preserveAttachmentCount = PreserveAttachments.size();
            vkSubpass.pPreserveAttachments = (PreserveAttachments.size() == 0) ? nullptr : PreserveAttachments.data();

            vkSubpass.pDepthStencilAttachment = (bDepthBuffer) ? &DepthAttachment : nullptr;

            vkSubpass.pipelineBindPoint = BindPoint;

            vkSubpass.pResolveAttachments = (ResolveAttachments.size() == 0) ? nullptr : ResolveAttachments.data();

            vkSubpass.flags = 0;
        }

        void Pass::AddDependency(VkSubpassDependency Dependency)
        {
            Dependencies.push_back(Dependency);
        }

        Renderpass::Renderpass(VkDevice& Device, uint32_t PassCount, Pass* Passes, FrameBufferDescription FBDesc, VkSampleCountFlagBits SampleCount) : VulkanObject(&Device, "Renderpass")
        {
            VkResult Err;

            FrameBufferDesc = FBDesc;

            VkSubpassDescription Subpasses[PassCount];

            for(uint32_t i = 0; i < PassCount; i++)
            {
                Subpasses[i] = Passes[i].GetSubpass();
            }

            std::vector<VkSubpassDependency> Dependencies;

            for(uint32_t i = 0; i < PassCount; i++)
            {
            uint32_t DepCount;
            VkSubpassDependency* tmp = Passes[i].GetDependencies(DepCount);

            for(uint32_t x = 0; x < DepCount; x++)
            {
                Dependencies.push_back(tmp[x]);
            }
            }

            VkAttachmentDescription* Attachments = new VkAttachmentDescription[FBDesc.BufferCount];

            for(uint32_t i = 0; i < FBDesc.BufferCount; i++)
            {
            Attachments[i].format = FBDesc.BufferDescs[i].Format;
            Attachments[i].storeOp = FBDesc.BufferDescs[i].StorageOp;
            Attachments[i].loadOp = FBDesc.BufferDescs[i].LoadOp;
            Attachments[i].initialLayout = FBDesc.BufferDescs[i].FirstLayout;
            Attachments[i].finalLayout = FBDesc.BufferDescs[i].FinalLayout;
            Attachments[i].samples = FBDesc.BufferDescs[i].Samples;
            //Attachments[i].samples = SampleCount;
            }

            VkRenderPassCreateInfo RenderpassCI{};
            RenderpassCI.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
            RenderpassCI.subpassCount = PassCount;
            RenderpassCI.pSubpasses = Subpasses;
            RenderpassCI.attachmentCount = FBDesc.BufferCount;
            RenderpassCI.pAttachments = Attachments;
            RenderpassCI.dependencyCount = (uint32_t)Dependencies.size();
            RenderpassCI.pDependencies = (Dependencies.size() == 0) ? nullptr : Dependencies.data();

            if((Err = vkCreateRenderPass(Device, &RenderpassCI, nullptr, &vkRenderpass)) != VK_SUCCESS)
            {
            throw std::runtime_error("Wrappers : failed to create renderpass\n");
            }
            
            delete[] Attachments;
        }

        void Renderpass::Begin(VkCommandBuffer* cmdBuffer, Framebuffer* pFrameBuffer, VkExtent2D RenderSize)
        {
            FrameBufferDescription Description = pFrameBuffer->GetDescription();

            VkClearValue ClearValues[Description.BufferCount];

            for(uint32_t i = 0; i < Description.BufferCount; i++)
            {
            if(Description.BufferDescs[i].BufferTypes & Enums::FrameBufferTypeBits::eDepth)
            {
                ClearValues[i] = { {1.f, 0} };
            }
            else
            {
                ClearValues[i].color = { { 0.f, 0.f, 0.f, 0.f} };
            }
            }

            VkRenderPassBeginInfo BeginInfo{};
            BeginInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
            BeginInfo.renderPass = vkRenderpass;
            BeginInfo.renderArea = { (uint32_t)0, (uint32_t)0, (uint32_t)RenderSize.width, (uint32_t)RenderSize.height };
            BeginInfo.framebuffer = *pFrameBuffer;
            BeginInfo.clearValueCount = Description.BufferCount;
            BeginInfo.pClearValues = ClearValues;

            vkCmdBeginRenderPass(*cmdBuffer, &BeginInfo, VK_SUBPASS_CONTENTS_INLINE);
        }

        void Renderpass::End(VkCommandBuffer* cmdBuffer)
        {
            vkCmdEndRenderPass(*cmdBuffer);
        }
    }
}