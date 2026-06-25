#include "Pipeline.hpp"
#include "Base.hpp"

#include <fstream>
#include <stdexcept>

#include <vulkan/vulkan_core.h>

namespace Wrappers
{
  namespace Shaders
  {
    Shader::Shader(VkDevice* Device, Enums::eShaderStage Stage, std::string ShaderPath, std::string ShaderName) : VulkanObject(Device, ShaderName)
    {
      VkResult Err;
      std::ifstream ShaderFile(ShaderPath, std::ifstream::binary);

      if(ShaderFile)
      {
        ShaderFile.seekg(0, ShaderFile.end);
        size_t BinSize = ShaderFile.tellg();
        ShaderFile.seekg(0, ShaderFile.beg);

        char BinBuffer[BinSize];

        ShaderFile.read(BinBuffer, BinSize);

        VkShaderModuleCreateInfo ShaderCI{};
        ShaderCI.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
        ShaderCI.codeSize = BinSize;
        ShaderCI.pCode = (const uint32_t*)BinBuffer;

        if((Err = vkCreateShaderModule(*pDevice, &ShaderCI, nullptr, &vkShader)) != VK_SUCCESS)
        {
          throw std::runtime_error("Wrappers : failed to create shader module for shader " + ShaderName);
        }

        StageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
        StageInfo.pNext = nullptr;
        StageInfo.flags = 0;
        StageInfo.module = vkShader;
        StageInfo.stage = VkShaderStageFlagBits(Stage);
        StageInfo.pName = "main";
        StageInfo.pSpecializationInfo = nullptr;
      }
      else
      {
        throw std::runtime_error("Wrappers : Shaders : failed to open shader file: " + ShaderPath + "\n");
      }
    }

    Shader::~Shader()
    {
      vkDestroyShaderModule(*pDevice, vkShader, nullptr);
    }

    DescriptorSet::DescriptorSet(VkDevice* Device, VkDescriptorPool* pDescriptorPool, Memory::Allocator* pAllocator, uint32_t SetIndex, std::string DescriptorName) : pAllocator(pAllocator), pPool(pDescriptorPool), SetIndex(SetIndex), VulkanObject(Device, DescriptorName)
    {
      vkSet = VK_NULL_HANDLE;
    }

    void DescriptorSet::AddUniform(ShaderBuffer_Base* Uniform)
    {
      VkResult Err;

      for(uint32_t i = 0; i < Bindings.size(); i++)
      {
        if(Uniform->GetBinding())
        {
          std::cout << "Wrappers : " << ObjectName << " : Attempted to add a uniform buffer (" << Uniform->ObjectName << ") which shares a binding index with one of the descriptors\n";
          return;
        }
      }

      UniformBuffers.push_back(Uniform);

      VkDescriptorSetLayoutBinding Binding{};
      Binding.binding = Uniform->GetBinding();
      Binding.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
      Binding.descriptorCount = 1;
      Binding.stageFlags = (VkShaderStageFlagBits)Uniform->ShaderStage;

      Bindings.push_back(Binding);

      VkDescriptorSetLayoutCreateInfo LayoutCI{};
      LayoutCI.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
      LayoutCI.bindingCount = Bindings.size();
      LayoutCI.pBindings = Bindings.data();

      if((Err = vkCreateDescriptorSetLayout(*pDevice, &LayoutCI, nullptr, &vkLayout)) != VK_SUCCESS)
      {
        throw std::runtime_error("Wrappers : failed to recreate descriptor set layout while adding buffer for descriptor " + ObjectName);
      }

      Dirty = true;
    }

