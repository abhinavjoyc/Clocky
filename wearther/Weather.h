#pragma once
#include <glad/glad.h>

#include "Weather.h"




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



void weathertab(ImGuiIO& io, std::vector<GLuint> &textures, ImFont* bigFont);
void RenderGlassTabs(int& activeTab);