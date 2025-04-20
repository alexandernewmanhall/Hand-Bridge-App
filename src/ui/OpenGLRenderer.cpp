#include "OpenGLRenderer.h"
#include <iostream>

// --- GLEW --- 
// IMPORTANT: Include GLEW before any standard OpenGL headers
// #define GLEW_STATIC // Define if linking static library (adjust if using DLL)
// --- END GLEW ---

// Windows.h must be included before OpenGL headers with proper defines
#ifdef _WIN32
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <Windows.h>
#endif

// Include OpenGL headers through SDL
#include <SDL_opengl.h>

OpenGLRenderer::OpenGLRenderer() {}

OpenGLRenderer::~OpenGLRenderer() {
    shutdown();
}

bool OpenGLRenderer::init(SDL_Window* window_, int width, int height) {
    window = window_;
    this->width = width;
    this->height = height;
    
    std::cout << "Initializing OpenGL renderer..." << std::endl;
    
    // Set OpenGL attributes before creating the context
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 3);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);
    SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);
    SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE, 24);
    
    // Create OpenGL context
    glContext = SDL_GL_CreateContext(window);
    if (!glContext) {
        std::cerr << "Failed to create OpenGL context: " << SDL_GetError() << std::endl;
        return false;
    }
    
    std::cout << "OpenGL context created successfully" << std::endl;
    
    // --- Make context current and Initialize GLEW ---
    OutputDebugStringA("OpenGLRenderer: Making context current...\n");
    if (SDL_GL_MakeCurrent(window, glContext) != 0) {
        OutputDebugStringA(("OpenGLRenderer: FAILED to make context current: " + std::string(SDL_GetError()) + "\n").c_str());
        std::cerr << "Failed to make OpenGL context current: " << SDL_GetError() << std::endl;
        SDL_GL_DeleteContext(glContext);
        glContext = nullptr;
        return false;
    }
    std::cout << "OpenGL context made current." << std::endl;

    /* 
    glewExperimental = GL_TRUE; // Needed for core profile
    OutputDebugStringA("OpenGLRenderer: Initializing GLEW...\n");
    GLenum glewError = glewInit();
    if (glewError != GLEW_OK) {
        const char* errorStr = reinterpret_cast<const char*>(glewGetErrorString(glewError));
        std::string errorMsg = "OpenGLRenderer: FAILED to initialize GLEW: " + (errorStr ? std::string(errorStr) : "Unknown Error") + "\n";
        OutputDebugStringA(errorMsg.c_str());
        std::cerr << "Failed to initialize GLEW: " << glewGetErrorString(glewError) << std::endl;
        SDL_GL_DeleteContext(glContext);
        glContext = nullptr;
        return false;
    }
    std::cout << "GLEW initialized successfully." << std::endl;
    OutputDebugStringA("OpenGLRenderer: GLEW initialized successfully.\n");
    */
    // --- END GLEW INIT ---

    // Set vsync
    if (SDL_GL_SetSwapInterval(1) < 0) {
        std::cerr << "Warning: Unable to set VSync! SDL Error: " << SDL_GetError() << std::endl;
    }
    
    // Initialize OpenGL
    glClearColor(0.1f, 0.1f, 0.1f, 1.0f);
    glEnable(GL_DEPTH_TEST);
    glDepthFunc(GL_LESS);
    
    // Initialize viewport
    glViewport(0, 0, width, height);
    
    // Output OpenGL version information
    const GLubyte* renderer = glGetString(GL_RENDERER);
    const GLubyte* version = glGetString(GL_VERSION);
    std::cout << "Renderer: " << (renderer ? (const char*)renderer : "unknown") << std::endl;
    std::cout << "OpenGL version: " << (version ? (const char*)version : "unknown") << std::endl;
    OutputDebugStringA(("OpenGLRenderer: Renderer: " + std::string(renderer ? (const char*)renderer : "unknown") + "\n").c_str());
    OutputDebugStringA(("OpenGLRenderer: OpenGL Version: " + std::string(version ? (const char*)version : "unknown") + "\n").c_str());
    
    return true;
}

void OpenGLRenderer::shutdown() {
    if (glContext) {
        std::cout << "Deleting OpenGL context..." << std::endl;
        SDL_GL_DeleteContext(glContext);
        glContext = nullptr;
    }
    
    // Don't destroy the window here as it's owned by UIManager
    window = nullptr;
}

bool OpenGLRenderer::initOpenGL() {
    // Get window size
    int width, height;
    SDL_GetWindowSize(window, &width, &height);
    
    std::cout << "Setting up OpenGL with viewport: " << width << "x" << height << std::endl;
    
    // Set viewport
    glViewport(0, 0, width, height);
    
    // Check for OpenGL errors
    GLenum error = glGetError();
    if (error != GL_NO_ERROR) {
        std::cerr << "OpenGL error setting viewport: " << error << std::endl;
        return false;
    }
    
    // Set clear color
    glClearColor(0.1f, 0.1f, 0.1f, 1.0f);
    
    error = glGetError();
    if (error != GL_NO_ERROR) {
        std::cerr << "OpenGL error setting clear color: " << error << std::endl;
        return false;
    }
    
    std::cout << "OpenGL initialization successful" << std::endl;
    return true;
}

void OpenGLRenderer::beginFrame() {
    // Clear the screen
    glClearColor(0.1f, 0.1f, 0.1f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    
    // Check for OpenGL errors
    GLenum error = glGetError();
    if (error != GL_NO_ERROR) {
        std::cerr << "OpenGL error during beginFrame: " << error << std::endl;
    }
}

void OpenGLRenderer::endFrame() {
    // Swap buffers
    SDL_GL_SwapWindow(window);
}

void OpenGLRenderer::handleResize(int width, int height) {
    std::cout << "Resizing OpenGL viewport to: " << width << "x" << height << std::endl;
    
    // Update viewport
    glViewport(0, 0, width, height);
    
    // Check for OpenGL errors
    GLenum error = glGetError();
    if (error != GL_NO_ERROR) {
        std::cerr << "OpenGL error during resize: " << error << std::endl;
    }
} 