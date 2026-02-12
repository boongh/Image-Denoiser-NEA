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

#include <algorithm>
#include <iostream>
#include <functional>
#include <queue>

//File read write lib

#include <FileReader.h>
#include <FileWriter.h>


//Custom widgets
#include <ImGuiimageloader.h>
#include <CustomWidget.h>


//Image container
#include <ImageContainer.h>


#define TheGoodBlueColor ImVec4(64, 145, 190, 0)

// Extra functions to add deletion support to ImGuiSelectionBasicStorage
//Included in the ImGui examples
class ExampleSelectionWithDeletion : public ImGuiSelectionBasicStorage {
public:

    // Extra functions to add deletion support to ImGuiSelectionBasicStorage
    // Find which item should be Focused after deletion.
    // Call _before_ item submission. Return an index in the before-deletion item list, your item loop should call SetKeyboardFocusHere() on it.
    // The subsequent ApplyDeletionPostLoop() code will use it to apply Selection.
    // - We cannot provide this logic in core Dear ImGui because we don't have access to selection data.
    // - We don't actually manipulate the ImVector<> here, only in ApplyDeletionPostLoop(), but using similar API for consistency and flexibility.
    // - Important: Deletion only works if the underlying ImGuiID for your items are stable: aka not depend on their index, but on e.g. item id/ptr.
    // FIXME-MULTISELECT: Doesn't take account of the possibility focus target will be moved during deletion. Need refocus or scroll offset.
    int ApplyDeletionPreLoop(ImGuiMultiSelectIO* ms_io, int items_count) {
        if (Size == 0)
            return -1;

        // If focused item is not selected...
        const int focused_idx = (int)ms_io->NavIdItem;  // Index of currently focused item
        if (ms_io->NavIdSelected == false)  // This is merely a shortcut, == Contains(adapter->IndexToStorage(items, focused_idx))
        {
            ms_io->RangeSrcReset = true;    // Request to recover RangeSrc from NavId next frame. Would be ok to reset even when NavIdSelected==true, but it would take an extra frame to recover RangeSrc when deleting a selected item.
            return focused_idx;             // Request to focus same item after deletion.
        }

        // If focused item is selected: land on first unselected item after focused item.
        for (int idx = focused_idx + 1; idx < items_count; idx++)
            if (!Contains(GetStorageIdFromIndex(idx)))
                return idx;

        // If focused item is selected: otherwise return last unselected item before focused item.
        for (int idx = std::min(focused_idx, items_count) - 1; idx >= 0; idx--)
            if (!Contains(GetStorageIdFromIndex(idx)))
                return idx;

        return -1;
    };

    // Rewrite item list (delete items) + update selection.
    // - Call after EndMultiSelect()
    // - We cannot provide this logic in core Dear ImGui because we don't have access to your items, nor to selection data.
    template<typename ITEM_TYPE>
    void ApplyDeletionPostLoop(ImGuiMultiSelectIO* ms_io, ImVector<ITEM_TYPE>& items, int item_curr_idx_to_select)
    {
        // Rewrite item list (delete items) + convert old selection index (before deletion) to new selection index (after selection).
        // If NavId was not part of selection, we will stay on same item.
        ImVector<ITEM_TYPE> new_items;
        new_items.reserve(items.Size - Size);
        int item_next_idx_to_select = -1;
        for (int idx = 0; idx < items.Size; idx++)
        {
            if (!Contains(GetStorageIdFromIndex(idx)))
                new_items.push_back(items[idx]);
            if (item_curr_idx_to_select == idx)
                item_next_idx_to_select = new_items.Size - 1;
        }
        items.swap(new_items);

        // Update selection
        Clear();
        if (item_next_idx_to_select != -1 && ms_io->NavIdSelected)
            SetItemSelected(GetStorageIdFromIndex(item_next_idx_to_select), true);
    }
};


void LogtoAppTerminal(std::string logmsg);

/// <summary>
/// Wrapper class around the entire application to avoid global variables
/// </summary>
class Application {
public:

    static Application* currentApp;
    bool refresh = false;

    struct FilterParameters {
        struct BilateralFilterParameters {
            float sigmaSpatial;
            float sigmaColor;
            int kernelWidth;
            int kernelHeight;
            BilateralFilterParameters() { Reset(); }
            void Reset() {
                sigmaSpatial = 1.0f;
                sigmaColor = 0.1f;
                kernelWidth = 5;
                kernelHeight = 5;
            }
        };

        struct SmoothFilterParameters {
            float strength;
            int kernelWidth;
            int kernelHeight;
            SmoothFilterParameters() { Reset(); }
            void Reset() {
                strength = 1.0f;
                kernelWidth = 5;
                kernelHeight = 5;
            }
        };

        struct DWTParameters {
            int decimationLevel;
            DWTParameters() { Reset(); }
            void Reset() {
                decimationLevel = 1;
            }
        };

        BilateralFilterParameters BFParameter;
        SmoothFilterParameters SFParameter;
        DWTParameters DWTParameter;
    };

	FilterParameters filterParameters;

    Application(ImVec4 backgroundColor = TheGoodBlueColor);

    int Run();

    void DEBUGRUN(const char* infiles);
    void BENCHMARKRUN(const char* infiles);

    void ImportImages(std::span<std::string> paths);
    void LogTerminal(std::string log);


#pragma region Denoiser Caller

    void SmoothFilter(std::shared_ptr<ImageEntry> image);
    void BatchSmoothFilter();

    void BilateralFilter(std::shared_ptr<ImageEntry> image);
    void BatchBilateralFilter();

	void DWTDenoise(std::shared_ptr<ImageEntry> image);
	void BatchDWTDenoise();

#pragma endregion
private:

    int InitWindow(GLFWwindow*& windowRet);
    static void glfw_error_callback(int error, const char* description);

    /// <summary>
    /// Build a dock inside the main OpenGL window
    /// </summary>
    void BuildDock();

    void DisplayMenu();
    void DisplayDenoiseParamMenu();
	void DisplayImageList(std::shared_ptr<ImageEntry>& selection);
	void DisplayImageSaveMenu();
    void DisplayDebugMenu();
    void DisplayWindowsMenu();

    void DisplayTerminal();

    //Holds the actual logs
    std::vector<std::string> logs;

    //Terminal display variables
    static const int terminalSizeLimit = 1 << 12;
    char* terminalbuffer;
    int currFirstCharOffset = terminalSizeLimit;

    void ShortcutChecks();

    void ForAllSelectedImage(const std::function<void(std::shared_ptr<ImageEntry>)>& func);


    float g_scale;

    ImVec4 clearColor;
    ImGuiID g_viewport_id;

    ImGuiID g_ImageListView;

    ImGuiID g_docklefttemp;

    ImGuiID g_DebugView;
    ImGuiID g_ImagePreview;

    bool g_firstframe = true;
    bool g_useCompress = true;
    bool g_useDebug = true;
    bool g_openSettings = false;


    bool g_refreshLayout = false;
	std::string g_layoutPath;

    std::mutex logterminalLock = std::mutex();

    ImageManager Manager;

    std::shared_ptr<ImageEntry> currselection = nullptr;
    ExampleSelectionWithDeletion Multiselection;
    
    //Menu functions for access outside of menu
    void LoadAllImages();
	void OpenImageFile();

    void SaveWorkspaceConfig();
	void LoadWorkspaceConfig();
};