    void DescriptorSet::Update()
    {
      VkResult Err;

      if(Dirty || vkSet == VK_NULL_HANDLE)
      {
        vkFreeDescriptorSets(*pDevice, *pPool, 1, &vkSet);

        VkDescriptorSetAllocateInfo AllocInfo{};
        AllocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
        AllocInfo.descriptorPool = *pPool;
        AllocInfo.descriptorSetCount = 1;
        AllocInfo.pSetLayouts = &vkLayout;

        if((Err = vkAllocateDescriptorSets(*pDevice, &AllocInfo, &vkSet)) != VK_SUCCESS)
        {
          throw std::runtime_error("Wrappers : failed to update(reallocate) descriptor set : " + ObjectName);
        }
      }

      std::vector<VkDescriptorBufferInfo> BufferInfos;
      std::vector<VkWriteDescriptorSet> WriteInfos;

      for(uint32_t i = 0; i < UniformBuffers.size(); i++)
      {
        BufferInfos.push_back(UniformBuffers[i]->DescriptorWrite());

        VkWriteDescriptorSet tmpWrite{};
        tmpWrite.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;

        tmpWrite.dstSet = vkSet;
        tmpWrite.dstBinding = UniformBuffers[i]->GetBinding();
        tmpWrite.dstArrayElement = 0;

        tmpWrite.pBufferInfo = &BufferInfos[i];

        tmpWrite.descriptorCount = 1;
        tmpWrite.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;

        WriteInfos.push_back(tmpWrite);
      }

      vkUpdateDescriptorSets(*pDevice, WriteInfos.size(), WriteInfos.data(), 0, nullptr);
    }

    void DescriptorSet::Bind(VkCommandBuffer* pCmdBuffer, VkPipelineLayout Pipeline)
    {
      vkCmdBindDescriptorSets(*pCmdBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, Pipeline, SetIndex, 1, &vkSet, 0, nullptr);
    }
  }

  void VertexDescription::AddAttribute(uint32_t AttribBinding, uint32_t AttribLocation, size_t AttribOffset, Enums::eAttributeType Type)
  {
    VkVertexInputAttributeDescription AttribDesc{};
    AttribDesc.binding = AttribBinding;
    AttribDesc.location = AttribLocation;
    AttribDesc.format = (VkFormat)Type;

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

    AttribDesc.offset = AttribOffset;

    Attributes.push_back(AttribDesc);
  }

