#include "ImageContainer.h"
#include <ImGuiimageloader.h>

#pragma region Image Renderer

ImageRenderer::ImageRenderer(std::shared_ptr<ImageEntry> Image) : source(Image), textureID(0), textureLoaded(false), width(0), height(0) {};

void ImageRenderer::LoadGPU() {
	width = source->GetWidth();
	height = source->GetHeight();
	LoadImage_s(textureID, reinterpret_cast<void*>(const_cast<PixelRGBA*>(source->ReadImageData().data())), width, height, source->GetChannels());
	textureLoaded = true;
}

void ImageRenderer::UnloadGPU() {
	width = 0;
	height = 0;
	UnloadImage_s(textureID);
	textureLoaded = false;
}

int ImageRenderer::DisplayImage(ImVec2 widgetDimension, ImVec4 bgColor) {
	if (textureLoaded) {
		ImVec2 uv_min = ImVec2(0.0f, 0.0f);
		ImVec2 uv_max = ImVec2(1.0f, 1.0f);
		ImGuiIO& io = ImGui::GetIO();
		ImVec2 pos = ImGui::GetCursorScreenPos();
		ImVec2 imageDimension = ImVec2(width, height);

		ImGui::ImageWithBg(textureID, imageDimension, uv_min, uv_max, bgColor);

		if (ImGui::BeginItemTooltip())
		{

			float region_x = io.MousePos.x - pos.x - widgetDimension.x * 0.5f; //Top left XY Coordinates of the region
			float region_y = io.MousePos.y - pos.y - widgetDimension.y * 0.5f;
			float zoom = 8.0f;

			//Clamp the region to be within the texture bounds
			if (region_x < 0.0f) { region_x = 0.0f; }
			else if (region_x > imageDimension.x - widgetDimension.x) { region_x = imageDimension.x - widgetDimension.x; }

			if (region_y < 0.0f) { region_y = 0.0f; }
			else if (region_y > imageDimension.y - widgetDimension.y) { region_y = imageDimension.y - widgetDimension.y; }

			// Display the region coordinates and size
			ImGui::Text("Image coord (%.2f, %.2f)", io.MousePos.x - pos.x, io.MousePos.y - pos.y);
			ImGui::SameLine();
			PixelRGBA P;
			if (source->GetPixel(io.MousePos.x - pos.x, io.MousePos.y - pos.y, P) == 0) {
				ImGui::Text("RGBA Val (%.2f, %.2f, %.2f, %.2f)", P.r, P.g, P.b, P.a);
			}
			ImGui::Text("Min: (%.2f, %.2f)", region_x, region_y);
			ImGui::Text("Max: (%.2f, %.2f)", region_x + widgetDimension.x, region_y + widgetDimension.y);


			ImVec2 uv0 = ImVec2((region_x) / imageDimension.x, (region_y) / imageDimension.y);
			ImVec2 uv1 = ImVec2((region_x + widgetDimension.x) / imageDimension.x, (region_y + widgetDimension.y) / imageDimension.y);
			ImGui::ImageWithBg(textureID, ImVec2(widgetDimension.x * zoom, widgetDimension.y * zoom), uv0, uv1, bgColor);
			ImGui::EndTooltip();
		}
		return 0;
	}
	else return -1;
}

#pragma endregion

