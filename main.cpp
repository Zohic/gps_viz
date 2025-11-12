#include <iostream>
#include <filesystem>

#include <imgui.h>
#include <imgui_impl_glfw.h>
#include <imgui_impl_opengl3.h>
#include <imstb_rectpack.h>
#include <imgui_internal.h>

#include <implot.h>
#include <implot_internal.h>

#include <GLFW/glfw3.h>

#include "GL/gl.h"

#include "gps.h"
#include <thread>
//#include "dspl.h"

//extern "C" {
//    void* dspl_load();
//    void dspl_free(void*);
//}

#define IMGUI_IMPL_OPENGL_ES3

namespace ImGui {

    bool BufferingBar(const char* label, float value, const ImVec2& size_arg, const ImU32& bg_col, const ImU32& fg_col) {
        ImGuiWindow* window = GetCurrentWindow();
        if (window->SkipItems)
            return false;
        
        ImGuiContext& g = *GImGui;
        const ImGuiStyle& style = g.Style;
        const ImGuiID id = window->GetID(label);

        ImVec2 pos = window->DC.CursorPos;
        ImVec2 size = size_arg;
        size.x -= style.FramePadding.x * 2;

        const ImRect bb(pos, ImVec2(pos.x + size.x, pos.y + size.y));
        ItemSize(bb, style.FramePadding.y);
        if (!ItemAdd(bb, id))
            return false;

        // Render
        const float circleStart = size.x * 0.7f;
        const float circleEnd = size.x;
        const float circleWidth = circleEnd - circleStart;

        window->DrawList->AddRectFilled(bb.Min, ImVec2(pos.x + circleStart, bb.Max.y), bg_col);
        window->DrawList->AddRectFilled(bb.Min, ImVec2(pos.x + circleStart * value, bb.Max.y), fg_col);

        const float t = g.Time;
        const float r = size.y / 2;
        const float speed = 1.5f;

        const float a = speed * 0;
        const float b = speed * 0.333f;
        const float c = speed * 0.666f;

        const float o1 = (circleWidth + r) * (t + a - speed * (int)((t + a) / speed)) / speed;
        const float o2 = (circleWidth + r) * (t + b - speed * (int)((t + b) / speed)) / speed;
        const float o3 = (circleWidth + r) * (t + c - speed * (int)((t + c) / speed)) / speed;

        window->DrawList->AddCircleFilled(ImVec2(pos.x + circleEnd - o1, bb.Min.y + r), r, bg_col);
        window->DrawList->AddCircleFilled(ImVec2(pos.x + circleEnd - o2, bb.Min.y + r), r, bg_col);
        window->DrawList->AddCircleFilled(ImVec2(pos.x + circleEnd - o3, bb.Min.y + r), r, bg_col);
    }
}

static void glfw_error_callback(int error, const char* description)
{
    fprintf(stderr, "GLFW Error %d: %s\n", error, description);
}

struct WindowSpecs {
    float size_x{}, size_y{};
};

static WindowSpecs g_window;

float win_rel_x(float v) {
    return v * g_window.size_x;
}
float win_rel_y(float v) {
    return v * g_window.size_y;
}

static unsigned int back_color = ImColor(10, 100, 100).operator unsigned int();
static unsigned int front_color = ImColor(100, 200, 250).operator unsigned int();
const char* filename = "C:\\Users\\zohich\\Desktop\\school\\master_degree\\gps\\30_08_2018__19_38_33_x02_1ch_16b_15pos_90000ms.dat";

struct app_logic {
    gps::data_proc data_proc;
    std::atomic<float> progress;
    bool loading = false;
    std::vector<char> ca_code;
    char* ca_code_selected;
    gps::system_params _params {};

    gps::detector _detector;

