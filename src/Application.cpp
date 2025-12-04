#include "Application.h"
#include <denoiser.h>
#include <filesystem>
#include <algorithm>
#include "FileManagement.h"
#include <imgui.h>
#include <tinyfiledialogs.h>

#ifdef DEBUG

#include <chrono>
#endif // DEBUG

#include <set>

namespace fs = std::filesystem;
using slockmutex = std::shared_lock<std::shared_mutex>;

Application::Application(ImVec4 backgroundColor) : 
    g_DebugView(false), 
    g_ImageListView(true), 
    g_ImagePreview(true), 
    g_docklefttemp(false), 
    g_viewport_id(0){
    //Default format filter

    clearColor = backgroundColor;
    currentApp = this;
    return;
}

Application* Application::currentApp = nullptr;

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
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 5);
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


void Application::BuildDock()
{
    //Create a dock space in your main window
    {
        static ImGuiDockNodeFlags dockspace_flags = ImGuiDockNodeFlags_None;
        ImGuiWindowFlags window_flags = ImGuiWindowFlags_MenuBar | ImGuiWindowFlags_NoDocking;


        const ImGuiViewport* viewport = ImGui::GetMainViewport();
        ImGui::SetNextWindowPos(viewport->Pos);
        ImGui::SetNextWindowSize(viewport->Size);
        ImGui::SetNextWindowViewport(viewport->ID);

        window_flags |= ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoBringToFrontOnFocus | ImGuiWindowFlags_NoNavFocus;

        ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 0.0f);
        ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.0f);

        //This part of the code causes error
        ImGui::PushStyleVar(ImGuiStyleVar_FrameBorderSize, 0.0f);
        ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2{ 0.0f , 0.0f });


        //Create a window that is at the main window position and size
        ImGui::Begin("DockSpace Demo", nullptr, window_flags);

        ImGui::PopStyleVar(4);

        ImGuiID dockspace_id = ImGui::GetID("MyDockSpace");
        ImGui::DockSpace(dockspace_id, ImVec2(0.0f, 0.0f), dockspace_flags);

        DisplayMenu();

        ImGui::End();
    }
}

