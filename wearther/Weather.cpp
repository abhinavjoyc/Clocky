#include "Weather.h"
#include <glad/glad.h>
#include <SDL.h>
#include <SDL_opengl.h>

#include "imgui.h"
#include "imgui_impl_sdl2.h"
#include "imgui_impl_opengl3.h"

#include <iostream>
#include <vector>
#include <string>
#include <chrono>

#include "http.h"
#include "time.h"

// -----------------------------------------------------------
// GLOBAL CACHES (FOR HIGH PERFORMANCE)
// -----------------------------------------------------------

// LOCATION CACHE
static bool locationLoaded = false;
static loc1 cachedLocation;
static double lastLocationFetch = 0.0; // fetch every minute

// WEATHER CACHE
static bool weatherLoaded = false;
static climate cachedWeather;
static double lastWeatherFetch = 0.0;

// TIME CACHE (UI ONLY)
static std::string currentTime = "00:00:00";
static double lastTimeUpdate = 0.0;
std::string city = "unknown";






// -----------------------------------------------------------
// MAIN UI FUNCTION
// -----------------------------------------------------------
void weathertab(ImGuiIO& io, std::vector<GLuint>& textures, ImFont* bigFont)
{
    // -----------------------------------------------------------
    // 1. BACKGROUND IMAGE (IMMOVABLE)
    // -----------------------------------------------------------
    ImGui::SetNextWindowPos({ 0, 0 });
    ImGui::SetNextWindowSize(io.DisplaySize);

    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, { 0, 0 });
    ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 0);
    ImGui::PushStyleColor(ImGuiCol_WindowBg, { 0, 0, 0, 0 });

    ImGui::Begin("BG", nullptr,
        ImGuiWindowFlags_NoDecoration |
        ImGuiWindowFlags_NoInputs |
        ImGuiWindowFlags_NoMove |
        ImGuiWindowFlags_NoResize |
        ImGuiWindowFlags_NoScrollbar |
        ImGuiWindowFlags_NoScrollWithMouse |
        ImGuiWindowFlags_NoSavedSettings);

    ImGui::Image((ImTextureID)(intptr_t)textures[0], io.DisplaySize);
    ImGui::End();

    ImGui::PopStyleColor();
    ImGui::PopStyleVar(3);


    // -----------------------------------------------------------
    // 2. FETCH LOCATION EVERY 60 SECONDS
    // -----------------------------------------------------------
    double now = ImGui::GetTime();

    if (!locationLoaded || (now - lastLocationFetch >= 60.0)) {
        cachedLocation = getpos();
        locationLoaded = true;
        lastLocationFetch = now;
    }


    // -----------------------------------------------------------
    // 3. FETCH WEATHER EVERY 60 SECONDS
    // -----------------------------------------------------------
    if (!weatherLoaded || (now - lastWeatherFetch >= 60.0)) {
        cachedWeather = Getweather(cachedLocation.lat, cachedLocation.lon);
        city = GetCity();
        weatherLoaded = true;
        lastWeatherFetch = now;
    }


    // -----------------------------------------------------------
    // 4. ICON (IMMOVABLE)
    // -----------------------------------------------------------
    ImGui::SetNextWindowPos({ 0, 0 });
    ImGui::SetNextWindowSize(io.DisplaySize);

    ImGui::Begin("Icon", nullptr,
        ImGuiWindowFlags_NoDecoration |
        ImGuiWindowFlags_NoBackground |
        ImGuiWindowFlags_NoInputs |
        ImGuiWindowFlags_NoMove |
        ImGuiWindowFlags_NoResize |
        ImGuiWindowFlags_NoScrollbar |
        ImGuiWindowFlags_NoScrollWithMouse |
        ImGuiWindowFlags_NoSavedSettings);

    ImVec2 iconSize(300, 200);
    ImVec2 iconPos(
        (io.DisplaySize.x - iconSize.x) * 0.5f,
        (io.DisplaySize.y - iconSize.y) * 0.5f - 150
    );

    ImGui::SetCursorPos(iconPos);
    ImGui::Image((ImTextureID)(intptr_t)textures[1], iconSize);

    ImGui::End();


    // -----------------------------------------------------------
    // 5. WEATHER TEXT (IMMOVABLE)
    // -----------------------------------------------------------
    ImGui::Begin("WeatherText", nullptr,
        ImGuiWindowFlags_NoDecoration |
        ImGuiWindowFlags_NoBackground |
        ImGuiWindowFlags_NoMove |
        ImGuiWindowFlags_NoResize |
        ImGuiWindowFlags_NoScrollbar |
        ImGuiWindowFlags_NoScrollWithMouse |
        ImGuiWindowFlags_NoSavedSettings);

    ImGui::PushFont(bigFont);
    ImGui::PushStyleColor(ImGuiCol_Text, { 0, 0, 0, 1 });
    ImGui::SetWindowFontScale(6.0f);

    double wind = cachedWeather.wind;
    double temp = cachedWeather.temp;

    std::string windStr = "Wind Speed: " + std::to_string(wind).substr(0, 3) + " km/hr";
    std::string tempStr = "Temperature: " + std::to_string(temp).substr(0, 4) + " C";

    std::string weatherText = tempStr + "\n" + windStr;

    ImVec2 ws = ImGui::CalcTextSize(weatherText.c_str());
    ImGui::SetCursorPos({
        io.DisplaySize.x * 0.5f - ws.x * 0.5f,
        io.DisplaySize.y * 0.5f + 30
        });

    ImGui::Text("%s", weatherText.c_str());

    ImGui::PopStyleColor();
    ImGui::PopFont();
    ImGui::End();


    // -----------------------------------------------------------
    // 6. TIME (IMMOVABLE)
    // -----------------------------------------------------------
    ImGui::Begin("Time", nullptr,
        ImGuiWindowFlags_NoDecoration |
        ImGuiWindowFlags_NoBackground |
        ImGuiWindowFlags_NoInputs |
        ImGuiWindowFlags_NoMove |
        ImGuiWindowFlags_NoResize |
        ImGuiWindowFlags_NoScrollbar |
        ImGuiWindowFlags_NoScrollWithMouse |
        ImGuiWindowFlags_NoSavedSettings);

    ImGui::PushStyleColor(ImGuiCol_Text, { 0, 0, 0, 1 });
    ImGui::SetWindowFontScale(6.0f);

    if (now - lastTimeUpdate >= 1.0) {
        currentTime = GetCurrentTimex();
        lastTimeUpdate = now;
    }

    ImGui::SetCursorPos({
        (io.DisplaySize.x - 300) * 0.5f,
        (io.DisplaySize.y - 200) * 0.5f + 250
        });

    ImGui::Text("%s", currentTime.c_str());
    ImGui::PopStyleColor();
    ImGui::End();


    // -----------------------------------------------------------
    // 7. CITY NAME (IMMOVABLE)
    // -----------------------------------------------------------
    ImGui::Begin("City", nullptr,
        ImGuiWindowFlags_NoDecoration |
        ImGuiWindowFlags_NoBackground |
        ImGuiWindowFlags_NoMove |
        ImGuiWindowFlags_NoResize |
        ImGuiWindowFlags_NoScrollbar |
        ImGuiWindowFlags_NoScrollWithMouse |
        ImGuiWindowFlags_NoSavedSettings);

    ImGui::PushFont(bigFont);
    ImGui::PushStyleColor(ImGuiCol_Text, { 0, 0, 0, 1 });
    ImGui::SetWindowFontScale(4.0f);

    ImGui::SetCursorPos({ 20, 1 });
    ImGui::Text("%s", city.c_str());

    ImGui::PopStyleColor();
    ImGui::PopFont();
    ImGui::End();
}

