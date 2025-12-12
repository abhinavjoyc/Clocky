// main.cpp
#include <glad/glad.h>
#define STB_IMAGE_IMPLEMENTATION
#include "image.h"
//
#include <SDL.h>
#include <SDL_opengl.h>

#include "imgui.h"
#include "imgui_impl_sdl2.h"
#include "imgui_impl_opengl3.h"

#include "httplib.h"
#include "json.hpp"

#include <iostream>
#include <string>
#include <sstream>


#include<chrono>
#include<ctime>


#include <SDL_mixer.h>




#include "Weather.h"
#include "pomedoro.h"
using json = nlohmann::json;

static GLuint LoadTexture(const char* filename)
{
    if (!filename) {
        std::cerr << "LoadTexture: filename is null\n";
        return 0;
    }

    int width = 0, height = 0, channels = 0;
    unsigned char* data =
        stbi_load(filename, &width, &height, &channels, STBI_rgb_alpha);

    if (!data) {
        std::cerr << "LoadTexture: failed to load " << filename << "\n";
        return 0;
    }

    GLuint tex = 0;
    glGenTextures(1, &tex);
    glBindTexture(GL_TEXTURE_2D, tex);

    glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

    glTexImage2D(
        GL_TEXTURE_2D, 0, GL_RGBA8,
        width, height, 0,
        GL_RGBA, GL_UNSIGNED_BYTE, data
    );

    glGenerateMipmap(GL_TEXTURE_2D);

    glBindTexture(GL_TEXTURE_2D, 0);
    stbi_image_free(data);

    std::cout << "Loaded texture: " << filename
        << " (" << width << "x" << height << ")\n";
    return tex;
}




void RenderCustomTabs(int& activeTab)
{
    // Style for rounded buttons
    ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, 14.0f);
    ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(18, 12));

    // Light button background
    ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.85f, 0.85f, 0.85f, 0.15f));
    ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.85f, 0.85f, 0.85f, 0.30f));
    ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0.20f, 0.20f, 0.20f, 0.90f));

    // Center the tab group horizontally
    float width = ImGui::GetWindowWidth();
    ImGui::SetCursorPosX((width - 360) * 0.5f);

    // ---- TAB 0 : WEATHER ----
    ImGui::PushStyleColor(ImGuiCol_Text,
        activeTab == 0 ? ImVec4(0, 0, 0, 1) : ImVec4(0.4f, 0.4f, 0.4f, 1));

    if (ImGui::Button("Pomodoro", ImVec2(110, 40))) {
        std::cout << "[TAB CLICKED] -> pomodoro\n";
        activeTab = 0;
    }
    ImGui::PopStyleColor();

    ImGui::SameLine();

    // ---- TAB 1 : DETAILS ----
    ImGui::PushStyleColor(ImGuiCol_Text,
        activeTab == 1 ? ImVec4(0, 0, 0, 1) : ImVec4(0.4f, 0.4f, 0.4f, 1));

    if (ImGui::Button("Weather", ImVec2(110, 40))) {
        std::cout << "[TAB CLICKED] -> Wether\n";
        activeTab = 1;
    }
    ImGui::PopStyleColor();

    ImGui::SameLine();

    // ---- TAB 2 : SETTINGS ----
    ImGui::PushStyleColor(ImGuiCol_Text,
        activeTab == 2 ? ImVec4(0, 0, 0, 1) : ImVec4(0.4f, 0.4f, 0.4f, 1));

    if (ImGui::Button("Settings", ImVec2(110, 40))) {
        std::cout << "[TAB CLICKED] -> Settings\n";
        activeTab = 2;
    }
    ImGui::PopStyleColor();

    // POP STYLES
    ImGui::PopStyleColor(3);
    ImGui::PopStyleVar(2);
}





