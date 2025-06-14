#pragma once

#include "imgui.h"
#include "imgui_impl_glfw.h"
#include "imgui_impl_opengl3.h"
#include <imgui_internal.h>
#include <stdio.h>
#define GL_SILENCE_DEPRECATION
#if defined(IMGUI_IMPL_OPENGL_ES2)
#include <GLES2/gl2.h>
#endif
#include <glad/glad.h>
#include <GLFW/glfw3.h> // Will drag system OpenGL headers

// [Win32] Our example includes a copy of glfw3.lib pre-compiled with VS2010 to maximize ease of testing and compatibility with old VS compilers.
// To link with VS2010-era libraries, VS2015+ requires linking with legacy_stdio_definitions.lib, which we do using this pragma.
// Your own project should not be affected, as you are likely to link with a newer binary of GLFW that is adequate for your version of Visual Studio.
#if defined(_MSC_VER) && (_MSC_VER >= 1900) && !defined(IMGUI_DISABLE_WIN32_FUNCTIONS)
#pragma comment(lib, "legacy_stdio_definitions")
#endif

// This example can also compile and run with Emscripten! See 'Makefile.emscripten' for details.
#ifdef __EMSCRIPTEN__
#include "../libs/emscripten/emscripten_mainloop_stub.h"
#endif

#include <algorithm> // For std::max, std::min
#include <iostream> // For std::cout, std::cerr

//File read write lib

#include <FileReader.h>
#include <FileWriter.h>


//Custom widgets
#include <ImGuiimageloader.h>
#include <CustomWidget.h>


//Image container
#include <ImageContainer.h>

class Application {
public:
    Application(ImVec4 backgroundColor = ImVec4(0.45f, 0.55f, 0.60f, 1.00f));
    int Run();

    void ImportFiles(std::span<std::string> paths);
private:

    int InitWindow(GLFWwindow*& windowRet);
    static void glfw_error_callback(int error, const char* description);
    void BuildDockLayout();
    int OpenImages(const char* const* formatfilter, unsigned int filtercount, std::vector<std::string>& paths);

    void DisplayMenu();

    ImVec4 clearColor;
    ImGuiID g_viewport_id;

    ImGuiID g_ImageListView;

    ImGuiID g_docklefttemp;

    ImGuiID g_DebugView;
    ImGuiID g_ImagePreview;

    bool g_firstframe = true;
    bool g_useCompress = false;

    ImageManager Manager;

    int selection = -1;

    std::vector<char*> formatfilter;
};