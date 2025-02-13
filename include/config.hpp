#include "../include/imgui/base.hpp"
#include "../include/imgui/markdown.hpp"
#include "../engine/include/utils/utils.hpp"
#include "../engine/include/utils/json.hpp"
using namespace nlohmann;

static ImVec4 get_vec(json t_src, const char* t_name) {
	if (!t_src.contains(t_name)) return ImVec4(1, 1, 1, 1);
	if (!t_src.at(t_name).is_array()) return ImVec4(1, 1, 1, 1);
	return ImVec4(t_src.at(t_name)[0], t_src.at(t_name)[1], t_src.at(t_name)[2], t_src.at(t_name)[3]);
}
static ImVec4 ColorEdit4(const char* t_name, ImVec4 t_value,
	float t_add_val = 1.0f, float t_min = -1024.f, float t_max = 1024.f) {
	float conversion_array[4] = { t_value.x, t_value.y, t_value.z, t_value.w };
	ImGui::ColorEdit4(t_name, conversion_array);
	return ImVec4(conversion_array[0], conversion_array[1], conversion_array[2], conversion_array[3]);
}
static float prc(float t_in) {
	return ceil(t_in * 100.f) / 100.f;
}

struct imgui_conf {
	bool shouldReloadFonts = false;
	float darkModifier = 0.125f, brightModifier = 0.125f, windowRounding = 1, baseFontSize = 14.0f;
	std::string baseFont = "res/fonts/Ubuntu-Regular.ttf", newBaseFont = baseFont;
	ImVec4 baseColor = ImVec4(0.6f, 0.6f, 0.6f, 0.35f),
		themeColor = ImVec4(1.f, 0.5f, 0.f, 1.f),
		windowColor = ImVec4(0.08f, 0.08f, 0.08f, 0.8f),
		textColor = ImVec4(1.f, 1.f, 1.f, 1.f),
		shadowColor = ImVec4(0.f, 0.f, 0.f, 1.f);

	void loadTheme(std::string tPath) {
		if(!std::filesystem::exists(tPath)) return;
		json data = json::parse(StrFromFile(tPath));
		//Font.
		baseFont = data.at("font").value("file", "res/fonts/Ubuntu-Regular.ttf");
		if(!std::filesystem::exists(baseFont)) baseFont = "res/fonts/Ubuntu-Regular.ttf";
		baseFontSize = data.at("font").value("size", 14.0f);
		//icon_size = data.at("font").value("icons_size", 39.0f);
		//Colors.
		baseColor = get_vec(data.at("colors"), "base");
		themeColor = get_vec(data.at("colors"), "theme");
		windowColor = get_vec(data.at("colors"), "window");
		textColor = get_vec(data.at("colors"), "text");
		shadowColor = get_vec(data.at("colors"), "shadows");
		//Other.
		windowRounding = data.value("rounding", 1.0f);
	}

	void saveTheme(std::string tPath) {
		json data;
		//Other.
		data["rounding"] = prc(windowRounding);
		//Colors.
		data["colors"]["theme"] = { prc(themeColor.x), prc(themeColor.y), prc(themeColor.z), prc(themeColor.w) };
		data["colors"]["window"] = { prc(windowColor.x), prc(windowColor.y), prc(windowColor.z), prc(windowColor.w) };
		data["colors"]["base"] = { prc(baseColor.x), prc(baseColor.y), prc(baseColor.z), prc(baseColor.w) };
		data["colors"]["text"] = { prc(textColor.x), prc(textColor.y), prc(textColor.z), prc(textColor.w) };
		data["colors"]["shadows"] = { prc(shadowColor.x), prc(shadowColor.y), prc(shadowColor.z), prc(shadowColor.w) };
		//Font.
		//data["font"]["icons_size"] = icon_size;
		data["font"]["size"] = baseFontSize;
		data["font"]["file"] = baseFont;
		//Compat.
		data["version"] = 1;
		//Write to file.
		std::ofstream file(tPath);
		file << data;
		file.flush();
	}

