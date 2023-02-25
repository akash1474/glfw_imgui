#include "cmath"
#include "future"
#include <GL/glew.h>
#include <GLFW/glfw3.h>
#include "image_loader.h"

#include <chrono>
#include <iostream>

#include "imgui.h"
#include "imgui_impl_glfw.h"
#include "imgui_impl_opengl2.h"
#include "imgui_internal.h"
#include "implot.h"


#define WIDTH 640
#define HEIGHT 480


// Declare a struct to hold the two colors of the gradient
struct GradientColors {
    ImVec4 color1;
    ImVec4 color2;
};


// Function to generate the radial gradient texture
GLuint GenerateRadialGradientTexture(GradientColors colors, int width, int height, int centerX, int centerY, int x1, int y1)
{
    static unsigned char* data = nullptr;
    static bool spaceAllocated = false;
    if (!spaceAllocated) {
        // Allocate memory for the texture data
        data = new unsigned char[width * height * 4];
        spaceAllocated = true;
    }

    // Create a new OpenGL texture
    GLuint texture;
    glGenTextures(1, &texture);
    glBindTexture(GL_TEXTURE_2D, texture);

    // Generate the texture data
    for (int y = 0; y < height; y++) {
        for (int x = 0; x < width; x++) {
            // Calculate the distance from the center of the gradient
            float distance = sqrt((x - centerX) * (x - centerX) + (y - centerY) * (y - centerY));

            // Calculate the interpolation factor
            float t = distance / sqrt(centerX * centerX + centerY * centerY);

            // Interpolate the colors using the interpolation factor
            ImVec4 color = ImLerp(colors.color1, colors.color2, t);

            // Set the texture data for this pixel
            data[(y * width + x) * 4 + 0] = (unsigned char)(color.x * 255.0f);
            data[(y * width + x) * 4 + 1] = (unsigned char)(color.y * 255.0f);
            data[(y * width + x) * 4 + 2] = (unsigned char)(color.z * 255.0f);
            data[(y * width + x) * 4 + 3] = (unsigned char)(color.w * 255.0f);
        }
    }

    // Set the texture parameters
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

    // Upload the texture data to the GPU
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, data);

    return texture;
}

// Function to render the gradient in an ImGui window
void RenderGradient(GLuint texture, int width, int height)
{
    // Bind the texture
    glBindTexture(GL_TEXTURE_2D, texture);

    // Calculate the size of the gradient based on the window size
    ImVec2 size = ImVec2((float)width, (float)height);

    // Render the gradient using ImGui's image widget
    ImGui::Image((void*)(intptr_t)texture, size);
}

void AddRadialGradient(ImDrawList* draw_list, const ImVec2& center, float radius, ImU32 col_in, ImU32 col_out)
{
    if (((col_in | col_out) & IM_COL32_A_MASK) == 0 || radius < 0.5f) return;

    // Use arc with automatic segment count
    draw_list->_PathArcToFastEx(center, radius, 0, IM_DRAWLIST_ARCFAST_SAMPLE_MAX, 0);
    const int count = draw_list->_Path.Size - 1;

    unsigned int vtx_base = draw_list->_VtxCurrentIdx;
    draw_list->PrimReserve(count * 3, count + 1);

    // Submit vertices
    const ImVec2 uv = draw_list->_Data->TexUvWhitePixel;
    draw_list->PrimWriteVtx(center, uv, col_in);
    for (int n = 0; n < count; n++) draw_list->PrimWriteVtx(draw_list->_Path[n], uv, col_out);

    // Submit a fan of triangles
    for (int n = 0; n < count; n++) {
        draw_list->PrimWriteIdx((ImDrawIdx)(vtx_base));
        draw_list->PrimWriteIdx((ImDrawIdx)(vtx_base + 1 + n));
        draw_list->PrimWriteIdx((ImDrawIdx)(vtx_base + 1 + ((n + 1) % count)));
    }
    draw_list->_Path.Size = 0;
}


void AddLinearGradient(ImDrawList* draw_list, const ImVec2& a, const ImVec2& b, ImU32 col_a, ImU32 col_b)
{
    if (((col_a | col_b) & IM_COL32_A_MASK) == 0) return;

    draw_list->PathLineTo(a);
    draw_list->PathLineTo(b);
    const int count = draw_list->_Path.Size - 1;

    unsigned int vtx_base = draw_list->_VtxCurrentIdx;
    draw_list->PrimReserve(count * 3, count + 1);

    // Submit vertices
    const ImVec2 uv = draw_list->_Data->TexUvWhitePixel;
    draw_list->PrimWriteVtx(a, uv, col_a);
    for (int n = 0; n < count; n++) draw_list->PrimWriteVtx(draw_list->_Path[n], uv, col_b);

    // Submit a fan of triangles
    for (int n = 0; n < count; n++) {
        draw_list->PrimWriteIdx((ImDrawIdx)(vtx_base));
        draw_list->PrimWriteIdx((ImDrawIdx)(vtx_base + 1 + n));
        draw_list->PrimWriteIdx((ImDrawIdx)(vtx_base + 1 + ((n + 1) % count)));
    }
    draw_list->_Path.Size = 0;
}