// ----------------------------------------------------------
// MAIN
// ----------------------------------------------------------
int main(int, char**)
{

    // ----------------------------------------------------------
    // SDL INIT
    // ----------------------------------------------------------
    if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_TIMER) != 0) {
        std::cerr << "SDL_Init failed: " << SDL_GetError() << "\n";
        return -1;
    }

    SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 3);

    SDL_Window* window = SDL_CreateWindow(
        "Weather UI",
        SDL_WINDOWPOS_CENTERED,
        SDL_WINDOWPOS_CENTERED,
        1280, 720,
        SDL_WINDOW_OPENGL | SDL_WINDOW_RESIZABLE
    );

    SDL_GLContext gl_context = SDL_GL_CreateContext(window);
    SDL_GL_MakeCurrent(window, gl_context);
    SDL_GL_SetSwapInterval(1);

    if (!gladLoadGLLoader((GLADloadproc)SDL_GL_GetProcAddress)) {
        std::cerr << "Failed to init GLAD\n";
        return -1;
    }

    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    // ----------------------------------------------------------
    // ImGui INIT
    // ----------------------------------------------------------
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiStyle& style = ImGui::GetStyle();


    style.Colors[ImGuiCol_Button] = ImVec4(0, 0, 0, 0);
    style.Colors[ImGuiCol_ButtonHovered] = ImVec4(0, 0, 0, 0);
    style.Colors[ImGuiCol_ButtonActive] = ImVec4(0, 0, 0, 0);
    style.FrameBorderSize = 0.0f;

    ImGuiIO& io = ImGui::GetIO();
    io.IniFilename = NULL;
    io.ConfigFlags &= ~ImGuiConfigFlags_NavEnableKeyboard;


    ImGui_ImplSDL2_InitForOpenGL(window, gl_context);
    ImGui_ImplOpenGL3_Init("#version 330");

   

  

    // ----------------------------------------------------------
    // MAIN LOOP
    // ----------------------------------------------------------
    bool running = true;
    SDL_Event e;
    GLuint bgtex = LoadTexture("C:/Users/XTEND/Desktop/HTTP/wearther/x64/Debug/image1.jpg");
    GLuint icontex = LoadTexture("C:/Users/XTEND/Desktop/HTTP/wearther/wearther/assets/images/thunderstorm.png");
    GLuint clockTex = LoadTexture("C:/Users/XTEND/Desktop/HTTP/wearther/wearther/assets/images/stopwatch.png");
    GLuint arrowTex = LoadTexture("C:/Users/XTEND/Desktop/HTTP/wearther/wearther/assets/images/arrow.jpg");
    GLuint startTex = LoadTexture("C:/Users/XTEND/Desktop/HTTP/wearther/wearther/assets/images/start.png");
    GLuint stopTex = LoadTexture("C:/Users/XTEND/Desktop/HTTP/wearther/wearther/assets/images/stop.png");
    GLuint pauseTex = LoadTexture("C:/Users/XTEND/Desktop/HTTP/wearther/wearther/assets/images/pause.png");
    GLuint resetTex = LoadTexture("C:/Users/XTEND/Desktop/HTTP/wearther/wearther/assets/images/reset.png");
    io.Fonts->AddFontDefault();

    ImFont* bigFont = io.Fonts->AddFontFromFileTTF(
        "C:/Users/XTEND/Desktop/HTTP/wearther/wearther/assets/fonts/ScienceGothic-Medium.ttf",
        8.0f
    );



    std::vector<GLuint> textures;
    textures.push_back(bgtex);
    textures.push_back(icontex);
    textures.push_back(clockTex);
    textures.push_back(arrowTex);
    textures.push_back(startTex);
    textures.push_back(stopTex);
    textures.push_back(pauseTex);
    textures.push_back(resetTex);
    int activeTab = 0;   
    while (running)
    {
        while (SDL_PollEvent(&e)) {
            ImGui_ImplSDL2_ProcessEvent(&e);
            if (e.type == SDL_QUIT) running = false;
        }

        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplSDL2_NewFrame();
        ImGui::NewFrame();
       
        if (activeTab == 0)

        {
            ImGui::SetWindowFontScale(6.0f);
           PomederoTab (io,textures,bigFont);


        }
      
        if (activeTab == 1)
        {
            ImGui::SetWindowFontScale(6.0f);
       weathertab(io, textures, bigFont);
            // -----------------------------------------------------------
            // 8. TABS BAR
            // -----------------------------------------------------------
           
        }
        ImGui::SetNextWindowPos({ 0, 6 });
        ImGui::SetNextWindowSize({ io.DisplaySize.x, 80 });

        ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, { 0, 6 });
        ImGui::PushStyleColor(ImGuiCol_WindowBg, { 0, 0, 0, 0 });

        ImGui::Begin("Tabs", nullptr,
            ImGuiWindowFlags_NoDecoration |
            ImGuiWindowFlags_NoMove |
            ImGuiWindowFlags_NoResize |
            ImGuiWindowFlags_NoBackground |
            ImGuiWindowFlags_NoNavFocus |          // <-- FIX HERE
            ImGuiWindowFlags_NoFocusOnAppearing    // <-- FIX HERE
        );


        RenderCustomTabs(activeTab);


        ImGui::End();

        ImGui::PopStyleColor();
        ImGui::PopStyleVar();

        //================= RENDER =================
        ImGui::Render();
        glViewport(0, 0, (int)io.DisplaySize.x, (int)io.DisplaySize.y);
        glClear(GL_COLOR_BUFFER_BIT);
        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
        SDL_GL_SwapWindow(window);
    }


    // ---------------- Cleanup ----------------
    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplSDL2_Shutdown();
    ImGui::DestroyContext();

    SDL_GL_DeleteContext(gl_context);
    SDL_DestroyWindow(window);
    SDL_Quit();

    return 0;
}
