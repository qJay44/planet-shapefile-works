#include <algorithm>
#include <format>
#include <direct.h>

#include "GLFW/glfw3.h"
#include "MiniMesh.hpp"
#include "Shader.hpp"
#include "glm/ext/matrix_transform.hpp"
#include "imgui.h"
#include "imgui_impl_glfw.h"
#include "imgui_impl_opengl3.h"
#include "pch.hpp"
#include "utils.hpp"

#define STB_IMAGE_IMPLEMENTATION
#include "stb/stb_image.h"

#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "stb/stb_image_write.h"

#define OUTPUT_WIDTH 2560
#define OUTPUT_HEIGHT 1280

constexpr vec2 scaleLimit{0.5f, 5.f};
constexpr float scaleAmount = 0.1f;
constexpr float panMoveScale = 0.3f;

static float dt;
static mat4 scaleMat = mat4(1.f);
static mat4 translateMat = mat4(1.f);
static bool imguiHovered = false;

void scrollCallback(GLFWwindow* window, double xoffset, double yoffset);
void mouseCursorCallback(GLFWwindow* window, double xpos, double ypos);

bool isImguiHovered(const vec2& mouse);

void produce(const Shader& drawShader, const Shader& compShader, const MiniMesh& borders, int w, int h);

int main() {
  // Assuming the executable is launching from its own directory
  _chdir("../../../src");

  // GLFW init
  glfwInit();
  glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
  glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 6);
  glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

  // Window init
  ivec2 winSize = {1600, 900};
  ivec2 winCenter = winSize / 2;
  GLFWwindow* window = glfwCreateWindow(winSize.x, winSize.y, "Sphere", NULL, NULL);

  if (!window) {
    printf("Failed to create GFLW window\n");
    glfwTerminate();
    return EXIT_FAILURE;
  }
  glfwMakeContextCurrent(window);
  glfwSetCursorPos(window, winCenter.x, winCenter.y);
  glfwSetScrollCallback(window, scrollCallback);
  glfwSetCursorPosCallback(window, mouseCursorCallback);

  // GLAD init
  if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress)) {
    printf("Failed to initialize GLAD\n");
    return EXIT_FAILURE;
  }

  glViewport(0, 0, winSize.x, winSize.y);

  IMGUI_CHECKVERSION();
  ImGui::CreateContext();
  ImGuiIO& io = ImGui::GetIO();
  io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
  io.ConfigFlags |= ImGuiConfigFlags_NoMouseCursorChange;
  ImGui_ImplGlfw_InitForOpenGL(window, true);
  ImGui_ImplOpenGL3_Init();

  Shader mainShader("main.vert", "main.frag");
  Shader framebufferShader("framebuffer.vert", "framebuffer.frag");
  Shader framebufferShader_w2("framebuffer_w2.vert", "framebuffer.frag");
  Shader mainComputeShader("main.comp");
  mainShader.setUniformTexture(2, 0);
  mainShader.setUniform2f(3, 1.f / vec2{OUTPUT_WIDTH, OUTPUT_HEIGHT});

  // ----- Screen triangles ----------------------------------- //

  float vertices[30] {
    -1.f, -1.f,  0.f,   0.f, 0.f,
    -1.f,  1.f,  0.f,   0.f, 1.f,
     1.f,  1.f,  0.f,   1.f, 1.f,
     1.f,  1.f,  0.f,   1.f, 1.f,
     1.f, -1.f,  0.f,   1.f, 0.f,
    -1.f, -1.f,  0.f,   0.f, 0.f
  };

  u32 vao;
  glGenVertexArrays(1, &vao);
  glBindVertexArray(vao);

  u32 vbo;
  glGenBuffers(1, &vbo);
  glBindBuffer(GL_ARRAY_BUFFER, vbo);
  glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);

  glEnableVertexAttribArray(0);
  glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)(0));
  glEnableVertexAttribArray(1);
  glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)(3 * sizeof(float)));

  // ----- Framebuffer ---------------------------------------- //

  GLuint fbo;
  glGenFramebuffers(1, &fbo);
  glBindFramebuffer(GL_FRAMEBUFFER, fbo);

	u32 framebufferTexture;
	glGenTextures(1, &framebufferTexture);
  glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, framebufferTexture);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RED, OUTPUT_WIDTH, OUTPUT_HEIGHT, 0, GL_RED, GL_UNSIGNED_BYTE, NULL);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE); // Prevents edge bleeding
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE); // Prevents edge bleeding
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, framebufferTexture, 0);

  // ---------------------------------------------------------- //

  MiniMesh borders = MiniMesh::loadShapefile("ne_10m_admin_0_countries_lakes");

  if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
    error("Incomplete framebuffer object");

  // Render loop
  while (!glfwWindowShouldClose(window)) {
    static double titleTimer = glfwGetTime();
    static double prevTime = titleTimer;
    static double currTime = prevTime;

    constexpr double fpsLimit = 1. / 90.;
    currTime = glfwGetTime();
    dt = currTime - prevTime;

    // FPS cap
    if (dt < fpsLimit) continue;
    else prevTime = currTime;

    ImGui_ImplOpenGL3_NewFrame();
    ImGui_ImplGlfw_NewFrame();
    ImGui::NewFrame();

    double mx, my;
    glfwGetCursorPos(window, &mx, &my);
    imguiHovered = isImguiHovered({mx, my});

    // Update window title every 0.3 seconds
    if (currTime - titleTimer >= 0.3) {
      u16 fps = static_cast<u16>(1.f / dt);
      glfwSetWindowTitle(window, std::format("FPS: {} / {:.5f} ms", fps, dt).c_str());
      titleTimer = currTime;
    }

    if (glfwGetKey(window, GLFW_KEY_Q) == GLFW_PRESS)
      glfwSetWindowShouldClose(window, GLFW_TRUE);

    ImGui::Begin("Settings");

    if (ImGui::Button("Save to borders.png (2560x1280)  ")) {
      produce(framebufferShader_w2, mainComputeShader, borders, 2560, 1280);
    }

    if (ImGui::Button("Save to borders.png (21600x10800)")) {
      produce(framebufferShader_w2, mainComputeShader, borders, 21600, 10800);
    }

    if (ImGui::Button("Reset View")) {
      scaleMat = mat4(1.f);
      translateMat = mat4(1.f);
    }

    glBindFramebuffer(GL_FRAMEBUFFER, fbo);
    glClearColor(0.f, 0.f, 0.f, 1.f);
    glClear(GL_COLOR_BUFFER_BIT);
    glViewport(0, 0, OUTPUT_WIDTH, OUTPUT_HEIGHT);

    framebufferShader.use();
    borders.draw();

    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    glClearColor(0.f, 0.f, 0.f, 1.f);
    glClear(GL_COLOR_BUFFER_BIT);
    glViewport(0, 0, 1600, 900);

    mainShader.setUniformMatrix4f(0, translateMat);
    mainShader.setUniformMatrix4f(1, scaleMat);

    glBindVertexArray(vao);
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, framebufferTexture);
    glDrawArrays(GL_TRIANGLES, 0, 6);

    glBindTexture(GL_TEXTURE_2D, 0);
    glBindVertexArray(0);

    ImGui::End();

    ImGui::Render();
    ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

    glfwSwapBuffers(window);
    glfwPollEvents();
  }

  ImGui_ImplOpenGL3_Shutdown();
  ImGui_ImplGlfw_Shutdown();
  ImGui::DestroyContext();
  glfwTerminate();

  return 0;
}

