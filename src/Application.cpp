#include "Application.h"
#include <denoiser.h>

#ifdef DEBUG
#include <chrono>
#endif // DEBUG

#include <set>

#define TheGoodBlueColor ImVec4(64, 145, 190, 0)


Application::Application(ImVec4 backgroundColor) : g_DebugView(false), g_ImageListView(true), g_ImagePreview(true), g_docklefttemp(false), g_viewport_id(0){
    //Default format filter

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

int Application::OpenImages(const char* const* formatfilter, unsigned int filtercount, std::vector<std::string>& paths) {
    // Load image with 4 channels
    const char* file = OpenFileDialogue("Select an Image", formatfilter, filtercount);

    std::cout << normalizePath(file) << "\n";

    SplitPaths(normalizePath(file), paths); // Split the file path into components if needed

    for (int i = 0; i < paths.size(); i++) {
        std::cout << paths[i] << std::endl;
    }
    std::cout << paths.size() << " files selected." << std::endl;
    return 0;
}



void Application::DisplayMenu() {

    //Format filter
    static const char* formatfilter[] = {
        "*.jpg", 
        "*.png", 
        "*.qoi",
    };

    if (ImGui::BeginMenuBar())
    {
        if (ImGui::BeginMenu("File"))
        {
            if (ImGui::MenuItem("Open file")) {
                std::vector<std::string> paths;
                OpenImages(formatfilter, 3, paths);
                ImportFiles(paths);
            }

            if (ImGui::MenuItem("Redo", "CTRL+Y", false, false)) {} // Disabled item
            ImGui::Separator();
            if (ImGui::MenuItem("Cut", "CTRL+X")) {}
            if (ImGui::MenuItem("Copy", "CTRL+C")) {}
            if (ImGui::MenuItem("Paste", "CTRL+V")) {}
            ImGui::EndMenu();
        }

        if(ImGui::BeginMenu("Edit"))
        {
            if (ImGui::MenuItem("Load All", "CTRL+SHIFT+A")) {
                for (int image = 0; image < Manager.GetImageCount(); image++) {
                    if (Manager.GetStatus(image) == ImageEntry::CompressionStatus::NOT_LOADED) {
                        Manager.LazyLoadImage(image);
                    }
				}
            }
            if (ImGui::MenuItem("Undo", "CTRL+Z")) {}
            if (ImGui::MenuItem("Redo", "CTRL+Y")) {}
            ImGui::Separator();
            if (ImGui::MenuItem("Cut", "CTRL+X")) {}
            if (ImGui::MenuItem("Copy", "CTRL+C")) {}
            if (ImGui::MenuItem("Paste", "CTRL+V")) {}
            ImGui::EndMenu();
		}
        ImGui::EndMenuBar();
    }
}

int Application::Run() {

    GLFWwindow* window;
    InitWindow(window);

    ImGuiID g_viewport_id = ImGui::GetMainViewport()->ID;

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

        {
            bool t = true;
            ImGui::Begin("ImageListView", &t, ImGuiWindowFlags_MenuBar | ImGuiWindowFlags_NoBackground);

            DisplayMenu();

            ImGui::PushStyleVar(ImGuiStyleVar_ImageBorderSize, std::max(1.0f, ImGui::GetStyle().ImageBorderSize));

            ImGui::BeginChild("Image list", ImVec2(0, 0), ImGuiChildFlags_Borders | ImGuiChildFlags_ResizeX | ImGuiChildFlags_ResizeY);

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
            
            if (ImGui::TreeNode("Selector", "Multi-Select (advanced)"))
            {
                // Options
                enum WidgetType { WidgetType_Selectable, WidgetType_TreeNode };
                static bool use_clipper = true;
                static bool use_deletion = true;
                static bool use_drag_drop = true;
                static bool show_in_table = false;
                static bool show_color_button = true;
                static ImGuiMultiSelectFlags flags = ImGuiMultiSelectFlags_ClearOnEscape | ImGuiMultiSelectFlags_BoxSelect1d;
                static WidgetType widget_type = WidgetType_Selectable;
                static bool want_delete;
                static std::set<std::shared_ptr<ImageEntry>> deletionSet;

                /*if (ImGui::TreeNode("Options"))
                {
                    if (ImGui::RadioButton("Selectables", widget_type == WidgetType_Selectable)) { widget_type = WidgetType_Selectable; }
                    ImGui::SameLine();
                    if (ImGui::RadioButton("Tree nodes", widget_type == WidgetType_TreeNode)) { widget_type = WidgetType_TreeNode; }
                    ImGui::SameLine();
                    ImGui::Checkbox("Enable clipper", &use_clipper);
                    ImGui::Checkbox("Enable deletion", &use_deletion);
                    ImGui::Checkbox("Enable drag & drop", &use_drag_drop);
                    ImGui::Checkbox("Show in a table", &show_in_table);
                    ImGui::Checkbox("Show color button", &show_color_button);
                    ImGui::CheckboxFlags("ImGuiMultiSelectFlags_SingleSelect", &flags, ImGuiMultiSelectFlags_SingleSelect);
                    ImGui::CheckboxFlags("ImGuiMultiSelectFlags_NoSelectAll", &flags, ImGuiMultiSelectFlags_NoSelectAll);
                    ImGui::CheckboxFlags("ImGuiMultiSelectFlags_NoRangeSelect", &flags, ImGuiMultiSelectFlags_NoRangeSelect);
                    ImGui::CheckboxFlags("ImGuiMultiSelectFlags_NoAutoSelect", &flags, ImGuiMultiSelectFlags_NoAutoSelect);
                    ImGui::CheckboxFlags("ImGuiMultiSelectFlags_NoAutoClear", &flags, ImGuiMultiSelectFlags_NoAutoClear);
                    ImGui::CheckboxFlags("ImGuiMultiSelectFlags_NoAutoClearOnReselect", &flags, ImGuiMultiSelectFlags_NoAutoClearOnReselect);
                    ImGui::CheckboxFlags("ImGuiMultiSelectFlags_BoxSelect1d", &flags, ImGuiMultiSelectFlags_BoxSelect1d);
                    ImGui::CheckboxFlags("ImGuiMultiSelectFlags_BoxSelect2d", &flags, ImGuiMultiSelectFlags_BoxSelect2d);
                    ImGui::CheckboxFlags("ImGuiMultiSelectFlags_BoxSelectNoScroll", &flags, ImGuiMultiSelectFlags_BoxSelectNoScroll);
                    ImGui::CheckboxFlags("ImGuiMultiSelectFlags_ClearOnEscape", &flags, ImGuiMultiSelectFlags_ClearOnEscape);
                    ImGui::CheckboxFlags("ImGuiMultiSelectFlags_ClearOnClickVoid", &flags, ImGuiMultiSelectFlags_ClearOnClickVoid);
                    if (ImGui::CheckboxFlags("ImGuiMultiSelectFlags_ScopeWindow", &flags, ImGuiMultiSelectFlags_ScopeWindow) && (flags & ImGuiMultiSelectFlags_ScopeWindow))
                        flags &= ~ImGuiMultiSelectFlags_ScopeRect;
                    if (ImGui::CheckboxFlags("ImGuiMultiSelectFlags_ScopeRect", &flags, ImGuiMultiSelectFlags_ScopeRect) && (flags & ImGuiMultiSelectFlags_ScopeRect))
                        flags &= ~ImGuiMultiSelectFlags_ScopeWindow;
                    if (ImGui::CheckboxFlags("ImGuiMultiSelectFlags_SelectOnClick", &flags, ImGuiMultiSelectFlags_SelectOnClick) && (flags & ImGuiMultiSelectFlags_SelectOnClick))
                        flags &= ~ImGuiMultiSelectFlags_SelectOnClickRelease;
                    if (ImGui::CheckboxFlags("ImGuiMultiSelectFlags_SelectOnClickRelease", &flags, ImGuiMultiSelectFlags_SelectOnClickRelease) && (flags & ImGuiMultiSelectFlags_SelectOnClickRelease))
                        flags &= ~ImGuiMultiSelectFlags_SelectOnClick;
                    ImGui::SameLine();
                    ImGui::TreePop();
                }*/

                // Initialize default list with 1000 items.
                // Use default selection.Adapter: Pass index to SetNextItemSelectionUserData(), store index in Selection
                static ImVector<int> items;
                static int items_next_id = 0;
                static ExampleSelectionWithDeletion Multiselection;
                static bool request_deletion_from_menu = false; // Queue deletion triggered from context menu

                ImGui::Text("Selection size: %d/%d", Multiselection.Size, items.Size);

                const float items_height = (widget_type == WidgetType_TreeNode) ? ImGui::GetTextLineHeight() : ImGui::GetTextLineHeightWithSpacing();
                ImGui::SetNextWindowContentSize(ImVec2(0.0f, items.Size * items_height));
                if (ImGui::BeginChild("##Basket", ImVec2(-FLT_MIN, ImGui::GetFontSize() * 20), ImGuiChildFlags_FrameStyle | ImGuiChildFlags_ResizeY))
                {
                    ImVec2 color_button_sz(ImGui::GetFontSize(), ImGui::GetFontSize());
                    if (widget_type == WidgetType_TreeNode)
                        ImGui::PushStyleVarY(ImGuiStyleVar_ItemSpacing, 0.0f);

                    ImGuiMultiSelectIO* ms_io = ImGui::BeginMultiSelect(flags, Multiselection.Size, items.Size);
                    Multiselection.ApplyRequests(ms_io);

                    if (deletionSet.size() > 0) {
                        Manager.UnloadImage(deletionSet);
						deletionSet.clear();
                    }

                    if (Manager.GetImageCount() != items.Size) {
                        items.clear();
                        items_next_id = 0;
                        for (int n = 0; n < Manager.GetImageCount(); n++) {
                            items.push_back(items_next_id++);
                        }
                    }

                    want_delete = (ImGui::Shortcut(ImGuiKey_Delete, ImGuiInputFlags_Repeat) && (Multiselection.Size > 0)) || request_deletion_from_menu;
                    const int item_curr_idx_to_focus = want_delete ? Multiselection.ApplyDeletionPreLoop(ms_io, items.Size) : -1;
                    request_deletion_from_menu = false;

                    if (show_in_table)
                    {
                        if (widget_type == WidgetType_TreeNode)
                            ImGui::PushStyleVar(ImGuiStyleVar_CellPadding, ImVec2(0.0f, 0.0f));
                        ImGui::BeginTable("##Split", 2, ImGuiTableFlags_Resizable | ImGuiTableFlags_NoSavedSettings | ImGuiTableFlags_NoPadOuterX);
                        ImGui::TableSetupColumn("", ImGuiTableColumnFlags_WidthStretch, 0.70f);
                        ImGui::TableSetupColumn("", ImGuiTableColumnFlags_WidthStretch, 0.30f);
                        //ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacingY, 0.0f);
                    }

                    ImGuiListClipper clipper;
                    if (use_clipper)
                    {
                        clipper.Begin(items.Size);
                        if (item_curr_idx_to_focus != -1)
                            clipper.IncludeItemByIndex(item_curr_idx_to_focus); // Ensure focused item is not clipped.
                        if (ms_io->RangeSrcItem != -1)
                            clipper.IncludeItemByIndex((int)ms_io->RangeSrcItem); // Ensure RangeSrc item is not clipped.
                    }

                    while (!use_clipper || clipper.Step())
                    {
                        const int item_begin = use_clipper ? clipper.DisplayStart : 0;
                        const int item_end = use_clipper ? clipper.DisplayEnd : items.Size;
                        for (int n = item_begin; n < item_end; n++)
                        {
                            if (show_in_table)
                                ImGui::TableNextColumn();

                            const int item_id = items[n];
                            const char* item_category = "";
                            char label[256];
                            sprintf_s(label, "Object %05d: %s", item_id, Manager.GetName(n).c_str());

                            // IMPORTANT: for deletion refocus to work we need object ID to be stable,
                            // aka not depend on their index in the list. Here we use our persistent item_id
                            // instead of index to build a unique ID that will persist.
                            // (If we used PushID(index) instead, focus wouldn't be restored correctly after deletion).
                            ImGui::PushID(item_id);

                            // Emit a color button, to test that Shift+LeftArrow landing on an item that is not part
                            // of the selection scope doesn't erroneously alter our selection.
                            if (show_color_button)
                            {
                                ImU32 dummy_col = (ImU32)((unsigned int)n * 0xC250B74B) | IM_COL32_A_MASK;
                                ImGui::ColorButton("##", ImColor(dummy_col), ImGuiColorEditFlags_NoTooltip, color_button_sz);
                                ImGui::SameLine();
                            }

                            // Submit item
                            bool item_is_selected = Multiselection.Contains((ImGuiID)n);
                            bool item_is_open = false;
                            ImGui::SetNextItemSelectionUserData(n);
                            if (widget_type == WidgetType_Selectable)
                            {
                                ImGui::Selectable(label, item_is_selected, ImGuiSelectableFlags_None);
                            }
                            else if (widget_type == WidgetType_TreeNode)
                            {
                                ImGuiTreeNodeFlags tree_node_flags = ImGuiTreeNodeFlags_SpanAvailWidth | ImGuiTreeNodeFlags_OpenOnArrow | ImGuiTreeNodeFlags_OpenOnDoubleClick;
                                if (item_is_selected)
                                    tree_node_flags |= ImGuiTreeNodeFlags_Selected;
                                item_is_open = ImGui::TreeNodeEx(label, tree_node_flags);
                            }

                            // Focus (for after deletion)
                            if (item_curr_idx_to_focus == n)
                                ImGui::SetKeyboardFocusHere(-1);

                            // Drag and Drop
                            if (use_drag_drop && ImGui::BeginDragDropSource())
                            {
                                // Create payload with full selection OR single unselected item.
                                // (the later is only possible when using ImGuiMultiSelectFlags_SelectOnClickRelease)
                                if (ImGui::GetDragDropPayload() == NULL)
                                {
                                    ImVector<int> payload_items;
                                    void* it = NULL;
                                    ImGuiID id = 0;
                                    if (!item_is_selected)
                                        payload_items.push_back(item_id);
                                    else
                                        while (Multiselection.GetNextSelectedItem(&it, &id))
                                            payload_items.push_back((int)id);
                                    ImGui::SetDragDropPayload("MULTISELECT_DEMO_ITEMS", payload_items.Data, (size_t)payload_items.size_in_bytes());
                                }

                                // Display payload content in tooltip
                                const ImGuiPayload* payload = ImGui::GetDragDropPayload();
                                const int* payload_items = (int*)payload->Data;
                                const int payload_count = (int)payload->DataSize / (int)sizeof(int);
                                if (payload_count == 1)
                                    ImGui::Text("Object %05d: %s", payload_items[0], "");
                                else
                                    ImGui::Text("Dragging %d objects", payload_count);

                                ImGui::EndDragDropSource();
                            }

                            if (widget_type == WidgetType_TreeNode && item_is_open)
                                ImGui::TreePop();

                            // Right-click: context menu
                            if (ImGui::BeginPopupContextItem())
                            {
                                ImGui::BeginDisabled(!use_deletion || Multiselection.Size == 0);
                                sprintf_s(label, "Delete %d item(s)###DeleteSelected", Multiselection.Size);
                                if (ImGui::Selectable(label))
                                    request_deletion_from_menu = true;
                                ImGui::EndDisabled();
                                ImGui::Selectable("Close");
                                ImGui::EndPopup();
                            }

                            // Demo content within a table
                            if (show_in_table)
                            {
                                ImGui::TableNextColumn();
                                ImGui::SetNextItemWidth(-FLT_MIN);
                                ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(0, 0));
                                ImGui::InputText("###NoLabel", (char*)(void*)item_category, strlen(item_category), ImGuiInputTextFlags_ReadOnly);
                                ImGui::PopStyleVar();
                            }

                            ImGui::PopID();
                        }
                        if (!use_clipper)
                            break;
                    }

                    if (show_in_table)
                    {
                        ImGui::EndTable();
                        if (widget_type == WidgetType_TreeNode)
                            ImGui::PopStyleVar();
                    }

                    // Apply multi-select requests
                    ms_io = ImGui::EndMultiSelect();
                    Multiselection.ApplyRequests(ms_io);
                    if (want_delete) {
                        for (int idx = 0; idx < Manager.GetImageCount(); idx++)
                        {
                            if (Multiselection.Contains(Multiselection.GetStorageIdFromIndex(idx))) {
								deletionSet.insert(Manager.GetImage(idx));
                            }
                        }

                        Multiselection.ApplyDeletionPostLoop(ms_io, items, item_curr_idx_to_focus);
                    }

                    if (widget_type == WidgetType_TreeNode)
                        ImGui::PopStyleVar();

                    selection = Manager.GetImage(ms_io->NavIdItem);
                }
                ImGui::EndChild();
                ImGui::TreePop();
            }

            if (&*selection != &*prevselection) {
                if (selection != nullptr) {
                    selection->LoadImage();
                    Manager.CreateRenderer(selection)->LoadGPU();
                }
                if (prevselection != nullptr) {
                    Manager.DestroyRenderer(prevselection);
                }
                prevselection = selection;
            }

            ImGui::EndChild();
            
            ImGui::SameLine(0.0f, 1.0f);
            

            ImGui::BeginChild("ImagePreview");
            {
                if (selection != nullptr) {
                    ImGui::LabelText("info", "File path: %s", selection->GetFilePath().c_str());
                    ImVec2 dim = ImVec2(selection->GetWidth(), selection->GetHeight());
                    ImGui::LabelText("Dimension", "%d x %d", static_cast<int>(dim.x), static_cast<int>(dim.y));
                    if (ImGui::Button("Smooth LF", ImVec2(0, 0))) {
                        auto currentSelection = selection;
                        ImVec2 currentDim = dim;
                        std::jthread([this, currentSelection, currentDim]() {

#ifdef DEBUG
                            auto timer = std::chrono::high_resolution_clock();
                            auto start = timer.now();
#endif // DEBUG

                            if (currentSelection != nullptr) {
                                std::vector<PixelRGBA> denoised = Denoiser::SmoothLF(
                                    currentSelection->ReadImageData(),
                                    static_cast<unsigned int>(currentDim.x),
                                    static_cast<unsigned int>(currentDim.y), 5, 5, 0.3);
                                std::string name = std::string(currentSelection->GetFilePath() + "Copy");

                                Manager.ImportFromSpan(
                                    denoised,
                                    static_cast<unsigned int>(currentDim.x),
                                    static_cast<unsigned int>(currentDim.y),
                                    name);
                            }

#ifdef DEBUG
                            auto end = timer.now();
                            auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);

                            std::cout << "Took " << elapsed.count() << " ms" << std::endl;
#endif // DEBUG

                        }).detach();
                    }
                    ImGui::SameLine();
                    auto renderer = Manager.GetRenderer(selection);
                    if(renderer == nullptr) {
                        renderer = Manager.CreateRenderer(selection);
					}
                    renderer->DisplayImage(ImVec2(64, 64), TheGoodBlueColor);
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

#ifdef DEBUG

void Application::DEBUGRUN(const char* infiles) {
    //Format filter
    const char* formatfilter[] = {
        "*.jpg",
        "*.png",
        "*.qoi",
    };


    std::vector<std::string> paths;

    SplitPaths(infiles, paths);

    for (std::string s : paths) {
        std::cout << s << "\n";
    }

    ImportFiles(paths);

    /*auto timer = std::chrono::high_resolution_clock();
    auto start = timer.now();

    std::vector<std::jthread> threads_;

    for (int i = 0; i < Manager.GetImageCount(); ++i) {
        Manager.LazyLoadImage(i);
        ImVec2 currentDim = Manager.GetDim(i);
        threads_.push_back(std::jthread([this, i, currentDim]() {

            auto image = Manager.GetImage(i);
            if (image) {
                std::vector<PixelRGBA> denoised = Denoiser::SmoothLF(
                    image->ReadImageData(),
                    static_cast<unsigned int>(currentDim.x),
                    static_cast<unsigned int>(currentDim.y), 5, 5, 0.3);
                std::string name = std::string(Manager.GetName(i) + "Copy");
        
                Manager.ImportFromSpan(
                    denoised,
                    static_cast<unsigned int>(currentDim.x),
                    static_cast<unsigned int>(currentDim.y),
                    name);
            }
        }));
    }

    for (auto &t : threads_) {
        if(t.joinable()) t.join();
    }

    auto end = timer.now();
    auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);

    std::cout << "Took " << elapsed.count() << " ms" << std::endl;*/

    
#endif // DEBUG

}

void Application::ImportFiles(std::span<std::string> paths) {
    for (auto path : paths) Manager.ImportFromFile(path);
}