void Application::DisplayMenu() {

    if (ImGui::BeginMenuBar())
    {
        if (ImGui::BeginMenu("File"))
        {
            if (ImGui::MenuItem("Open file", "CTRL+O")) {
				OpenImageFile();
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
				LoadAllImages();
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

void Application::DisplayDenoiseParamMenu() {

#pragma region SF
    ImGui::PushItemWidth(std::min(ImGui::GetContentRegionAvail().x, 200.0f));
    if (ImGui::Button("Smooth LF All", ImVec2(0, 0)) && currselection != nullptr) {
        BatchSmoothFilter();
	    refresh = true;
    }

    ImGui::InputFloat("Strength##SF", &filterParameters.SFParameter.strength, 0.02f, 0.2f);
    ImGui::PopItemWidth();

    ImGui::PushItemWidth(std::min(ImGui::GetContentRegionAvail().x, 100.0f));

    ImGui::InputInt("HalfWidth##SF", &filterParameters.SFParameter.kernelWidth, 1, 5);
    ImGui::InputInt("HalfHeight##SF", &filterParameters.SFParameter.kernelHeight, 1, 5);

    ImGui::PopItemWidth();

#pragma endregion

#pragma region BF

    ImGui::PushItemWidth(std::min(ImGui::GetContentRegionAvail().x, 200.0f));
    if (ImGui::Button("Bilateral Filter", ImVec2(0, 0)) && currselection != nullptr) {
        BilateralFilter(currselection);
        refresh = true;
    }

    ImGui::InputFloat("Strength Spatial##BF", &filterParameters.BFParameter.sigmaSpatial, 1, 5);
    ImGui::InputFloat("Strength Intensity##BF", &filterParameters.BFParameter.sigmaColor, 1, 5);
    ImGui::PopItemWidth();

    ImGui::PushItemWidth(std::min(ImGui::GetContentRegionAvail().x, 100.0f));
    ImGui::InputInt("Half Width##BF", &filterParameters.BFParameter.kernelWidth, 1, 5);
    ImGui::SameLine();
    ImGui::InputInt("Half Height##BF", &filterParameters.BFParameter.kernelHeight, 1, 5);
    ImGui::PopItemWidth();
    
#pragma endregion

#pragma region DWT
    ImGui::Separator();
    ImGui::Text("DWT Denoising");
    ImGui::PushItemWidth(std::min(ImGui::GetContentRegionAvail().x, 100.0f));
    ImGui::InputInt("Decomposition Level##DWT", &filterParameters.DWTParameter.decimationLevel, 1, 2);
    ImGui::PopItemWidth();
    if (ImGui::Button("DWT Denoise##DWT", ImVec2(0, 0)) && currselection != nullptr) {
        BatchDWTDenoise();
        refresh = true;
	}
#pragma endregion
}

void Application::ForAllSelectedImage(const std::function<void(std::shared_ptr<ImageEntry>)>& func) {  
    for (int idx = 0; idx < Manager.GetImageCount(); idx++) {  
        if (Multiselection.Contains(Multiselection.GetStorageIdFromIndex(idx))) {  
            func(Manager.GetImage(idx));  
        }  
    }
}

void Application::DisplayImageList(std::shared_ptr<ImageEntry>& selection) {

	//Modified from the example multi-selection with deletion widget
    //of ImGui example projects

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

    // Use default selection.Adapter: Pass index to SetNextItemSelectionUserData(), store index in Selection
    static ImVector<int> items;
    static int items_next_id = 0;
    
    static bool request_deletion_from_menu = false; // Queue deletion triggered from context menu

    ImGui::Text("Selection size: %d/%d", Multiselection.Size, items.Size);

    const float items_height = (widget_type == WidgetType_TreeNode) ? ImGui::GetTextLineHeight() : ImGui::GetTextLineHeightWithSpacing();
    ImGui::SetNextWindowContentSize(ImVec2(0.0f, items.Size * items_height));
    if (ImGui::BeginChild("##Basket", ImVec2(-FLT_MIN, 0), ImGuiChildFlags_FrameStyle))
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

                fs::path item_path = Manager.GetPath_path(n);
                fs::path filename = item_path.filename();
                fs::path parentDir = item_path.parent_path();

                std::string fileIdentifier = item_path.parent_path().filename().string() + "/" + item_path.filename().string();

                sprintf_s(label, "%s", fileIdentifier.c_str());

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
            ForAllSelectedImage([](std::shared_ptr<ImageEntry> image) { deletionSet.insert(image); });
            Multiselection.ApplyDeletionPostLoop(ms_io, items, item_curr_idx_to_focus);
        }

        if (widget_type == WidgetType_TreeNode)
            ImGui::PopStyleVar();

        selection = Manager.GetImage(ms_io->NavIdItem);
    }
    ImGui::EndChild();

}

fs::path PathCleanup(fs::path inputPath) {
    std::regex rgxvalidspace(R"([\s\\]*(\\)[\s\\]*)");
    std::regex rgxvalidchar(R"([^\w:\\\[\]\(\)]\%)");
    std::string regexBuffer = std::regex_replace(inputPath.string(), rgxvalidspace, "$1");
    regexBuffer = std::regex_replace(regexBuffer, rgxvalidchar, "");
    return regexBuffer;
}

void Application::DisplayImageSaveMenu()
{
    static char buf[256] = "";

    ImGui::PushItemWidth(std::min(ImGui::GetContentRegionAvail().x, 400.0f));

    //preliminary path clean-up
    if (ImGui::InputText("string", buf, IM_ARRAYSIZE(buf))) {
        //Remove space before and after slashes (Invalid spaces)
        strcpy_s(buf, PathCleanup(buf).string().c_str());
    }

    ImGui::PopItemWidth();

    ImGui::SameLine();

    if (ImGui::Button("Open with File Explorer")) {
        const char* folder = OpenFolderDialogue("Choose destination directory");
        if (folder != nullptr) {
            std::string folderstr(folder);
            size_t len = folderstr.length();
            if (len + 1 < sizeof(buf)) {
                strcpy_s(buf, folderstr.c_str());
            }
        }
    }

    static int currentFormat = 0;
    ImGui::Combo("Format", &currentFormat, formatfilter, formatfiltercount);


    if (ImGui::Button("Save Image As")) {

        std::unordered_map<std::string, ValidFormatter> formatter{
        {"\\[DATE\\(\(.*?\)\\)\\]", [](std::smatch match) {
                time_t rawtime;
                struct tm* timeinfo;

                time(&rawtime);
                timeinfo = localtime(&rawtime);
                char buffer[256];
                strftime(buffer, 256, match[1].str().c_str(), timeinfo);
                return std::string(buffer);

                return match.str();

            }}
        };

        std::vector<std::tuple<fs::path, const RGBAImageI>> imageSaveList;

        //Make sure to maintain read permission to lock writes
        std::vector<std::tuple<slockmutex, std::shared_ptr<const RGBAImageI>>> readperms;
        int count = 0;

        for (int idx = 0; idx < Manager.GetImageCount(); idx++) {
            if (Multiselection.Contains(Multiselection.GetStorageIdFromIndex(idx))) {

                std::shared_ptr<ImageEntry> image = Manager.GetImage(idx);
                if (image->LoadImage() == 0 && image->DecompressImageData() == 0) {
                    readperms.push_back(image->RGBAIRead());
                    auto& readperm = readperms[count];
                    auto RGBAImageIptr = std::get<1>(readperm);
                    count++;

                    //Insert extra, file dependent formatter after
                    formatter.insert({ "\\[FILENAME\\]", [&](std::smatch str) {
                          return image->GetFileName().stem().string(); } });

                    fs::path filepath = fs::path(buf) / fs::path(image->GetFileName());
                    filepath = FormatPath(formatter, filepath);
                    filepath = PathCleanup(filepath);

                    imageSaveList.push_back(std::make_tuple(filepath, *RGBAImageIptr));
                }
                else {
                    return;
                }
            }
        }

        SaveImages(imageSaveList, static_cast<ImageFormat>(currentFormat));

    }
}

void Application::DisplayDebugMenu()
{
    ImGui::Checkbox("Use Memory Compression", &g_useCompress);
}

void Application::DisplayTerminal()
{

    float availx = ImGui::GetContentRegionAvail().x;
    float availy = ImGui::GetContentRegionAvail().y;

    ImGui::InputTextMultiline("Output", &buf[currFirstCharOffset], terminalSizeLimit - currFirstCharOffset, ImGui::GetContentRegionAvail(), ImGuiInputTextFlags_ReadOnly);
}

void Application::LogTerminal(std::string log) {

    //Clears terminal
    memset(buf, '\n', terminalSizeLimit);

    logs.insert(logs.begin(), log);
    while (logs.size() > 1000) {
        logs.erase(logs.end() - 4, logs.end());
    }

    int index = terminalSizeLimit;

    for (std::string logmsg : logs) {
        int temp = index - logmsg.length() - 1;
        if (temp >= 0) {
            index -= logmsg.length();
            memcpy(&buf[index], logmsg.c_str(), logmsg.length());
        }
        else {
            break;
        }
    }

    currFirstCharOffset = index;
    buf[terminalSizeLimit - 1] = '\0';

    return;
}

void Application::ShortcutChecks()
{

    //Shortcut to open files
    if (ImGui::Shortcut(ImGuiMod_Ctrl | ImGuiKey_O, ImGuiInputFlags_Repeat | ImGuiInputFlags_RouteGlobal)) {
        OpenImageFile();
    }
    //Batch load
    if (ImGui::Shortcut(ImGuiKey_S | ImGuiMod_Ctrl)) {
        LoadAllImages();
    }

    if (ImGui::Shortcut(ImGuiKey_C | ImGuiMod_Ctrl, ImGuiInputFlags_RouteGlobal)) {
        const char* imguiini[] = { ".ini" };
        std::vector<std::string> path;
        if (FileSelection("Select Config File", "", imguiini, 1, "ImGui Ini", false, path) == 0) {
            ImGui::LoadIniSettingsFromDisk(path[0].c_str());
        }
    }

    if (ImGui::Shortcut(ImGuiKey_C | ImGuiMod_Shift | ImGuiMod_Ctrl, ImGuiInputFlags_RouteGlobal)) {
        const char* imguiini[] = { ".ini" };
        const char* saveloc = tinyfd_saveFileDialog("Save Config", "", 1, imguiini, "ImGui Ini");
        if (saveloc != NULL) {
            ImGui::SaveIniSettingsToDisk(saveloc);
        }
    }
}

void Application::LoadAllImages() {

    std::thread thread([this]() {
        for (auto& image : Manager) {
            if (image->GetStatus() == ImageEntry::CompressionStatus::NOT_LOADED) {
                image->LoadImage();
                image->CompressImageData();
            }
        }
	});

    thread.detach();
}


void Application::OpenImageFile() {
    std::vector<std::string> paths;
    FileSelection("Select an image", "", formatfilter, formatfiltercount, "Image", true, paths);
    ImportImages(paths);
}



int Application::Run() {

    GLFWwindow* window;
    InitWindow(window);

    ImGuiIO& io = ImGui::GetIO();

    LogTerminal(io.IniFilename);

    ImGuiID g_viewport_id = ImGui::GetMainViewport()->ID;

    std::shared_ptr<ImageEntry> prevselection = nullptr;

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

        BuildDock();

        ShortcutChecks();

        ImGuiIO& io = ImGui::GetIO();

		ImGui::SetNextWindowSize(ImVec2(1280, 720), ImGuiCond_FirstUseEver);

        
        ImGui::Begin("ImageDenoisingParameter");
        {
            DisplayDenoiseParamMenu();
        }

        ImGui::End();

        ImGui::Begin("SaveImage", nullptr);
        {
            DisplayImageSaveMenu();
        }

        ImGui::End();

        ImGui::Begin("Debug", nullptr);
        {
            DisplayDebugMenu();
        }

        ImGui::End();

        ImGui::Begin("Terminal", nullptr);
        {
            DisplayTerminal();
        }
        ImGui::End();

		ImGui::Begin("Preview", nullptr);

        ImGui::BeginChild("ImagePreview", ImVec2(0, 0));
        {
            if (currselection != nullptr) {
                if (currselection->GetStatus() == ImageEntry::CompressionStatus::NOT_LOADED) {
                    ImGui::Text("FAILED TO LOAD IMAGE");
                }
                else {

                    ImGui::LabelText("info", "File path: %s", currselection->GetFileName().string().c_str());
                    ImVec2 dim = ImVec2(currselection->GetWidth(), currselection->GetHeight());
                    ImGui::LabelText("Dimension", "%d x %d", static_cast<int>(dim.x), static_cast<int>(dim.y));

					ImGui::BeginChild("ImageRenderer", ImVec2(0, 0), ImGuiChildFlags_Borders);
                    auto renderer = Manager.GetRenderer(currselection);
                    renderer->DisplayImage(ImVec2(64, 64), TheGoodBlueColor);
					ImGui::EndChild();
                }
            }
        }

        ImGui::EndChild();

        ImGui::End();
        
        ImGui::Begin("ImageListView", nullptr, ImGuiWindowFlags_NoNav);
        {

            ImGui::PushStyleVar(ImGuiStyleVar_ImageBorderSize, std::max(1.0f, ImGui::GetStyle().ImageBorderSize));

            ImGui::BeginChild("Image list", ImVec2(0, 0), ImGuiChildFlags_Borders);

            ImVec2 pos = ImGui::GetCursorScreenPos();

            
			DisplayImageList(currselection);

            if (currselection != prevselection) {
                if (currselection != nullptr) {
                    int load = currselection->LoadImage();
                    if (load != 0) {
                        Manager.CreateErrorRenderer(currselection)->LoadGPU();
                    }
                    else if (load == 0 && currselection->DecompressImageData() == 0) 
                        Manager.CreateRenderer(currselection)->LoadGPU();
                    else {
                        LogTerminal("Fail to load image file");
                    }
                }
                if (prevselection != nullptr) {
                    Manager.DestroyRenderer(prevselection);
                    if (g_useCompress) {
                        int comp = prevselection->CompressImageData();

                        //Only fails if compressor fails unrocoverably
                        if (comp != 0) {
                            throw std::exception("COMPRESION FAILED", comp);
                        }
                    };
                }
                prevselection = currselection;
            }

            ImGui::EndChild();
            
            ImGui::SameLine(0.0f, 1.0f);

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



void Application::DEBUGRUN(const char* infiles) {
    //Only runs in debug compile MSVC   
#ifdef DEBUG


    std::vector<std::string> paths;

    SplitPaths(infiles, paths);

    //SplitPaths(infiles, paths);
    ImportImages(paths);

#endif // DEBUG

    return;
}

void Application::ImportImages(std::span<std::string> paths) {
    for (auto& path : paths) Manager.ImportFromFile(path);
}


#pragma region Denoiser wrappers


void Application::SmoothFilter(std::shared_ptr<ImageEntry> image)
{
    auto param = filterParameters.SFParameter;
    bool isDecompressed = image->IsDecompressed();

    if (image->LoadImage() == 0 && image->DecompressImageData() == 0) {
        std::print("Successfully load and decompress");
        std::jthread([this, image, param, isDecompressed]() {

#ifdef DEBUG
            auto timer = std::chrono::high_resolution_clock();
            auto start = timer.now();
#endif // DEBUG

            {
                auto readwriteperm = image->ReadWriteImageData();

                Denoiser::SmoothLF(
                    std::get<1>(readwriteperm),
                    static_cast<unsigned int>(image->GetWidth()),
                    static_cast<unsigned int>(image->GetHeight()),
                    param.kernelWidth, param.kernelHeight, param.strength);

            }

#ifdef DEBUG
            auto end = timer.now();
            auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);

            LogTerminal("Took SLF " + std::to_string(elapsed.count()) + " ms\n");
#endif // DEBUG

            LogTerminal("Done SLF " + image->GetFileName().string() + "\n");

            if (!isDecompressed) image->CompressImageData();

            }).detach();
    }
    else {
        std::print("File couldn't be loaded");
    }
}

void Application::BilateralFilter(std::shared_ptr<ImageEntry> image) {
    auto param = filterParameters.BFParameter;
    bool isDecompressed = image->IsDecompressed();

    if (image->LoadImage() == 0 && image->DecompressImageData() == 0) {
        std::jthread([this, image, param, isDecompressed]() {

#ifdef DEBUG
            auto timer = std::chrono::high_resolution_clock();
            auto start = timer.now();
#endif // DEBUG
            {

                auto readwriteperm = image->ReadWriteImageData();

                Denoiser::BilateralFilter(
                    std::get<1>(readwriteperm),
                    static_cast<unsigned int>(image->GetWidth()),
                    static_cast<unsigned int>(image->GetHeight()),
                    param.kernelWidth, param.kernelHeight,
                    param.sigmaSpatial, param.sigmaColor);
            }

#ifdef DEBUG
            auto end = timer.now();
            auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);

            LogTerminal("Took BL " + std::to_string(elapsed.count()) + " ms\n");
#endif // DEBUG

            LogTerminal("Done BL " + image->GetFileName().string() + "\n");
            if (!isDecompressed) image->CompressImageData();

            }).detach();
    }
    else {
        std::print("Image Loading Fail");
    }
}

void Application::DWTDenoise(std::shared_ptr<ImageEntry> image)
{
    auto param = filterParameters.DWTParameter;
    bool isDecompressed = image->IsDecompressed();

    if (image->LoadImage() == 0 && image->DecompressImageData() == 0) {
        std::jthread([this, image, param, isDecompressed]() {

#ifdef DEBUG
            auto timer = std::chrono::high_resolution_clock();
            auto start = timer.now();
#endif // DEBUG
            {
                int a = 1;
                auto readwriteperm = image->ReadWriteImageData();

                Denoiser::DWT(
                    std::get<1>(readwriteperm),
                    static_cast<unsigned int>(image->GetWidth()),
                    static_cast<unsigned int>(image->GetHeight()),
                    param.decimationLevel);
            }
#ifdef DEBUG
            auto end = timer.now();
            auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);

            LogTerminal("Took DWT " + std::to_string(elapsed.count()) + " ms\n");
#endif // DEBUG

            LogTerminal("Done DWT " + image->GetFileName().string() + "\n");
            if (!isDecompressed) image->CompressImageData();

            }).detach();
    }
    else {
        std::print("Image Loading Fail");
    }
}

#pragma endregion


#pragma region Batch fitlers

void Application::BatchSmoothFilter()
{
    ForAllSelectedImage([this](std::shared_ptr<ImageEntry> image) {
        SmoothFilter(image);
        });
}

void Application::BatchBilateralFilter()
{
    ForAllSelectedImage([this](std::shared_ptr<ImageEntry> image) {
        BilateralFilter(image);
        });
}

void Application::BatchDWTDenoise()
{
    ForAllSelectedImage([this](std::shared_ptr<ImageEntry> image) {
        DWTDenoise(image);
        });
}

#pragma endregion

void LogtoAppTerminal(std::string logmsg)
{
    Application::currentApp->LogTerminal(logmsg);
}