    app_logic(): _detector(_params) {
        _params.accum_count = 20;
        ca_code_selected = new char[3];
    }
    bool CaCodeWindow(bool* open, ImGuiIO& io) {
        
        ImGui::Begin("CA Code Graph", open);
        if (ca_code.size() != 0) {
            if (ImPlot::BeginPlot("CA Code Values")) {
                ImPlot::PlotStairs("spectrum values", (signed char*)ca_code.data(), ca_code.size());
                ImPlot::EndPlot();
            }
        }
        ImGui::End();
        
        ImGui::Begin("CA Code", open);
        
        if (ImGui::BeginCombo("ca code number", ca_code_selected, ImGuiComboFlags_NoArrowButton))
        {
            for (int n = 1; n <= 37; n++)
            {
                auto n_s = std::to_string(n);
                char* cur_v = new char[n_s.size()+1];
                memcpy(cur_v, n_s.data(), n_s.size());
                cur_v[n_s.size()] = '\0';

                bool is_selected = std::strcmp(ca_code_selected, cur_v);
                if (ImGui::Selectable(std::to_string(n).c_str(), is_selected))
                    strcpy(ca_code_selected, cur_v);
                if (is_selected)
                    ImGui::SetItemDefaultFocus();

                delete[] cur_v;
            }
            ImGui::EndCombo();
        }

        if (ImGui::Button("Generate Ca Code")) {
            int ind = std::stoi(ca_code_selected);
            auto code = gps::GetCACodeFast(ind, 1);
            if (code.isOk())
                ca_code = code.GetValue().code;
            else
                std::cout << code.GetError();
        }

        ImGui::End();
        return true;
    }

    bool main_logic(bool* open, ImGuiIO& io) {
        // 1. Show the big demo window (Most of the sample code is in ImGui::ShowDemoWindow()! You can browse its code to learn more about Dear ImGui!).
        //if (false)
            //ImGui::ShowDemoWindow(&show_demo_window);
        

        ImGui::Begin("Start menu", open);    // Create a window called "Hello, world!" and append into it.

        if (loading) {
            ImGui::Text(gps::cat("loading from file: ", progress.load()).c_str());
            ImGui::BufferingBar("loading data", progress, ImVec2(300, 10), back_color, front_color);
        }

        if (!loading) {
            if (ImGui::Button("Load File")) {
                loading = true;
                std::thread load_thread([this]() {
                    auto res = data_proc.load<int16_t>(filename, 30, [this](size_t done, size_t total) {
                        float v = (float)done / total;

                        if (done == total)
                            loading = false;

                        progress = v;
                        //std::cout << gps::cat("done reading: ", v, "%\n");
                        });
                    if (!res.isOk())
                        std::cout << res.GetError();
                    });
                load_thread.detach();
            }
            if (ImGui::Button("detect")) {
                std::thread load_thread([this]() {_detector.CalcCACorrs(data_proc); });
                load_thread.join();
            }
        }


        ImGui::Text("Application average %.3f ms/frame (%.1f FPS)", 1000.0f / io.Framerate, io.Framerate);
        ImGui::SetWindowPos(ImVec2(win_rel_x(0.1f), win_rel_y(0.1f)));
        /*if (ImPlot::BeginPlot("input")) {
            ImPlot::PlotLine("signal values", (double*)nullptr, (double*)nullptr, 0);
            ImPlot::EndPlot();
        }

        if (ImPlot::BeginPlot("output")) {
            ImPlot::PlotStems("spectrum values", (double*)nullptr, (double*)nullptr, 0);
            ImPlot::EndPlot();
        }*/

        ImGui::End();
        
        CaCodeWindow(open, io);

        return true;
    }
};



