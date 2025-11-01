#pragma once
#include <imgui.h>

const char* OpenFileDialogue(const char* title, const char* const* filterPatterns, int filterCount);
const char* OpenFolderDialogue(const char* title);
void LoadImageTooltipWidget(unsigned int textureID, ImVec2 dimension, ImVec2 widgetDim, ImVec4 bgColor, float zoom);