int main(){
    GLFWwindow* window;
    GradientColors colors;
    colors.color1 = ImVec4(1.0f, 0.0f, 0.0f, 1.0f); // Red
    colors.color2 = ImVec4(0.0f, 0.0f, 1.0f, 1.0f); // Blue

    if (!glfwInit()) return -1;

    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 2);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 0);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_ANY_PROFILE);

    window = glfwCreateWindow(WIDTH, HEIGHT, "Hello World", NULL, NULL);
    if (!window) {
        glfwTerminate();
        return -1;
    }

    glfwMakeContextCurrent(window);
    if (glewInit() != GLEW_OK) std::cout << "Error" << std::endl;

    // Initialize ImGUI
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO();
    ImGui::StyleColorsDark();
    ImGui_ImplGlfw_InitForOpenGL(window, true);

    if (!ImGui_ImplOpenGL2_Init()) std::cout << "Failed to initit OpenGL 2" << std::endl;
    GLuint texture;
    ImageTexture* main_img=nullptr;
    bool isLoaded = false;
    bool isReady = false;
    int count=0;
    std::future<std::pair<ImageTexture*, cv::Mat*>> m_fut;
    while (!glfwWindowShouldClose(window)) {
        if (!isLoaded) {
            std::cout << "-- Async Launching\n";
            m_fut = std::async(std::launch::async, &ImageTexture::asyncTextureLoad, "./assets/brick.png", -1, true);
            isLoaded = true;
        }
        if (m_fut.valid() && m_fut.wait_for(std::chrono::milliseconds(20)) == std::future_status::ready) {
            auto s=std::chrono::high_resolution_clock::now();
            std::cout << "-- Async Done\n";
            auto x = m_fut.get();
            ImageTexture::bindTexture(main_img, x);
            isReady = true;
            std::cout << (std::chrono::high_resolution_clock::now()-s).count() << std::endl;
        }
        static float x_p = 0.0f;
        static float y_p = 0.0f;
        static int x1 = 0;
        static int y1 = 0;
        glClearColor(0.07f, 0.13f, 0.17f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        ImGui_ImplOpenGL2_NewFrame();
        ImGui_ImplGlfw_NewFrame();
        ImGui::NewFrame();
        // Render Other Stuff


        // Render Imgui Stuff
        RenderGradient(texture, 256, 256);
        ImGui::SetNextWindowBgAlpha(1.0);
        ImGui::Begin("Radial Controller", NULL);
        AddRadialGradient(ImGui::GetWindowDrawList(), {ImGui::GetWindowWidth() / 2.0f, ImGui::GetWindowSize().y / 2.0f}, 250.0f,
                          IM_COL32(40, 40, 40, 220), IM_COL32(20, 20, 20, 0));
        ImVec2 a(ImGui::GetWindowPos().x, ImGui::GetWindowPos().y);
        ImVec2 b(ImGui::GetWindowWidth(), ImGui::GetWindowHeight());
        ImU32 col_a = IM_COL32(255, 0, 0, 255); // Red
        ImU32 col_b = IM_COL32(0, 255, 0, 255); // Green

        // AddLinearGradient(ImGui::GetWindowDrawList(), a, b, col_a, col_b);
        static float color1[4] = {0.0, 0.1, 0.0, 1.0};
        static float color2[4] = {0.0, 0.1, 0.0, 1.0};
        if (ImGui::ColorEdit4("#ColorPicker1", color1) || ImGui::ColorEdit4("#ColorPicker2", color2)) {
            texture = GenerateRadialGradientTexture(colors, 256, 256, x_p, y_p, x1, y1);
        }
        ImGui::SliderFloat("controlX", &x_p, 0.0, 256.0f);
        ImGui::SliderFloat("controlY", &y_p, 0.0, 256.0f);
        ImGui::SliderInt("PosX", &x1, 0, 255);
        ImGui::SliderInt("PosY", &y1, 0, 255);

        colors.color1 = {color1[0], color1[1], color1[2], color1[3]};
        colors.color2 = {color2[0], color2[1], color2[2], color2[3]};

        ImGui::End();
        count++;

        ImGui::Begin("Image");
            ImGui::Text("%s",std::to_string(count).c_str());
        if (isReady) {
            ImGui::Image((void*)(intptr_t)main_img->texture, ImVec2(256, 256));
        }
        ImGui::End();


        // End of render
        ImGui::Render();
        ImGui_ImplOpenGL2_RenderDrawData(ImGui::GetDrawData());
        glfwSwapBuffers(window);
        glfwPollEvents();
    }
    delete main_img;
    ImGui_ImplOpenGL2_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();

    glfwDestroyWindow(window);
    glfwTerminate();
    return 0;
}