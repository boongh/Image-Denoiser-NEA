#include "Application.h"

Application::Application(ImVec4 backgroundColor) {
    clearColor = backgroundColor;
    return;
}

void Application::glfw_error_callback(int error, const char* description) {
    fprintf(stderr, "GLFW Error %d: %s\n", error, description);
}

int Application::InitWindow(GLFWwindow*& windowRet) {
    glfwSetErrorCallback(glfw_error_callback);
    if (!glfwInit())
        return 1;

    // Decide GL+GLSL versions
#if defined(IMGUI_IMPL_OPENGL_ES2)
    // GL ES 2.0 + GLSL 100 (WebGL 1.0)
    const char* glsl_version = "#version 100";
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 2);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 0);
    glfwWindowHint(GLFW_CLIENT_API, GLFW_OPENGL_ES_API);
#elif defined(IMGUI_IMPL_OPENGL_ES3)
    // GL ES 3.0 + GLSL 300 es (WebGL 2.0)
    const char* glsl_version = "#version 300 es";
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 0);
    glfwWindowHint(GLFW_CLIENT_API, GLFW_OPENGL_ES_API);
#elif defined(__APPLE__)
    // GL 3.2 + GLSL 150
    const char* glsl_version = "#version 150";
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 2);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);  // 3.2+ only
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);            // Required on Mac
#else
    // GL 3.0 + GLSL 130
    const char* glsl_version = "#version 130";
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);  // 3.2+ only
    //glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);            // 3.0+ only
#endif

    // Create window with graphics context
    windowRet = glfwCreateWindow(1280, 720, "Main Program", nullptr, nullptr);
    if (windowRet == nullptr)
        return 1;
    glfwMakeContextCurrent(windowRet);

    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress)) {
        std::cerr << "Failed to initialize GLAD" << std::endl;
        return -1;
    }

    glfwSwapInterval(1); // Enable vsync

    // Setup Dear ImGui context
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO(); (void)io;
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;     // Enable Keyboard Controls
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;      // Enable Gamepad Controls
    io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;         // Enable Docking
    io.ConfigFlags |= ImGuiConfigFlags_ViewportsEnable;       // Enable Multi-Viewport / Platform Windows
    //io.ConfigViewportsNoAutoMerge = true;
    //io.ConfigViewportsNoTaskBarIcon = true;

    // Setup Dear ImGui style
    ImGui::StyleColorsDark();
    //ImGui::StyleColorsLight();

    // When viewports are enabled we tweak WindowRounding/WindowBg so platform windows can look identical to regular ones.
    ImGuiStyle& style = ImGui::GetStyle();
    if (io.ConfigFlags & ImGuiConfigFlags_ViewportsEnable)
    {
        style.WindowRounding = 0.0f;
        style.Colors[ImGuiCol_WindowBg].w = 1.0f;
    }

    // Setup Platform/Renderer backends
    ImGui_ImplGlfw_InitForOpenGL(windowRet, true);
#ifdef __EMSCRIPTEN__
    ImGui_ImplGlfw_InstallEmscriptenCallbacks(window, "#canvas");
#endif
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
    //ImFont* font = io.Fonts->AddFontFromFileTTF("c:\\Windows\\Fonts\\ArialUni.ttf", 18.0f, nullptr, io.Fonts->GetGlyphRangesJapanese());
    //IM_ASSERT(font != nullptr);

    // Our state

    // Main loop
#ifdef __EMSCRIPTEN__
// For an Emscripten build we are disabling file-system access, so let's not attempt to do a fopen() of the imgui.ini file.
// You may manually call LoadIniSettingsFromMemory() to load settings from your own storage.
    io.IniFilename = nullptr;
    EMSCRIPTEN_MAINLOOP_BEGIN
#else
#endif
    return 0;
}

void Application::BuildDockLayout() {
#if 1
    ImGui::DockBuilderRemoveNode(g_viewport_id);
    ImGui::DockBuilderAddNode(g_viewport_id, ImGuiDockNodeFlags_DockSpace);
    ImGui::DockBuilderSetNodeSize(g_viewport_id, ImGui::GetMainViewport()->Size);

    ImGui::DockBuilderSplitNode(g_viewport_id, ImGuiDir_Left, 0.5f, &g_docklefttemp, &g_ImagePreview);
    ImGui::DockBuilderSplitNode(g_viewport_id, ImGuiDir_Up, 0.5f, &g_ImageListView, &g_DebugView);


    ImGui::DockBuilderDockWindow("ImageListView", g_ImageListView);
    ImGui::DockBuilderDockWindow("DebugView", g_DebugView);
    ImGui::DockBuilderDockWindow("ImagePreview", g_ImagePreview);

    ImGui::DockBuilderFinish(g_viewport_id);
#else

#endif
}