// Main code
int main(int argc, char** argv)
{
    auto ca_code = gps::GetCACodeFast(1, 1);
    void* dspl_lib = dspl_load();

    if (!dspl_lib)
        return 1;
    if (dspl_lib)
        printf("dspl loaded\n");

    random_t rnd = { 0 };     /* random structure   */
    constexpr int N = 500;
    constexpr int P = 490;
    double x[N];
    double z[2 * P + 1];
    double t[2 * P + 1];

    volatile int err;

    err = random_init(&rnd, RAND_TYPE_MRG32K3A, NULL);
    if (err != RES_OK)
        return 1;

    err = randn(x, N, 0.0, 1.0, &rnd);
    if (err != RES_OK)
        return 1;

    err = xcorr(x, N, x, N, DSPL_XCORR_UNBIASED, P, z, t);


    glfwSetErrorCallback(glfw_error_callback);
    if (!glfwInit())
        return 1;
    
    
    
        // GL ES 3.0 + GLSL 300 es (WebGL 2.0)
    const char* glsl_version = "#version 300 es";
    //glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 2);
    //glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 0);
    //glfwWindowHint(GLFW_CLIENT_API, GLFW_OPENGL_ES_API);

    // Create window with graphics context
    GLFWwindow* window = glfwCreateWindow(1280, 720, "Dear ImGui GLFW+OpenGL3 example", nullptr, nullptr);
    g_window.size_x = 1270;
    g_window.size_y = 720;

    if (window == nullptr)
        return 1;
    glfwMakeContextCurrent(window);
    glfwSwapInterval(1); // Enable vsync

    // Setup Dear ImGui context
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImPlot::CreateContext();
    ImGuiIO& io = ImGui::GetIO(); (void)io;
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;     // Enable Keyboard Controls
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;      // Enable Gamepad Controls

    // Setup Dear ImGui style
    ImGui::StyleColorsDark();
    //ImGui::StyleColorsLight();

    // Setup Platform/Renderer backends
    ImGui_ImplGlfw_InitForOpenGL(window, true);

    ImGui_ImplOpenGL3_Init(glsl_version);

    // Load Fonts
    // - If no fonts are loaded, dear imgui will use the default font. You can also load multiple fonts and use ImGui::PushFont()/PopFont() to select them.
    // - AddFontFromFileTTF() will return the ImFont* so you can store it if you need to select the font among multiple.
    // - If the file cannot be loaded, the function will return a nullptr. Please handle those errors in your application (e.g. use an assertion, or display an error and quit).
    // - The fonts will be rasterized at a given size (w/ oversampling) and stored into a texture when calling ImFontAtlas::Build()/GetTexDataAsXXXX(), which ImGui_ImplXXXX_NewFrame below will call.
    // - Use '#define IMGUI_ENABLE_FREETYPE' in your imconfig file to use Freetype for higher quality font rendering.
    // - Read 'docs/FONTS.md' for more instructions and details.
    // - Remember that in C/C++ if you want to include a backslash \ in a string literal you need to write a double backslash \\ !
    // - Our Emscripten build process allows embedding fonts to be accessible at runtime from the "fonts/" folder. See Makefile.emscripten for details.
    //io.Fonts->AddFontDefault();
    //io.Fonts->AddFontFromFileTTF("c:\\Windows\\Fonts\\segoeui.ttf", 18.0f);
    //io.Fonts->AddFontFromFileTTF("../../misc/fonts/DroidSans.ttf", 16.0f);
    //io.Fonts->AddFontFromFileTTF("../../misc/fonts/Roboto-Medium.ttf", 16.0f);
    //io.Fonts->AddFontFromFileTTF("../../misc/fonts/Cousine-Regular.ttf", 15.0f);
    //ImFont* font = io.Fonts->AddFontFromFileTTF("C:/Windows/Fonts/RAVIE.TTF", 18.0f);
    //IM_ASSERT(font != nullptr);

    // Our state
    bool show_demo_window = false;
    bool show_another_window = false;
    bool show_main_window = true;
    ImVec4 clear_color = ImVec4(0.0f, 0.0f, 0.1f, 1.00f);

    app_logic app;

    // Main loop

    while (!glfwWindowShouldClose(window))
    {
        // Poll and handle events (inputs, window resize, etc.)
        // You can read the io.WantCaptureMouse, io.WantCaptureKeyboard flags to tell if dear imgui wants to use your inputs.
        // - When io.WantCaptureMouse is true, do not dispatch mouse input data to your main application, or clear/overwrite your copy of the mouse data.
        // - When io.WantCaptureKeyboard is true, do not dispatch keyboard input data to your main application, or clear/overwrite your copy of the keyboard data.
        // Generally you may always pass all inputs to dear imgui, and hide them from your application based on those two flags.
        glfwPollEvents();
        if (glfwGetWindowAttrib(window, GLFW_ICONIFIED) != 0)
        {
            ImGui_ImplGlfw_Sleep(10);
            continue;
        }

        // Start the Dear ImGui frame
        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplGlfw_NewFrame();
        ImGui::NewFrame();

        app.main_logic(&show_main_window, io);

        // Rendering
        ImGui::Render();
        int display_w, display_h;
        glfwGetFramebufferSize(window, &display_w, &display_h);
        glViewport(0, 0, display_w, display_h);
        glClearColor(clear_color.x * clear_color.w, clear_color.y * clear_color.w, clear_color.z * clear_color.w, clear_color.w);
        glClear(GL_COLOR_BUFFER_BIT);
        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

        glfwSwapBuffers(window);
    }

    // Cleanup
    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplGlfw_Shutdown();

    ImPlot::DestroyContext();
    ImGui::DestroyContext();

    glfwDestroyWindow(window);
    glfwTerminate();
    dspl_free(dspl_lib);

    return 0;
}