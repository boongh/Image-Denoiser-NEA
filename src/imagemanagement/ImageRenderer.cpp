#include "ImageContainer.h"
#include <ImGuiimageloader.h>
#include <imgui_internal.h>
#include <algorithm>
#include <iostream>
#include <tuple>

#pragma region image Renderer

std::shared_ptr<ImageRenderer> ImageRenderer::CreateImageRenderer(std::shared_ptr<ImageEntry> image)
{
	return std::make_shared<ImageRenderer>(image, nullptr);
}

std::shared_ptr<ImageRenderer> ImageRenderer::CreateErrorRenderer(const char* msg)
{
	return std::make_shared<ImageRenderer>(nullptr, msg);
}

ImageRenderer::ImageRenderer(std::shared_ptr<ImageEntry> image, const char* errMsg) : tag(_strdup(errMsg)), source(image), textureID(0), textureLoaded(false), width(0), height(0) {}

ImageRenderer::~ImageRenderer() { UnloadGPU(); };

void ImageRenderer::LoadGPU() {
	if (source == nullptr) {
		return;
	}

	if (!textureLoaded) {
		width = source->GetWidth();
		height = source->GetHeight();
		std::tuple<
			std::shared_lock<std::shared_mutex>,
			std::shared_ptr<const RGBAImageI>> img = source->RGBAIRead();

		if (std::get<1>(img) == nullptr) {
			//Error occured
			source == nullptr;
			tag = "error occured while loading to GPU";
			return;
		}
		LoadImage_s(textureID, reinterpret_cast<const void*>(std::get<1>(img)->data.data()), width, height, source->GetChannels());


		textureLoaded = true;
	}
}

void ImageRenderer::UnloadGPU() {
	if (source == nullptr) {
		return;
	}
	if (textureLoaded) {
		width = 0;
		height = 0;
		UnloadImage_s(textureID);
		textureLoaded = false;
	}
}

/// <summary>
/// Renders image out as a window
/// </summary>
/// <param name="widgetDimension"></param>
/// <param name="bgColor"></param>
/// <returns></returns>
int ImageRenderer::DisplayImage(ImVec2 widgetDimension, ImVec4 bgColor) {
	if (source == nullptr) {
		ImGui::LabelText("ErrorLable", "Unexpected error type %s", tag);
		return 0;
	}

	if (!textureLoaded) {
		ImGui::LabelText("ErrorLable", "Unexpected error: texture not loaded");
		return 0;
	}

	//Valid renderer
	ImVec2 uv_min = ImVec2(0.0f, 0.0f);
	ImVec2 uv_max = ImVec2(1.0f, 1.0f);
	ImGuiIO& io = ImGui::GetIO();
	ImVec2 pos = ImGui::GetCursorScreenPos();
	static float scale = 1;
	static float targetScale = 1;

	scale = std::min(scale, 10.0f);
	scale = std::max(scale, 0.1f);

	targetScale = std::min(scale, 10.0f);
	targetScale = std::max(scale, 0.1f);

	ImVec2 displayImageDimension = ImVec2(width * scale, height * scale);
	ImVec2 imgPixelPos = ImVec2((io.MousePos.x - pos.x) / scale, (io.MousePos.y - pos.y) / scale);


	if (ImGui::IsKeyDown(ImGuiMod_Ctrl)) {
		targetScale -= (io.MouseWheel < 0) * targetScale / 5;
		targetScale += (io.MouseWheel > 0) * targetScale / 5;
	}

	float deltaScale = targetScale - scale;
	scale += deltaScale / 10;

	ImGui::ImageWithBg(textureID, displayImageDimension, uv_min, uv_max, bgColor);

	if (ImGui::BeginItemTooltip())
	{
		float zoom = 1 / (scale);

		float region_x = (imgPixelPos.x - widgetDimension.x * 0.5f * zoom); //Top left XY Coordinates of the region
		float region_y = (imgPixelPos.y - widgetDimension.y * 0.5f * zoom);

		//Clamp the region to be within the texture bounds
		//0 Min
		region_x = (region_x >= 0.0f) * region_x;
		region_y = (region_y >= 0.0f) * region_y;


		if (region_x > width - widgetDimension.x * zoom) {
			region_x = width - widgetDimension.x * zoom;
		}
		if (region_y > height - widgetDimension.y * zoom) {
			region_y = height - widgetDimension.y * zoom;
		}

		// Display the region coordinates and size
		ImGui::Text("Image coord (%.0f, %.0f)", imgPixelPos.x, imgPixelPos.y);
		ImGui::SameLine();
		PixelRGBA P;
		if (source->GetPixel(imgPixelPos.x, imgPixelPos.y, P) == 0) {
			ImGui::Text("RGBA Val (%d, %d, %d, %d)", P.r, P.g, P.b, P.a);
		}
		ImGui::Text("Min: (%.0f, %.0f)", region_x, region_y);
		ImGui::Text("Max: (%.0f, %.0f)", region_x + widgetDimension.x * zoom, region_y + widgetDimension.y * zoom);


		ImVec2 uv0 = ImVec2((region_x) / width, (region_y) / height);
		ImVec2 uv1 = ImVec2(std::min((region_x + widgetDimension.x * zoom) / width, 1.0f), std::min((region_y + widgetDimension.y * zoom) / height, 1.0f));
		ImGui::ImageWithBg(textureID, ImVec2(8 * widgetDimension.x, 8 * widgetDimension.y), uv0, uv1, bgColor);
		ImGui::EndTooltip();
	}

}

#pragma endregion