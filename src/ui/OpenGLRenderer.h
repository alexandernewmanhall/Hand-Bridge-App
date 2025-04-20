#pragma once

#include <SDL.h>
#include <string>
#include <vector>

class OpenGLRenderer {
public:
    OpenGLRenderer();
    ~OpenGLRenderer();

    // Initialization and shutdown
    bool init(SDL_Window* window, int width = 800, int height = 600);
    void shutdown();
    
    // Begin and end frame rendering
    void beginFrame();
    void endFrame();
    
    // Handle window resize
    void handleResize(int width, int height);
    
    // Access to the GL context
    SDL_GLContext getGLContext() { return glContext; }

private:
    // SDL window reference (owned by UIManager)
    SDL_Window* window = nullptr;
    
    // OpenGL context
    SDL_GLContext glContext = nullptr;
    
    // Window dimensions
    int width = 800;
    int height = 600;
    
    // Initialize OpenGL
    bool initOpenGL();
}; 