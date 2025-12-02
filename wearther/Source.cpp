// main.cpp
#include <glad/glad.h>
#define STB_IMAGE_IMPLEMENTATION
#include "image.h"

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
#include <string>
using json = nlohmann::json;

// ----------------------------------------------------------
// Texture Loader
// ----------------------------------------------------------
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




//-------------------------Get current time -------------------------------------

static std::string GetCurrentTimex()
{
    auto now = std::chrono::system_clock::now();
    std::time_t t = std::chrono::system_clock::to_time_t(now);

    std::tm local{};
    localtime_s(&local, &t);   // SAFE & warning-free

    char buffer[32];
    std::strftime(buffer, sizeof(buffer), "%H:%M:%S", &local);

    return std::string(buffer);
}


// ----------------------------------------------------------
// MAIN
// ----------------------------------------------------------
int main(int, char**)
{
    // ------------------------- Get Location -------------------------
    double lat = 0.0, lon = 0.0;
    std::string city;
    try {
        httplib::Client geo("http://ip-api.com");
        if (auto res = geo.Get("/json")) {
            if (res->status == 200) {
                json j = json::parse(res->body);
                lat = j.value("lat", 0.0);
                lon = j.value("lon", 0.0);
              city = j.value("city", "unknown");
                std::cout << "Location: " << city << std::endl;


                    std::cout<< " latitude =" <<lat << std::endl<<"longitude " << lon << ")\n";
            }
        }
    }
    catch (...) {
        std::cout << "Location fetch failed\n";
    }

    // ----------------------- Get Weather ----------------------------
    double temp;
    double wind;
    try {
        if (lat != 0.0 || lon != 0.0) {
            httplib::Client cli("http://api.open-meteo.com");
            std::ostringstream url;
            url << "/v1/forecast?latitude=" << lat
                << "&longitude=" << lon
                << "&current_weather=true";

            if (auto res = cli.Get(url.str().c_str())) {
                if (res->status == 200) {
                    json w = json::parse(res->body);
                    if (w.contains("current_weather")) {
                        temp = w["current_weather"].value("temperature", 0.0);
                         wind = w["current_weather"].value("windspeed", 0.0);
                        std::cout << "Temperature: " << temp << " C\n";
                        std::cout << "Wind Speed: " << wind << " km/h\n";
                    }
                }
            }
        }
    }
    catch (...) {
        std::cout << "Weather fetch failed\n";
    }

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
    ImGuiIO& io = ImGui::GetIO();

    ImGui::StyleColorsDark();
    ImGui_ImplSDL2_InitForOpenGL(window, gl_context);
    ImGui_ImplOpenGL3_Init("#version 330");

    stbi_set_flip_vertically_on_load(0);

    // ----------------------------------------------------------
    // Load Textures
    // ----------------------------------------------------------
    GLuint bgtex = LoadTexture("C:/Users/XTEND/Desktop/HTTP/wearther/x64/Debug/image1.jpg");
    GLuint icontex = LoadTexture("C:/Users/XTEND/Desktop/HTTP/wearther/wearther/assets/images/thunderstorm.png");

    // ----------------------------------------------------------
    // MAIN LOOP
    // ----------------------------------------------------------
    bool running = true;
    SDL_Event e;
    io.Fonts->AddFontDefault();
    ImFont* bigFont = io.Fonts->AddFontFromFileTTF(
        "C:/Users/XTEND/Desktop/HTTP/wearther/wearther/assets/fonts/ScienceGothic-Medium.ttf",
        8.0f
    );

    static std::string currentTime = GetCurrentTimex();
    static double lastUpdate = ImGui::GetTime();
    while (running) {
        while (SDL_PollEvent(&e)) {
            ImGui_ImplSDL2_ProcessEvent(&e);
            if (e.type == SDL_QUIT) running = false;
        }

        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplSDL2_NewFrame();
        ImGui::NewFrame();

        // ===== Background Fullscreen =====
        ImGui::SetNextWindowPos(ImVec2(0, 0));
        ImGui::SetNextWindowSize(io.DisplaySize);

        ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0, 0));
        ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0);
        ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 0);
        ImGui::PushStyleColor(ImGuiCol_WindowBg, ImVec4(0, 0, 0, 0));

        ImGui::Begin("bg", nullptr,
            ImGuiWindowFlags_NoDecoration |
            ImGuiWindowFlags_NoInputs |
            ImGuiWindowFlags_NoMove |
            ImGuiWindowFlags_NoResize |
            ImGuiWindowFlags_NoBringToFrontOnFocus);


        ImGui::Image((ImTextureID)(intptr_t)bgtex, io.DisplaySize);
        ImGui::End();

        ImGui::PopStyleColor();
        ImGui::PopStyleVar(3);

        // ===== Center Icon =====
        ImGui::SetNextWindowPos(ImVec2(0, 0));
        ImGui::SetNextWindowSize(io.DisplaySize);
        ImGui::Begin("center", nullptr,
            ImGuiWindowFlags_NoBackground |
            ImGuiWindowFlags_NoDecoration |
            ImGuiWindowFlags_NoInputs);

        ImVec2 iconSize(300, 200);
        ImVec2 pos(
            (io.DisplaySize.x - iconSize.x) * 0.5f,
            (io.DisplaySize.y - iconSize.y) * 0.5f - 150.0f
        );

        ImGui::SetCursorPos(pos);
        ImGui::Image((ImTextureID)(intptr_t)icontex, iconSize);

        ImGui::End();



        //========Time=====
        ImGui::SetNextWindowPos(ImVec2(0, 0));
        ImGui::SetNextWindowSize(io.DisplaySize);
        ImGui::SetNextWindowFocus();
        ImGui::Begin("Time", nullptr,
            ImGuiWindowFlags_NoBackground |
            ImGuiWindowFlags_NoDecoration );
        ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.0f, 0.0f, 0.0f, 1.0f));

        ImVec2 timepos(
            (io.DisplaySize.x - iconSize.x) * 0.5f,
            (io.DisplaySize.y - iconSize.y) * 0.5f +200
        );


        ImGui::SetWindowFontScale(6.0f);
        ImGui::SetCursorPos(timepos);
        double now = ImGui::GetTime();
        if (now - lastUpdate >= 1.0) {   // every 1 second
            currentTime = GetCurrentTimex();
            lastUpdate = now;
        }
        ImGui::Text("%s",currentTime.c_str());
        ImGui::PopStyleColor();

        //======================place======================
    
        ImGui::PushFont(bigFont);

       
 
        ImVec2 placepos(20.0f, 1.0f);
        ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.0f, 0.0f, 0.0f, 1.0f));
        ImGui::SetCursorPos(placepos);
        std::string loc = city;
        ImGui::Text("%s", loc.c_str());
   
        ImGui::PopFont();

        ImGui::PopStyleColor();

  

        //=========================== Weather =========================
        ImGui::PushFont(bigFont);
   
        std::string wind1 = std::to_string(wind);
        wind1 = wind1.substr(0, 3);

        wind1 = "Wind Speed  is " + wind1 + " km/hr";
        std::string temparature = std::to_string(temp);
        temparature = temparature.substr(0, 4);
        temparature = "Current Temparature " + temparature;

        std::string weather;
        weather = temparature + "\n"+wind1;



        ImVec2 ws = ImGui::CalcTextSize(weather.c_str());


        ImVec2 centerPos(io.DisplaySize.x * 0.5f,
            io.DisplaySize.y * 0.5f+30 );



        ImVec2 weatherpos(
            centerPos.x - (ws.x ) * 0.5f,
            centerPos.y - (ws.y) * 0.5f
        );


        ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.0f, 0.0f, 0.0f, 1.0f));
        ImGui::SetCursorPos(weatherpos);
  

        ImGui::Text("%s", weather.c_str());

        ImGui::PopFont();

        ImGui::PopStyleColor();




        ImGui::End();
        // ===== Render =====
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