  Pipeline::Pipeline(VkDevice* Device, PipelineDescription PipeDesc, std::string PipelineName) : RP(PipeDesc.pRenderpass), VulkanObject(Device, PipelineName)
  {
    VkResult Err;

    VkDescriptorSetLayout DescriptorLayouts[PipeDesc.Descriptors.size()];

    for(uint32_t i = 0; i < PipeDesc.Descriptors.size(); i++)
    {
      DescriptorLayouts[i] = PipeDesc.Descriptors[i]->GetLayout();
    }

    VkPipelineLayoutCreateInfo LayoutCI{};
    LayoutCI.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
    LayoutCI.setLayoutCount = PipeDesc.Descriptors.size();
    LayoutCI.pSetLayouts = DescriptorLayouts;

    if((Err = vkCreatePipelineLayout(*pDevice, &LayoutCI, nullptr, &Layout)) != VK_SUCCESS)
    {
      throw std::runtime_error("Wrappers: failed to create pipeline layout\n");
    }

    // we must have a fragment shader and a vertex OR geometry shader. Otherwise it's an invalid pipeline.
    VkPipelineShaderStageCreateInfo ShaderStages[2];

    bUseGeometry = false;

    if(PipeDesc.VertexShaderPath.compare("") != 0)
    {
      VertexShader = new Shaders::Shader(pDevice, Enums::eShaderStage::eVertex, PipeDesc.VertexShaderPath);
      ShaderStages[0] = VertexShader->GetShaderStageInfo();
    }
    else if(PipeDesc.GeometryShaderPath.compare("") != 0)
    {
      GeometryShader = new Shaders::Shader(pDevice, Enums::eShaderStage::eGeometry, PipeDesc.GeometryShaderPath);
      ShaderStages[0] = GeometryShader->GetShaderStageInfo();
      bUseGeometry = true;
    }
    if(PipeDesc.FragmentShaderPath.compare("") != 0)
    {
      FragmentShader = new Shaders::Shader(pDevice, Enums::eShaderStage::eFragment, PipeDesc.FragmentShaderPath);
      ShaderStages[1] = FragmentShader->GetShaderStageInfo();
    }

    uint32_t vtxBindingCount;
    VkVertexInputBindingDescription* vtxBindings;
    VkVertexInputAttributeDescription* vtxAttributes;
    VkPipelineVertexInputStateCreateInfo InputCI{};
    VkPipelineInputAssemblyStateCreateInfo AssemblyCI{};
    VkPipelineRasterizationStateCreateInfo RasterCI{};
    VkRect2D Scissor;
    VkViewport Viewport;
    VkPipelineViewportStateCreateInfo ViewportCI{};
    std::vector<VkPipelineColorBlendAttachmentState> ColorBlendAttachments;
    VkPipelineColorBlendStateCreateInfo ColorBlendCI{};
    VkPipelineMultisampleStateCreateInfo MultiSampleCI{};
    VkPipelineDepthStencilStateCreateInfo DepthStencilCI{};

    // pipeline state structs
    {
      vtxBindings = PipeDesc.vtxBinding.GetBindings(vtxBindingCount);
      uint32_t vtxAttribCount;
      vtxAttributes = PipeDesc.vtxDesc.GetAttributes(vtxAttribCount);

      InputCI.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
      InputCI.vertexBindingDescriptionCount = vtxBindingCount;
      InputCI.pVertexBindingDescriptions = vtxBindings;
      InputCI.vertexAttributeDescriptionCount = vtxAttribCount;
      InputCI.pVertexAttributeDescriptions = vtxAttributes;

      AssemblyCI.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
      AssemblyCI.topology = PipeDesc.Topology;

      RasterCI.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
      RasterCI.cullMode = VK_CULL_MODE_BACK_BIT;
      RasterCI.frontFace = VK_FRONT_FACE_CLOCKWISE;
      RasterCI.lineWidth = 1.f;
      RasterCI.polygonMode = VK_POLYGON_MODE_FILL;

      Scissor.extent = PipeDesc.PipelineResolution;
      Scissor.offset = {0, 0};

      Viewport.x = 0;
      Viewport.y = 0;
      Viewport.width = PipeDesc.PipelineResolution.width;
      Viewport.height = PipeDesc.PipelineResolution.height;
      Viewport.minDepth = 0;
      Viewport.maxDepth = 1;

      ViewportCI.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
      ViewportCI.scissorCount = 1;
      ViewportCI.pScissors = &Scissor;
      ViewportCI.viewportCount = 1;
      ViewportCI.pViewports = &Viewport;

      ColorBlendAttachments.push_back({});

      ColorBlendAttachments[0].colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
      ColorBlendAttachments[0].blendEnable = VK_FALSE;

      ColorBlendCI.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
      ColorBlendCI.attachmentCount = ColorBlendAttachments.size();
      ColorBlendCI.pAttachments = ColorBlendAttachments.data();
      ColorBlendCI.logicOpEnable = VK_FALSE;

      MultiSampleCI.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
      MultiSampleCI.sampleShadingEnable = VK_FALSE;
      MultiSampleCI.rasterizationSamples = PipeDesc.SampleCount;

      DepthStencilCI.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
      DepthStencilCI.minDepthBounds = 0.f;
      DepthStencilCI.maxDepthBounds = 1.f;
      DepthStencilCI.depthTestEnable = VK_TRUE;
      DepthStencilCI.depthCompareOp = VK_COMPARE_OP_LESS_OR_EQUAL;
      DepthStencilCI.depthWriteEnable = VK_TRUE;
      DepthStencilCI.stencilTestEnable = VK_FALSE;
    }

    VkGraphicsPipelineCreateInfo PipelineCI{};
    PipelineCI.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
    PipelineCI.layout = Layout;
    PipelineCI.renderPass = *PipeDesc.pRenderpass;
    PipelineCI.subpass = PipeDesc.Subpass;
    PipelineCI.stageCount = 2;
    PipelineCI.pStages = ShaderStages;
    PipelineCI.pVertexInputState = &InputCI;
    PipelineCI.pInputAssemblyState = &AssemblyCI;
    PipelineCI.pViewportState = &ViewportCI;
    PipelineCI.pColorBlendState = &ColorBlendCI;
    PipelineCI.pMultisampleState = &MultiSampleCI;
    PipelineCI.pDepthStencilState = &DepthStencilCI;
    PipelineCI.pRasterizationState = &RasterCI;

    if((Err = vkCreateGraphicsPipelines(*pDevice, VK_NULL_HANDLE, 1, &PipelineCI, nullptr, &vkPipe)) != VK_SUCCESS)
    {
      throw std::runtime_error("Wrappers : failed to create pipeline\n");
    }
  }

  Pipeline::~Pipeline()
  {
    if(bUseGeometry)
    {
      delete GeometryShader;
      delete FragmentShader;
    }
    else
    {
      delete VertexShader;
      delete FragmentShader;
    }

    vkDestroyPipelineLayout(*pDevice, Layout, nullptr);
    vkDestroyPipeline(*pDevice, vkPipe, nullptr);
  }
}

