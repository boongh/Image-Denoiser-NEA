#pragma once

#include "CustomWidget.h"
#include "imgui.h"
#include <tinyfiledialogs.h>
#include <iostream>

const char* OpenFileDialogue(const char* title, const char* const* filterPatterns, int filterCount)
{
    const char* file = tinyfd_openFileDialog(
        "Select an Image",
        "",
        filterCount,
        filterPatterns,
        "Image Files",
		true // Allow multiple file selection
    );

    if (file) {
        std::cout << "Selected file: " << file << std::endl;
        return file;
    }
    else {
        std::cout << "No file selected." << std::endl;
        return nullptr;
    }

}

void LoadImageTooltipWidget(unsigned int textureID, ImVec2 dimension, ImVec2 widgetDim, ImVec4 bgColor, float zoom)
{
    ImVec2 uv_min = ImVec2(0.0f, 0.0f);
    ImVec2 uv_max = ImVec2(1.0f, 1.0f);      
    ImGuiIO& io = ImGui::GetIO();
    ImVec2 pos = ImGui::GetCursorScreenPos();

    ImGui::ImageWithBg(textureID, dimension, uv_min, uv_max, bgColor);
    
    if (ImGui::BeginItemTooltip())
    {
       
    	float region_x = io.MousePos.x - pos.x - widgetDim.x* 0.5f; //Top left XY Coordinates of the region
        float region_y = io.MousePos.y - pos.y - widgetDim.y * 0.5f;
        float zoom = 8.0f;
    
    	//Clamp the region to be within the texture bounds
        if (region_x < 0.0f) { region_x = 0.0f; }
        else if (region_x > dimension.x - widgetDim.x) { region_x = dimension.x - widgetDim.x; }
        if (region_y < 0.0f) { region_y = 0.0f; }
        else if (region_y > dimension.y - widgetDim.y) { region_y = dimension.y - widgetDim.y; }
    
        // Display the region coordinates and size
        ImGui::Text("Image coord (%.2f, %.2f)", io.MousePos.x - pos.x, io.MousePos.y - pos.y);

        ImGui::Text("Min: (%.2f, %.2f)", region_x, region_y);
        ImGui::Text("Max: (%.2f, %.2f)", region_x + widgetDim.x, region_y + widgetDim.y);
    
    
        ImVec2 uv0 = ImVec2((region_x) / dimension.x, (region_y) / dimension.y);
        ImVec2 uv1 = ImVec2((region_x + widgetDim.x) / dimension.x, (region_y + widgetDim.y) / dimension.y);
        ImGui::ImageWithBg(textureID, ImVec2(widgetDim.x * zoom, widgetDim.y * zoom), uv0, uv1, bgColor);
        ImGui::EndTooltip();
    }
}