int Application::Run() {
    static const char* formatfilter[] = { "*.jpg", "*.png", "*.qoi" };

    GLFWwindow* window;
    InitWindow(window);

    // Load image with 4 channels
    const char* file = OpenFileDialogue("Select an Image", formatfilter, 3);

    std::vector<std::string> paths;
    SplitPaths(normalizePath(file), paths); // Split the file path into components if needed

    for (int i = 0; i < paths.size(); i++) {
        std::cout << paths[i] << std::endl;
    }

    std::cout << paths.size() << " files selected." << std::endl;

    //if (!FileReader::ReadImage(normalizePath(file), imageSource)) {
    //	std::cout << "Failed to read image file: " << stbi_failure_reason() << normalizePath(file) << std::endl;
    //}

    ImGuiID g_viewport_id = ImGui::GetMainViewport()->ID;

    for (auto path : paths) {
        Manager.ImportFromFile(path);
    }

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


        if (g_firstframe) {
            g_firstframe = false;
            //BuildDockLayout();
        }

        ImGuiIO& io = ImGui::GetIO();

        // Below we are displaying the font texture because it is the only texture we have access to inside the demo!
        // Remember that ImTextureID is just storage for whatever you want it to be. It is essentially a value that
        // will be passed to the rendering backend via the ImDrawCmd structure.
        // If you use one of the default imgui_impl_XXXX.cpp rendering backend, they all have comments at the top
        // of their respective source file to specify what they expect to be stored in ImTextureID, for example:
        // - The imgui_impl_dx11.cpp renderer expect a 'ID3D11ShaderResourceView*' pointer
        // - The imgui_impl_opengl3.cpp renderer expect a GLuint OpenGL texture identifier, etc.
        // More:
        // - If you decided that ImTextureID = MyEngineTexture*, then you can pass your MyEngineTexture* pointers
        //   to ImGui::Image(), and gather width/height through your own functions, etc.
        // - You can use ShowMetricsWindow() to inspect the draw data that are being passed to your renderer,
        //   it will help you debug issues if you are confused about it.
        // - Consider using the lower-level ImDrawList::AddImage() API, via ImGui::GetWindowDrawList()->AddImage().
        // - Read https://github.com/ocornut/imgui/blob/master/docs/FAQ.md
        // - Read https://github.com/ocornut/imgui/wiki/Image-Loading-and-Displaying-Examples

        // We can also get the ID of a specific ImGui::Begin() window
        // if we want to dock within that window. For fullscreen docking,
        // we use the main viewport ID.

        {
            ImGui::Begin("ImageListView");
            ImGui::PushStyleVar(ImGuiStyleVar_ImageBorderSize, std::max(1.0f, ImGui::GetStyle().ImageBorderSize));

            ImGui::BeginChild("Image list", ImVec2(320, 540));
            ImVec2 pos = ImGui::GetCursorScreenPos();

            //if (ImGui::Checkbox("Use memory compression", &g_useCompress)) {
            //    if (g_useCompress) {
            //        for (int i = 0; i < Manager.GetImageCount(); ++i) {
            //            std::cout << "Compressed " << Manager.GetName(i) << "\n";
            //            Manager.Compress(i);
            //        }
            //    }
            //    else {
            //        for (int i = 0; i < Manager.GetImageCount(); ++i) {
            //            std::cout << "Decompressed " << Manager.GetName(i) << "\n";
            //            Manager.Decompress(i);
            //        }
            //    }
            //}
            // 
            
            for (int i = 0; i < Manager.GetImageCount(); ++i) {
                if (ImGui::Selectable(Manager.GetName(i).c_str(), selection == i, 1)) {
                    if (i != selection && selection != -1) {
                        Manager.GetRenderer(selection)->UnloadGPU();
                    }
                    selection = i;
                    Manager.LazyLoadImage(selection);
                    Manager.CreateRenderer(selection);
                    Manager.GetRenderer(selection)->LoadGPU();
                }
            }
            ImGui::EndChild();
            
            ImGui::SameLine(0.0f, 1.0f);
            
            ImGui::BeginChild("ImagePreview");
            {
                if (selection != -1) {
                    ImGui::LabelText("info", "File path: %s", Manager.GetName(selection).c_str());
                    ImVec2 dim = Manager.GetDim(selection);
                    ImGui::LabelText("Dimension", "%d x %d", dim.x, dim.y);
                    Manager.GetRenderer(selection)->DisplayImage();
                }
            }
            ImGui::EndChild();

            ImGui::PopStyleVar();
            ImGui::End();
        }
        // Rendering
        ImGui::Render();
        int display_w, display_h;
        glfwGetFramebufferSize(window, &display_w, &display_h);
        glViewport(0, 0, display_w, display_h);
        glClearColor(clearColor.x * clearColor.w, clearColor.y * clearColor.w, clearColor.z * clearColor.w, clearColor.w);
        glClear(GL_COLOR_BUFFER_BIT);
        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

        // Update and Render additional Platform Windows
        // (Platform functions may change the current OpenGL context, so we save/restore it to make it easier to paste this code elsewhere.
        //  For this specific demo app we could also call glfwMakeContextCurrent(window) directly)
        if (io.ConfigFlags & ImGuiConfigFlags_ViewportsEnable)
        {
            GLFWwindow* backup_current_context = glfwGetCurrentContext();
            ImGui::UpdatePlatformWindows();
            ImGui::RenderPlatformWindowsDefault();
            glfwMakeContextCurrent(backup_current_context);
        }

        glfwSwapBuffers(window);
    }
#ifdef __EMSCRIPTEN__
    EMSCRIPTEN_MAINLOOP_END;
#endif

    // Cleanup
    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();

    glfwDestroyWindow(window);
    glfwTerminate();

    return 0;
}



