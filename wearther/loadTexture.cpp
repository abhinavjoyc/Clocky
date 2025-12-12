#include <glad/glad.h>
#define STB_IMAGE_IMPLEMENTATION
#include "image.h"

#include <SDL.h>
#include <SDL_opengl.h>

#include "imgui.h"
#include "imgui_impl_sdl2.h"
#include "imgui_impl_opengl3.h"



#include <iostream>
#include <string>
#include <sstream>

#include "loadTexture.h"


static GLuint loadTexture(const char* filename)
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