void scrollCallback(GLFWwindow* window, double xoffset, double yoffset) {
  static float prevScale = 1.f;
  float scale = prevScale + yoffset * scaleAmount;
  scale = std::clamp(scale, scaleLimit.x, scaleLimit.y);
  float scaleFactor = scale / prevScale;
  scaleMat = glm::scale(scaleMat, vec3(scaleFactor));
  prevScale = scale;
}

void mouseCursorCallback(GLFWwindow* window, double xpos, double ypos) {
  static bool isHoldingButton = false;
  static vec3 prevPos = {0.f, 0.f, 0.f};

  vec3 currPos = {xpos, ypos, 0.f};

  if (!imguiHovered) {
    if (glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_LEFT) == GLFW_PRESS) {
      vec3 toMove = (currPos - prevPos * glm::sign(currPos)) * dt * panMoveScale;
      toMove.y *= -1.f;
      translateMat = glm::translate(translateMat, toMove);
    }
  }

  prevPos = currPos;
}

bool isImguiHovered(const vec2& mouse) {
  auto& io = ImGui::GetIO();
  return io.WantCaptureMouse || io.WantCaptureKeyboard;
}

void produce(const Shader& drawShader, const Shader& compShader, const MiniMesh& borders, int w, int h) {
  int w2 = w / 2;
  int channels = 1;

  // ----- Framebuffer ---------------------------------------- //

  GLuint fbo;
  glGenFramebuffers(2, &fbo);
  glBindFramebuffer(GL_FRAMEBUFFER, fbo);

	u32 framebufferTexture;
	glGenTextures(1, &framebufferTexture);
  glActiveTexture(GL_TEXTURE1);
	glBindTexture(GL_TEXTURE_2D, framebufferTexture);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RED, w2, h, 0, GL_RED, GL_UNSIGNED_BYTE, NULL);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE); // Prevents edge bleeding
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE); // Prevents edge bleeding
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, framebufferTexture, 0);

  // ---------------------------------------------------------- //

  u32 texOutput;
  u32 texOutputUnit = 0;
  glGenTextures(1, &texOutput);
  glActiveTexture(GL_TEXTURE0 + texOutputUnit);
  glBindTexture(GL_TEXTURE_2D, texOutput);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
  glTexImage2D(GL_TEXTURE_2D, 0, GL_R8, w2, h, 0, GL_RED, GL_UNSIGNED_BYTE, NULL);
  glBindImageTexture(texOutputUnit, texOutput, 0, GL_FALSE, 0, GL_WRITE_ONLY, GL_R8);

  glBindFramebuffer(GL_FRAMEBUFFER, fbo);
  glViewport(0, 0, w2, h);

  byte* pixels = new byte[w2 * h * channels];
  stbi_flip_vertically_on_write(true);

  for (u32 i = 0; i < 2; i++) {
    glClearColor(0.f, 0.f, 0.f, 1.f);
    glClear(GL_COLOR_BUFFER_BIT);

    drawShader.setUniform1f(0, i);
    borders.draw();
    std::string fileName = std::format("borders{}_{}.png", w, i);

    printf("Creating %s (%dx%d)...\n", fileName.c_str(), w2, h);
    compShader.setUniform2f(0, 1.f / vec2{w2, h});
    glDispatchCompute(w2, h, 1);
    glMemoryBarrier(GL_SHADER_IMAGE_ACCESS_BARRIER_BIT);
    glActiveTexture(GL_TEXTURE0);
    glGetTexImage(GL_TEXTURE_2D, 0, GL_RED, GL_UNSIGNED_BYTE, pixels);
    stbi_write_png(fileName.c_str(), w2, h, channels, pixels, w2 * channels);
  }

  puts("Done");
  glBindTexture(GL_TEXTURE_2D, 0);
  glViewport(0, 0, 1600, 900);
  delete[] pixels;
}

