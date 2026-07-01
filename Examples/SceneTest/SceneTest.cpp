#include <glm/ext/matrix_clip_space.hpp>
#include <glm/ext/matrix_transform.hpp>
#include <glm/ext/matrix_projection.hpp>
#include <glm/fwd.hpp>
#include <glm/matrix.hpp>

#include "Wrappers/Instance.hpp"
#include "Wrappers/Device.hpp"
#include "Wrappers/Swapchain.hpp"
#include "Wrappers/Mesh.hpp"

#include <vector>

#define APP_WIDTH 1280
#define APP_HEIGHT 720

/******* NOTES ************
 * 
 * Curves will not function when scaled (through position on CPU side, But it will work when placed through transformations through a vertex shader.)
*/

/* tiny struct to handle Action States */
struct {
  bool Forward = false;
  bool Back = false;
  bool Right = false;
  bool Left = false;
  bool Up = false;
  bool Down = false;

  bool uiEnable = false;
} ActionTable;

struct MVP
{
  glm::mat4 World;
  glm::mat4 View;
  glm::mat4 Proj;
};

class Camera
{
  public:
    Camera() { CameraMatrix = glm::translate(glm::mat4(1.f), glm::vec3(0.f, 0.f, 10.f)); pUniformData = nullptr; }
    ~Camera() { delete pUniformData; }

    void Move(glm::vec3 PosDelta)
    {
      /*
      glm::vec3 Right = glm::vec3(CameraMatrix[0][0], CameraMatrix[1][0], CameraMatrix[2][0]);
      glm::vec3 Up = glm::vec3(CameraMatrix[0][1], CameraMatrix[1][1], CameraMatrix[2][1]);
      glm::vec3 Forward = glm::vec3(CameraMatrix[0][2], CameraMatrix[1][2], CameraMatrix[2][2])*-1.f;
      */

      glm::vec3 Right(1.f, 0.f, 0.f);
      glm::vec3 Up(0.f, 1.f, 0.f);
      glm::vec3 Forward(0.f, 0.f, 1.f);
      Forward *= -1.f;

      glm::vec3 Total(0.f);
      Total += PosDelta.x*Right;
      Total += PosDelta.y*Up;
      Total += PosDelta.z*Forward;

      //Total = normalize(Total);

      CameraMatrix = glm::translate(CameraMatrix, Total);
    }

    void Rotate(glm::vec2 RotDelta)
    {
      // Up Direction is always constant when handling cameras

      glm::vec3 Up(CameraMatrix[0][1], CameraMatrix[1][1], CameraMatrix[2][1]);
      Up *= -1.f;
      CameraMatrix = glm::rotate(CameraMatrix, glm::radians(RotDelta.x), Up);

      glm::vec3 Right(1.f, 0.f, 0.f);
      Right *= -1.f;
      CameraMatrix = glm::rotate(CameraMatrix, glm::radians(RotDelta.y), Right);
    }

    void Update()
    {
      glm::vec3 MoveVector(0.f, 0.f, 0.f);
      if(ActionTable.Forward)
      {
        MoveVector.z += 0.01f;
      }
      if(ActionTable.Back)
      {
        MoveVector.z -= 0.01f;
      }
      if(ActionTable.Right)
      {
        MoveVector.x += 0.01f;
      }
      if(ActionTable.Left)
      {
        MoveVector.x -= 0.01f;
      }
      if(ActionTable.Up)
      {
        MoveVector.y += 0.01f;
      }
      if(ActionTable.Down)
      {
        MoveVector.y -= 0.01f;
      }

      Move(MoveVector);

      pUniformData->pBuffer->World = glm::mat4(1.f);
      pUniformData->pBuffer->View = glm::inverse(CameraMatrix);
      pUniformData->pBuffer->Proj = glm::perspective(45.f, 16.f/9.f, 0.01f, 1000.f);
      pUniformData->pBuffer->Proj[1][1] *= -1.f;

      pUniformData->Update();
    }

    glm::mat4 CameraMatrix;

    VkMall::Wrappers::ShaderBuffer<MVP>* pUniformData; // A pointer to a Shader buffer created by our device.
} SceneCamera;