	void setTheme() const {
		ImGuiStyle& style = ImGui::GetStyle();
		ImVec4 darkerBase = ImVec4(baseColor.x - darkModifier, baseColor.y - darkModifier, baseColor.z - darkModifier, baseColor.w);
		ImVec4 brightBase = ImVec4(baseColor.x + brightModifier, baseColor.y + brightModifier, baseColor.z + brightModifier, baseColor.w);
		ImVec4 brightTheme = ImVec4(themeColor.x + brightModifier, themeColor.y + brightModifier, themeColor.z + brightModifier, themeColor.w);
		ImVec4 darkerTheme = ImVec4(themeColor.x - darkModifier, themeColor.y - darkModifier, themeColor.z - darkModifier, themeColor.w);
		// Color array
		style.Colors[ImGuiCol_Text] = textColor;
		style.Colors[ImGuiCol_TextDisabled] = baseColor;
		style.Colors[ImGuiCol_WindowBg] = windowColor;
		style.Colors[ImGuiCol_ChildBg] = windowColor;
		style.Colors[ImGuiCol_PopupBg] = windowColor;
		style.Colors[ImGuiCol_Border] = baseColor;
		style.Colors[ImGuiCol_BorderShadow] = shadowColor;
		style.Colors[ImGuiCol_FrameBg] = darkerBase;
		style.Colors[ImGuiCol_FrameBgHovered] = darkerTheme;
		style.Colors[ImGuiCol_FrameBgActive] = baseColor;
		style.Colors[ImGuiCol_TitleBg] = darkerBase;
		style.Colors[ImGuiCol_TitleBgActive] = darkerBase;
		style.Colors[ImGuiCol_TitleBgCollapsed] = darkerBase;
		style.Colors[ImGuiCol_MenuBarBg] = darkerBase;
		style.Colors[ImGuiCol_ScrollbarBg] = darkerBase;
		style.Colors[ImGuiCol_ScrollbarGrabHovered] = baseColor;
		style.Colors[ImGuiCol_ScrollbarGrabActive] = baseColor;
		style.Colors[ImGuiCol_CheckMark] = brightBase;
		style.Colors[ImGuiCol_SliderGrab] = brightBase;
		style.Colors[ImGuiCol_SliderGrabActive] = textColor;
		style.Colors[ImGuiCol_Button] = darkerBase;
		style.Colors[ImGuiCol_ButtonHovered] = baseColor;
		style.Colors[ImGuiCol_ButtonActive] = baseColor;
		style.Colors[ImGuiCol_Header] = darkerBase;
		style.Colors[ImGuiCol_HeaderHovered] = baseColor;
		style.Colors[ImGuiCol_HeaderActive] = baseColor;
		style.Colors[ImGuiCol_Separator] = baseColor;
		style.Colors[ImGuiCol_SeparatorHovered] = themeColor;
		style.Colors[ImGuiCol_SeparatorActive] = brightTheme;
		style.Colors[ImGuiCol_ResizeGrip] = darkerBase;
		style.Colors[ImGuiCol_ResizeGripHovered] = themeColor;
		style.Colors[ImGuiCol_ResizeGripActive] = brightTheme;
		style.Colors[ImGuiCol_Tab] = themeColor;
		style.Colors[ImGuiCol_TabHovered] = brightTheme;
		style.Colors[ImGuiCol_TabActive] = themeColor;
		style.Colors[ImGuiCol_TabSelectedOverline] = brightTheme;
		style.Colors[ImGuiCol_TabUnfocused] = baseColor;
		style.Colors[ImGuiCol_TabUnfocused] = baseColor;
		style.Colors[ImGuiCol_TabUnfocusedActive] = baseColor;
		style.Colors[ImGuiCol_DockingPreview] = themeColor;
		style.Colors[ImGuiCol_DockingEmptyBg] = themeColor;
		style.Colors[ImGuiCol_PlotLines] = textColor;
		style.Colors[ImGuiCol_PlotLinesHovered] = baseColor;
		style.Colors[ImGuiCol_PlotHistogram] = textColor;
		style.Colors[ImGuiCol_PlotHistogramHovered] = baseColor;
		style.Colors[ImGuiCol_TableHeaderBg] = windowColor;
		style.Colors[ImGuiCol_TableBorderStrong] = darkerBase;
		style.Colors[ImGuiCol_TableBorderLight] = baseColor;
		style.Colors[ImGuiCol_TableRowBg] = shadowColor;
		style.Colors[ImGuiCol_TableRowBgAlt] = textColor;
		style.Colors[ImGuiCol_TextSelectedBg] = darkerBase;
		style.Colors[ImGuiCol_TextLink] = themeColor;
		style.Colors[ImGuiCol_DragDropTarget] = darkerBase;
		style.Colors[ImGuiCol_NavHighlight] = baseColor;
		style.Colors[ImGuiCol_NavWindowingHighlight] = baseColor;
		style.Colors[ImGuiCol_NavWindowingDimBg] = baseColor;
		style.Colors[ImGuiCol_ModalWindowDimBg] = baseColor;
		//Additional style.
		style.ButtonTextAlign = ImVec2(0.f, 0.5f);
		style.WindowRounding = windowRounding;
		style.FrameRounding = style.WindowRounding;
	}

	void drawThemeEditor(bool* tDraw) {
		if(ImGui::Begin("Theme editor", tDraw)) {
			ImGui::SliderFloat(u8"Скругление", &windowRounding, 0.f, 10.f);
			ImGui::Text(u8"Цвета");
			ImGui::SliderFloat(u8"Затемнение доп.цветов", &darkModifier, 0.f, 1.f);
			ImGui::SliderFloat(u8"Осветление доп.цветов", &brightModifier, 0.f, 1.f);
			baseColor = ColorEdit4(u8"Базовый цвет", baseColor);
			themeColor = ColorEdit4(u8"Цвет темы", themeColor);
			windowColor = ColorEdit4(u8"Цвет окна", windowColor);
			textColor = ColorEdit4(u8"Цвет шрифта", textColor);
			shadowColor = ColorEdit4(u8"Цвет теней", shadowColor);
			ImGui::Text(u8"Шрифт");
			ImGui::InputText(u8"Путь", &newBaseFont);
			ImGui::SliderFloat(u8"Размер", &baseFontSize, 0.1f, 100.f);
			if(ImGui::Button(u8"Перезагрузить шрифты")) shouldReloadFonts = true;
			setTheme();
			ImGui::End();
		}
	}

};