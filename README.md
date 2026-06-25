### Framework
 This is a vulkan framework I wrote to help me write testing applications. I considered using my [EZVK](https://github.com/mrdudeman1111/EZVK) Framework, but the memory allocator and a few of the abstractions I used in the wrappers was not what I was looking for. I plan to separate this framework into a separate repository for future use. I would like to shift towards a header ony approach that would allow users to simply download or copy a header and use the entire memory allocator, mesh system, or texture loader, etc. This way, if I only want to use my memory allocator in a future project, I can just grab the header and start using it. I wrote this framework pretty early on into development of this UI library, but I got a little more invested than I should have and ended writing a lot more than I planned. It took around 4 months or so to finish writing this framework.

Wrappers Classes:
    instance
    device
    command pool
    command buffer
    Swapchain
    BufferDescription (Attachment description)
    FrameBufferDescriptions (BufferDescriptions paired with attachment format)
    PassInfo (Subpass attachments)
    Pass (Subpass)
    Renderpass

        Shaders: 
            Shader
            ShaderBuffer_Base
            ShaderBuffer
            DescriptorSet

    VertexDescription
    PipelineDescription
    Pipeline