void KeyBoardInput(GLFWwindow* pWindow, int Key, int ScanCode, int Action, int Mods)
{
  if(!ActionTable.uiEnable)
  {
    glm::vec3 TotalMove(0.f);

    bool bPressed = (Action == GLFW_PRESS || Action == GLFW_REPEAT) ? 1 : 0;

    switch(Key)
    {
      case GLFW_KEY_W:
        ActionTable.Forward = bPressed;
        break;
      case GLFW_KEY_S:
        ActionTable.Back = bPressed;
        break;
      case GLFW_KEY_D:
        ActionTable.Right = bPressed;
        break;
      case GLFW_KEY_A:
        ActionTable.Left = bPressed;
        break;
      case GLFW_KEY_E:
        ActionTable.Up = bPressed;
        break;
      case GLFW_KEY_Q:
        ActionTable.Down = bPressed;
        break;
      case GLFW_KEY_TAB:
        if(bPressed)
        {
          ActionTable.uiEnable = bPressed;

          glfwSetInputMode(pWindow, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
        }
        break;
    }
  }
  else
  {
    if(Action == GLFW_PRESS && Key == GLFW_KEY_TAB)
    {
      ActionTable.uiEnable = false;

      glfwSetInputMode(pWindow, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
    }
  }
}

glm::vec2 MousePos;
void MousePosInput(GLFWwindow* pWindow, double xPos, double yPos)
{
  if(!ActionTable.uiEnable)
  {
    glm::vec2 Rotate = glm::vec2(xPos, yPos) - MousePos;
    Rotate /= 100.f;
    SceneCamera.Rotate(Rotate);
    MousePos = {xPos, yPos};
  }
}

inline bool CompVec(glm::vec3& A, glm::vec3& B)
{
  if(glm::all(glm::equal(A, B)))
  {
    return true;
  }

  return false;
}

int main()
{
  VkMall::Wrappers::Instance* pInstance;
  VkMall::Wrappers::Device* pDevice;
  VkMall::Wrappers::Swapchain* pSwapchain;
  VkMall::Wrappers::Renderpass* pRenderpass;
  VkMall::Wrappers::ShaderBuffer<MVP>* pMvpBuffer;
  VkMall::Wrappers::Mesh* pMesh;
  VkMall::Wrappers::Mesh* pTestingMesh;
  VkMall::Wrappers::Mesh* pGlyphMesh;
  VkMall::Wrappers::Pipeline* pPipeline;
  VkMall::Wrappers::Pipeline* pVisPipe;
  VkMall::Wrappers::Pipeline* pWireFramePipe;
  VkMall::Wrappers::Pipeline* pCubicPipe;
  VkMall::Wrappers::CommandBuffer* pRenderBuffer;
  VkMall::Wrappers::Semaphore** pRenderSemaphores;
  std::vector<const char*> DeviceExtensions = {};
	
  // Create Instance and window
  pInstance = new VkMall::Wrappers::Instance();

  pInstance->CreateWindow(APP_WIDTH, APP_HEIGHT, "SceneTest");
  glfwSetKeyCallback(pInstance->GetWindow(), KeyBoardInput);
  glfwSetCursorPosCallback(pInstance->GetWindow(), MousePosInput);

  // Create device
  pDevice = pInstance->CreateDevice(DeviceExtensions.size(), DeviceExtensions.data());

  // Create swapchain
  pSwapchain = pDevice->CreateSwapchain({APP_WIDTH, APP_HEIGHT});

  // define framebuffers
    VkMall::Wrappers::FrameBufferDescription Framebuffer;
    Framebuffer.BufferCount = 2; // The number of attachments our framebuffers will have

    /* These structures tell our renderpass how this framebuffer will change during a render. */
    Framebuffer.BufferDescs.push_back({});
    Framebuffer.BufferDescs[0].BufferTypes = VkMall::Enums::eColor; // This is a color attachment.
    Framebuffer.BufferDescs[0].FirstLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR; // That will start in a present-ready layout
    Framebuffer.BufferDescs[0].FinalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR; // and will end in a present-ready layout
    Framebuffer.BufferDescs[0].StorageOp = VK_ATTACHMENT_STORE_OP_STORE; // it will be stored during the store phase of the renderpass
    Framebuffer.BufferDescs[0].LoadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;  // and it will be cleared during the load phase of the renderpass
    Framebuffer.BufferDescs[0].Samples = VK_SAMPLE_COUNT_1_BIT; // The multi-sampling count will be 1 (off)
    Framebuffer.BufferDescs[0].Format = pSwapchain->GetFormat().format; // and the format of the image will be the same as our swapchain's format

    Framebuffer.BufferDescs.push_back({});
    Framebuffer.BufferDescs[1].BufferTypes = VkMall::Enums::eDepth; // This is a depth attachment.
    Framebuffer.BufferDescs[1].FirstLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL; // That will start in a depth stencil layout
    Framebuffer.BufferDescs[1].FinalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL; // and will end in a depth stencil format
    Framebuffer.BufferDescs[1].StorageOp = VK_ATTACHMENT_STORE_OP_STORE; // It will be stored during the store phase of the renderpass
    Framebuffer.BufferDescs[1].LoadOp = VK_ATTACHMENT_LOAD_OP_CLEAR; // and it will be cleared during the load phase of the rendperass
    Framebuffer.BufferDescs[1].Samples = VK_SAMPLE_COUNT_1_BIT; // The multi-sampling count will be 1 (off)
    Framebuffer.BufferDescs[1].Format = VK_FORMAT_D32_SFLOAT; // and the format of the image will be Depth 32-bit signed floats (i.e. each pixel in the attachment/image will be represented by a 32-bit signed float)

  // define subpass [0], The main rendering pass for our meshes
    VkMall::Wrappers::PassInfo MainPassInfo;
    MainPassInfo.Attachments.push_back({VkMall::Enums::AttachmentType::eColorAtt, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL}); // During the first subpass, our attachments will transition (if needed) from their starting layouts to the ones defined here. The first attachment will be a color attachment,
    MainPassInfo.Attachments.push_back({VkMall::Enums::AttachmentType::eDepthAtt, VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL}); // And the second attachment will be a depth stencil attachment.

    /* create the subpass */
      VkMall::Wrappers::Pass MainPass(MainPassInfo, VK_PIPELINE_BIND_POINT_GRAPHICS);

  /* Create Renderpass */
    pRenderpass = pDevice->CreateRenderpass(1, &MainPass, Framebuffer, VK_SAMPLE_COUNT_1_BIT); // Create a renderpass with 1 subpass (MainPass) using the FrameBufferDescription we filled out above and an MSAA count of 1 (off)

  /* Create Framebuffers */
    pSwapchain->CreateFrameBuffers(*pRenderpass, Framebuffer); // Using the renderpass we just made Create the framebuffer objects defined by our FrameBufferDescription above.

  /* When we create a uniform buffer, the data in the buffer can be accessed through the provided pBuffer attribute, which is a pointer to the uniform's mapped memory. whenever you are done editing the pBuffer's data, you should stage the changes with Update() */
  /* When using uniform buffers, we  */
    SceneCamera.pUniformData = pDevice->CreateGlobalUniformBuffer<MVP>(VkMall::Enums::eShaderStage::eVertex, 0);

    /* Create and load a mesh */
    pMesh = pDevice->CreateMesh();
    pMesh->Load(RESDIR"Pawn.glb");

    /*
      pTestingMesh = pDevice->CreateMesh();
      pTestingMesh->Load("TestBorderHole.dae");

      std::vector<glm::vec2> TestPos(pTestingMesh->Vertices.size());

      for(uint32_t i = 0; i < pTestingMesh->Vertices.size(); i++)
      {
        TestPos[i] = glm::vec2(pTestingMesh->Vertices[i].Position.x, pTestingMesh->Vertices[i].Position.y);
      }
    */

    VkMall::Wrappers::PipelineDescription PipeDesc;
    PipeDesc.VertexShaderPath = SHADER_DIR"Vert.spv";
    PipeDesc.FragmentShaderPath = SHADER_DIR"Frag.spv";
    PipeDesc.pRenderpass = pRenderpass;
    PipeDesc.Subpass = 0;
    PipeDesc.vtxBinding = pMesh->GetVertexBinding();
    PipeDesc.vtxDesc = pMesh->GetVertexDescription();
    PipeDesc.Topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
    PipeDesc.SampleCount = VK_SAMPLE_COUNT_1_BIT;
    PipeDesc.PipelineResolution.width = APP_WIDTH;
    PipeDesc.PipelineResolution.height = APP_HEIGHT;

    pPipeline = pDevice->CreatePipeline(PipeDesc, "Standard flat Pipeline");
   
    pRenderSemaphores = new VkMall::Wrappers::Semaphore*[pSwapchain->SwapchainImages.size()];

    for(uint32_t i = 0; i < pSwapchain->SwapchainImages.size(); i++)
    {
      pRenderSemaphores[i] = pDevice->CreateSemaphore("Render Semaphore");
    }

    glfwSetInputMode(pInstance->GetWindow(), GLFW_CURSOR, GLFW_CURSOR_DISABLED);

    /* Was testing the Curve rendering via FontRenderer's curve system
      std::vector<uint32_t> DelaunayIndices;
      glm::vec2* cdtVertices = nullptr;
      uint32_t* cdtIndices = nullptr;

      {
        pDevice->Allocate(vtxCdt, Wrappers::Enums::eMemoryType::eHost);
        pDevice->Allocate(idxCdt, Wrappers::Enums::eMemoryType::eHost);

        cdtVertices = (glm::vec2*)vtxCdt->Alloc->Map();
        cdtIndices = (uint32_t*)idxCdt->Alloc->Map();

        std::vector<std::pair<uint32_t, uint32_t>> Constraints = {{9, 8}, {8, 6}, {6, 3}, {3, 1}, {1, 5}, {5, 11}, {11, 10}, {10, 4}, {4, 0}, {0, 2}, {2, 7}, {7, 9}};
        Constraints.insert(Constraints.begin(), {{21, 19}, {19, 14}, {14, 12}, {12, 16}, {16, 22}, {22, 23}, {23, 17}, {17, 13}, {13, 15}, {15, 18}, {18, 20}, {20, 21}});

        DelaunayIndices = ekCdt::Triangulate(.size(), (ekCdt::vec2*)TestPos.data(), Constraints);
      }

      // https://www.khronos.org/opengl/wiki/Calculating_a_Surface_Normal#:~:text=A%20surface%20normal%20for%20a,of%20the%20face%20w.r.t.%20winding).

      memcpy(cdtVertices, TestPos.data(), sizeof(glm::vec2)*TestPos.size());
      memcpy(cdtIndices, DelaunayIndices.data(), sizeof(uint32_t)*DelaunayIndices.size());
    */

	pMesh->Update();
	// pMesh->Update();

	/* Try not to make calls to Allocator::Copy() while rendering. */
    while(!pInstance->ShouldClose())
    {
      pInstance->PollWindow(); // Performs any input polling we need from the window API (i.e. Click events, character input, window close signals, etc.)

      pRenderBuffer = pDevice->BeginRender(pSwapchain, pRenderpass); // Begin recording a rendering command. This requires a swapchain to present to and a renderpass to use. A pointer to the recording rendering command buffer is returned

        pRenderBuffer->Signal(pRenderSemaphores[pDevice->GetFbIdx()]); // Inform the rendering command buffer to signal the Render semaphore at the framebuffer index when finished.

        SceneCamera.Update(); // Update our camera transform and uniforms.

        pDevice->BindScene(*pRenderBuffer, *pPipeline);

        pPipeline->Bind(*pRenderBuffer);

          pMesh->Bind(*pRenderBuffer);
          pMesh->Draw(*pRenderBuffer);
		
        VkDeviceSize ZeroOffset = 0;

      pDevice->EndRender(); // End the recording command buffer.
      pRenderBuffer = nullptr;

      pDevice->SubmitRender();

      pDevice->Present(1, pRenderSemaphores[pDevice->GetFbIdx()]);
    }

    vkDeviceWaitIdle(*pDevice);

  delete pPipeline;
  delete pMesh;
  delete pMvpBuffer;
  delete pRenderpass;
  delete pSwapchain;
  delete pDevice;
  delete pInstance;

  std::cout << "Success!\n";
}

