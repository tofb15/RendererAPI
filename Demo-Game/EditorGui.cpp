#include "EditorGui.h"

#include "../D3D12Engine/ResourceManager.h"
#include "../D3D12Engine/WindowInput.hpp"
#include "../D3D12Engine/Utills/Utills.h"
#include "../D3D12Engine/Texture.hpp"
#include "../D3D12Engine/Material.hpp"
#include "../D3D12Engine/Window.hpp"
#include "../D3D12Engine/Mesh.hpp"

#include "../D3D12Engine/External/IMGUI/imgui.h"
#include "../D3D12Engine/External/IMGUI/imgui_internal.h"

#include <algorithm>
#include "GameObject.h"
#include <unordered_set>

EditorGUI::EditorGUI(std::vector<Object*>* sceneObjects, WindowInput* windowInput, ResourceManager* resourceManager, Window* w) {
	m_objects = sceneObjects;
	m_windowInput = windowInput;
	m_resourceManager = resourceManager;
	m_window = w;

	FileSystem::ListDirectory(m_resourceFileBrowser, m_resourceManager->GetAssetPath(), 1);
}

EditorGUI::~EditorGUI() {
}

void EditorGUI::RenderGUI() {
	m_window->BeginUIRendering();
	m_icon_blueprint_id = nullptr;
	m_icon_material_id = nullptr;
	m_icon_scene_id = nullptr;
	m_icon_shader_id = nullptr;
	m_icon_up_folder_id = nullptr;
	m_icon_down_folder_id = nullptr;
	m_icon_object_id = nullptr;
	m_icon_unknown_id = nullptr;

	//static bool b = true;
	//ImGui::ShowDemoWindow(&b);

	RenderMenuBar();
	RenderSceneWindow();
	RenderPropertiesWindow();
	RenderResourceWindow();
}

void EditorGUI::RenderMenuBar() {
	// ===========Menu==============
	if (ImGui::BeginMainMenuBar()) {
		if (ImGui::BeginMenu("Project")) {
			if (ImGui::MenuItem("New Project", "")) {

			}
			if (ImGui::MenuItem("Open Project", "")) {}
			if (ImGui::MenuItem("Save", "", false)) {}
			if (ImGui::MenuItem("Save As", "", false)) {}

			//ShowExampleMenuFile();
			ImGui::EndMenu();
		}
		if (ImGui::BeginMenu("Scene")) {
			if (ImGui::MenuItem("New Scene", "")) {

			}

			//ShowExampleMenuFile();
			ImGui::EndMenu();
		}
		if (ImGui::BeginMenu("Edit")) {
			if (ImGui::MenuItem("Undo", "CTRL+Z")) {}
			if (ImGui::MenuItem("Redo", "CTRL+Y", false, false)) {}  // Disabled item
			ImGui::Separator();
			if (ImGui::MenuItem("Cut", "CTRL+X")) {}
			if (ImGui::MenuItem("Copy", "CTRL+C")) {}
			if (ImGui::MenuItem("Paste", "CTRL+V")) {}
			ImGui::EndMenu();
		}
		ImGui::EndMainMenuBar();
	}
}

void EditorGUI::RenderSceneWindow() {
	ImGui::SetNextWindowSize(ImVec2(200, 500), ImGuiCond_Once);
	ImGui::SetNextWindowPos(ImVec2(10, 20), ImGuiCond_Once);

	if (ImGui::Begin("Scene", NULL, ImGuiWindowFlags_None)) {
		//=====Top Part=====
		ImGui::BeginChild("Scene Object List", ImVec2(0, ImGui::GetWindowHeight() - 100), true);
		if (ImGui::IsItemActive()) {
			m_currentPropertyWindowState = SceneObject;
		}
		int i = 0;
		for (auto& e : *m_objects) {
			if (ImGui::Selectable(("obj#" + std::to_string(i) + " : " + m_resourceManager->GetBlueprintName(e->blueprint)).c_str(), Contains<std::vector, int>(m_selectedSceneObjects, i))) {
				//m_selectedObjects.push_back(i);
			}

			if (ImGui::IsItemClicked(0) && ImGui::IsItemHovered()) {
				m_currentPropertyWindowState = SceneObject;

				if (!m_selectedSceneObjects.empty() && m_windowInput->IsKeyDown(WindowInput::KEY_CODE_SHIFT)) {
					int min = m_selectedSceneObjects.front();
					int max = i;
					if (min > max) {
						std::swap(min, max);
					}

					m_selectedSceneObjects.clear();
					for (int a = min; a <= max; a++) {
						m_selectedSceneObjects.push_back(a);
						m_selectedSceneObjectBlueprints.insert(m_objects->at(a)->blueprint);
					}
				} else if (!m_selectedSceneObjects.empty() && m_windowInput->IsKeyDown(WindowInput::KEY_CODE_CTRL)) {
					if (!Contains<std::vector, int>(m_selectedSceneObjects, i)) {
						m_selectedSceneObjects.push_back(i);
						m_selectedSceneObjectBlueprints.insert(m_objects->at(i)->blueprint);
					}
				} else {
					m_selectedSceneObjects.clear();
					m_selectedSceneObjects.push_back(i);

					m_selectedSceneObjectBlueprints.clear();
					m_selectedSceneObjectBlueprints.insert(m_objects->at(i)->blueprint);
				}
			}
			i++;
		}
		ImGui::EndChild();
		//=====Bottom Part=====
		//ImGui::SetNextWindowPos(ImVec2(0, 500));
		ImGui::BeginChild("Actions", ImVec2(0, 0), true);

		size_t numberOfObjects = (int)m_objects->size();
		size_t nSelected = (int)m_selectedSceneObjects.size();

		if (nSelected > 0) {
			if (ImGui::Button("Copy")) {
				for (auto i : m_selectedSceneObjects) {
					Object* obj = new Object();
					memcpy(obj, m_objects->at(i), sizeof(Object));
					m_objects->push_back(obj);
				}
				m_selectedSceneObjects.clear();
				for (size_t i = numberOfObjects; i < numberOfObjects + nSelected; i++) {
					m_selectedSceneObjects.push_back(i);
				}
			}
			ImGui::SameLine();

			if (ImGui::Button("Delete")) {
				std::sort(m_selectedSceneObjects.begin(), m_selectedSceneObjects.end(), std::greater <int>());
				for (auto i : m_selectedSceneObjects) {
					delete m_objects->at(i);
					m_objects->erase(m_objects->begin() + i);
				}

				m_selectedSceneObjects.clear();
				m_selectedSceneObjectBlueprints.clear();
				if (!m_objects->empty()) {
					m_selectedSceneObjects.push_back(0);
					m_selectedSceneObjectBlueprints.insert(m_objects->front()->blueprint);
				}
			}

			if (ImGui::Button("Select All Identical")) {
				m_selectedSceneObjects.clear();
				int index = 0;
				for (auto i : *m_objects) {
					if (m_selectedSceneObjectBlueprints.count(i->blueprint) > 0) {
						m_selectedSceneObjects.push_back(index);
					}
					index++;
				}
			}
		}
		ImGui::EndChild();

	}
	ImGui::End();
}

void EditorGUI::RenderResourceWindow() {
	ImGui::SetNextWindowSize(ImVec2(1900, 200), ImGuiCond_Once);
	ImGui::SetNextWindowPos(ImVec2(0, 1080 - 250), ImGuiCond_Once);

	auto drawIcon = [&](ImTextureID textureId, ImVec2 selectableSize, std::string text, bool selected) ->bool {
		ImGui::BeginGroup();
		bool clicked = false;
		int width = std::min(selectableSize.x, selectableSize.y - 25);
		ImVec2 imgSize(width, width);

		ImVec2 pos = ImGui::GetCursorPos();
		if (ImGui::Selectable("##item", selected, 0, selectableSize)) {
			clicked = true;
		}
		pos.x -= 1;
		ImGui::SetCursorPos(pos);
		ImGui::PushTextWrapPos(pos.x + selectableSize.x - 2);
		ImGui::Image(textureId, imgSize, ImVec2(0, 0), ImVec2(1, 1), ImVec4(1.0f, 1.0f, 1.0f, 1.0f), ImVec4(1.0f, 1.0f, 1.0f, 0.5f));
		pos = ImGui::GetCursorPos();

		if (ImGui::IsItemHovered()) {
			m_hoveredFile = text;
		}
		ImGui::TextWrapped(text.c_str());

		ImGui::SetCursorPos(pos);
		ImGui::PopTextWrapPos();

		ImGui::EndGroup();
		return clicked;
	};

	if (ImGui::Begin("Resources", NULL, ImGuiWindowFlags_None)) {
		if (ImGui::BeginTabBar("##Tabs", ImGuiTabBarFlags_None)) {
			if (ImGui::BeginTabItem("Resource Browser")) {
				ImGui::Text(("Path: " + m_resourceFileBrowser.path.string() + " : " + m_hoveredFile).c_str());
				ImGui::Text(("File: " + m_selectedFile).c_str());

				ImGuiStyle& style = ImGui::GetStyle();
				float window_visible_x2 = ImGui::GetWindowPos().x + ImGui::GetWindowContentRegionMax().x;

				ImVec2 imgSize(64, 94);
				int nEntries = m_resourceFileBrowser.files.size() + m_resourceFileBrowser.directories.size();
				std::filesystem::path textureFolderPath = m_resourceManager->GetAssetPath();

				int i = 0;
				if (m_resourceFileBrowser.path.string() != textureFolderPath) {
					nEntries++;
					ImGui::PushID(i);
					bool clicked = drawIcon(SelectFileIcon(""), imgSize, "..", false);
					ImGui::PopID();
					float last_button_x2 = ImGui::GetItemRectMax().x;
					float next_button_x2 = last_button_x2 + style.ItemSpacing.x + imgSize.x; // Expected position if next button was on same line
					if ((i + 1) < nEntries && next_button_x2 < window_visible_x2)
						ImGui::SameLine();

					if (clicked) {
						std::filesystem::path parrent = m_resourceFileBrowser.path.parent_path().parent_path();
						FileSystem::ListDirectory(m_resourceFileBrowser, parrent.string() + "/", 1);
						m_selectedFile = "";
					}

					i++;
				}

				for (auto& e : m_resourceFileBrowser.directories) {
					ImGui::PushID(i);

					bool clicked = drawIcon(SelectFileIcon(e.path), imgSize, e.path.stem().string(), false);
					ImGui::PopID();
					float last_button_x2 = ImGui::GetItemRectMax().x;
					float next_button_x2 = last_button_x2 + style.ItemSpacing.x + imgSize.x; // Expected position if next button was on same line
					if ((i + 1) < nEntries && next_button_x2 < window_visible_x2)
						ImGui::SameLine();

					if (clicked) {
						FileSystem::ListDirectory(m_resourceFileBrowser, e.path.string() + "/", 1);
						m_selectedFile = "";
					}

					i++;
				}

				for (auto& e : m_resourceFileBrowser.files) {
					ImGui::PushID(i);
					std::string fName = e.path.stem().string() + e.path.extension().string();
					bool clicked = drawIcon(SelectFileIcon(e.path), imgSize, fName, fName == m_selectedFile);
					ImGui::PopID();
					float last_button_x2 = ImGui::GetItemRectMax().x;
					float next_button_x2 = last_button_x2 + style.ItemSpacing.x + imgSize.x; // Expected position if next button was on same line
					if ((i + 1) < nEntries && next_button_x2 < window_visible_x2)
						ImGui::SameLine();
					if (clicked) {
						HandleResourceClick(e.path);
					}
					i++;
				}
				ImGui::EndTabItem();
			}

			ImGui::EndTabBar();
		}
	}
	ImGui::End();
}

void EditorGUI::RenderPropertiesWindow() {
	ImGui::SetNextWindowSize(ImVec2(250, 500), ImGuiCond_Once);
	ImGui::SetNextWindowPos(ImVec2(1920 - 260, 20), ImGuiCond_Once);

	if (ImGui::Begin("Properties", NULL, ImGuiWindowFlags_None)) {
		switch (m_currentPropertyWindowState) {
		case EditorGUI::SceneObject:
			RenderPropertyWindowSceneObject();
			break;
		case EditorGUI::BlueprintEditor:
			RenderPropertyWindowBlueprint();
			break;
		case EditorGUI::MaterialEditor:
			break;
		default:
			break;
		}
	}
	ImGui::End();
}

void EditorGUI::RenderPropertyWindowSceneObject() {
	size_t nSelected = m_selectedSceneObjects.size();
	if (nSelected == 0) {
		ImGui::Text("No Object Selected.");
		return;
	} else if (nSelected == 1) {
		std::string name = "obj#" + std::to_string(m_selectedSceneObjects.front());
		ImGui::Text(("Selected: " + name).c_str());
	} else {
		ImGui::Text(("Selected: " + std::to_string(nSelected) + " Objects").c_str());
	}
	ImGui::Separator();

	ImGui::Separator();

	if (ImGui::CollapsingHeader("Transform")) {
		if (m_selectedSceneObjects.size() == 1) {
			int& selectedObject = m_selectedSceneObjects.front();

			ImGui::DragFloat3("Pos", (float*)& m_objects->at(selectedObject)->transform.pos, 0.1, -100, 100);
			ImGui::DragFloat3("Rot", (float*)& m_objects->at(selectedObject)->transform.rotation, 0.1, -100, 100);
			ImGui::DragFloat3("Scale", (float*)& m_objects->at(selectedObject)->transform.scale, 0.1, -100, 100);
		}
		if (ImGui::Button("Zero Rotation")) {
			for (auto i : m_selectedSceneObjects) {
				m_objects->at(i)->transform.rotation = { 0,0,0 };
			}
		}

		if (ImGui::Button("Random X-Rot")) {
			for (auto i : m_selectedSceneObjects) {
				m_objects->at(i)->transform.rotation.x = (rand() / (float)RAND_MAX) * 2 * 3.15;
			}
		}

		ImGui::SameLine();

		if (ImGui::Button("Random Y-Rot")) {
			for (auto i : m_selectedSceneObjects) {
				m_objects->at(i)->transform.rotation.y = (rand() / (float)RAND_MAX) * 2 * 3.15;
			}
		}

		ImGui::SameLine();

		if (ImGui::Button("Random Z-Rot")) {
			for (auto i : m_selectedSceneObjects) {
				m_objects->at(i)->transform.rotation.z = (rand() / (float)RAND_MAX) * 2 * 3.15;
			}
		}

		/////////////

		if (ImGui::Button("Move to origin")) {
			for (auto i : m_selectedSceneObjects) {
				m_objects->at(i)->transform.pos = { 0,0,0 };
			}
		}

		ImGui::DragFloat("Spread", &m_maxRandomPos, 1, 0, 10000);

		if (ImGui::Button("Random X-Pos")) {
			for (auto i : m_selectedSceneObjects) {
				m_objects->at(i)->transform.pos.x = (rand() / (float)RAND_MAX) * m_maxRandomPos - m_maxRandomPos / 2;
			}
		}

		ImGui::SameLine();

		if (ImGui::Button("Random Y-Pos")) {
			for (auto i : m_selectedSceneObjects) {
				m_objects->at(i)->transform.pos.y = (rand() / (float)RAND_MAX) * m_maxRandomPos - m_maxRandomPos / 2;
			}
		}

		ImGui::SameLine();

		if (ImGui::Button("Random Z-Pos")) {
			for (auto i : m_selectedSceneObjects) {
				m_objects->at(i)->transform.pos.z = (rand() / (float)RAND_MAX) * m_maxRandomPos - m_maxRandomPos / 2;
			}
		}
	}


	ImGui::Separator();
}

void EditorGUI::RenderPropertyWindowBlueprint() {
	if (!m_selectedBlueprint) {
		m_currentPropertyWindowState = None;
		return;
	}

	std::string bp_name = m_resourceManager->GetBlueprintName(m_selectedBlueprint);
	ImGui::Text(("Selected blueprint: " + bp_name).c_str());

	std::string name = "";
	name = m_resourceManager->GetMeshName(m_selectedBlueprint->mesh);
	name = name.substr(0, name.find_last_of("."));
	if (ImGui::BeginCombo("Mesh Select", name.c_str())) {
		std::filesystem::path clickedItem = DrawRecursiveDirectoryList(m_resourceManager->m_foundMeshes, name);
		if (clickedItem != "") {
			size_t len1 = m_resourceManager->m_foundMeshes.path.string().length();
			size_t len2 = clickedItem.string().length() - len1;

			std::string s = clickedItem.string().substr(len1, len2);

			m_selectedBlueprint->hasChanged = true;
			m_selectedBlueprint->mesh = m_resourceManager->GetMesh(s);
			int nNeededMaterials = m_selectedBlueprint->mesh->GetNumberOfSubMeshes();
			for (size_t i = m_selectedBlueprint->materials.size(); i < nNeededMaterials; i++) {
				m_selectedBlueprint->materials.push_back(m_selectedBlueprint->materials.back());
			}

			if (m_selectedBlueprint->materials.size() > nNeededMaterials) {
				m_selectedBlueprint->materials.erase(m_selectedBlueprint->materials.begin() + nNeededMaterials, m_selectedBlueprint->materials.end());
			}
		}
		ImGui::EndCombo();
	}
	ImGui::Separator();

	int nMaterials = m_selectedBlueprint->materials.size();
	for (int i = 0; i < nMaterials; i++) {
		ImGui::PushID(i);

		name = m_resourceManager->GetMaterialName(m_selectedBlueprint->materials[i]);
		name = name.substr(0, name.find_last_of("."));
		if (ImGui::BeginCombo(("Select Material #" + std::to_string(i)).c_str(), name.c_str())) {
			std::filesystem::path clickedItem = DrawRecursiveDirectoryList(m_resourceManager->m_foundMaterials, name);
			if (clickedItem != "") {
				size_t len1 = m_resourceManager->m_foundMaterials.path.string().length();
				size_t len2 = clickedItem.string().length() - len1;

				std::string s = clickedItem.string().substr(len1, len2);

				m_selectedBlueprint->hasChanged = true;
				m_selectedBlueprint->materials[i] = m_resourceManager->GetMaterial(s);
			}
			ImGui::EndCombo();
		}

		ImGui::PopID();
	}


}

void EditorGUI::HandleResourceClick(std::filesystem::path clickedResource) {
	m_selectedFile = clickedResource.stem().string() + clickedResource.extension().string();
	std::string ext = clickedResource.extension().string();

	for (auto& c : ext) {
		c = std::tolower(c);
	}
	if (ext == ".bp") {
		std::string blueprintFolder = m_resourceManager->GetAssetPath() + BLUEPRINT_FOLDER_NAME;
		std::string name = clickedResource.string().substr(blueprintFolder.length(), clickedResource.string().length());
		m_selectedBlueprint = m_resourceManager->GetBlueprint(name);

		if (m_selectedBlueprint) {
			m_currentPropertyWindowState = BlueprintEditor;
		} else {
			m_currentPropertyWindowState = None;
		}
	} else if (ext == ".scene") {
		m_currentPropertyWindowState = None;
	} else if (ext == ".hlsl" || ext == ".hlsli") {
		m_currentPropertyWindowState = None;
	} else if (ext == ".png" || ext == ".dds") {
	}
}

void* EditorGUI::SelectFileIcon(std::filesystem::path clickedResource) {

	if (clickedResource == "") {
		if (!m_icon_up_folder_id) {
			m_icon_up_folder_id = m_window->PrepareTextureForGuiRendering(m_resourceManager->GetTexture(m_icon_up_folder));
		}
		return m_icon_up_folder_id;
	} else {
		std::string ext = clickedResource.extension().string();
		for (auto& c : ext) {
			c = std::tolower(c);
		}


		if (ext == "") {
			if (!m_icon_down_folder_id) {
				m_icon_down_folder_id = m_window->PrepareTextureForGuiRendering(m_resourceManager->GetTexture(m_icon_down_folder));
			}
			return m_icon_down_folder_id;
		} else if (ext == ".bp") {
			if (!m_icon_blueprint_id) {
				m_icon_down_folder_id = m_window->PrepareTextureForGuiRendering(m_resourceManager->GetTexture(m_icon_blueprint));
			}
			return m_icon_down_folder_id;
		} else if (ext == ".scene") {
			if (!m_icon_scene_id) {
				m_icon_scene_id = m_window->PrepareTextureForGuiRendering(m_resourceManager->GetTexture(m_icon_scene));
			}
			return m_icon_scene_id;
		} else if (ext == ".mat") {
			if (!m_icon_material_id) {
				m_icon_material_id = m_window->PrepareTextureForGuiRendering(m_resourceManager->GetTexture(m_icon_material));
			}
			return m_icon_material_id;
		} else if (ext == ".sp" || ext == ".hlsl" || ext == ".hlsli") {
			if (!m_icon_shader_id) {
				m_icon_shader_id = m_window->PrepareTextureForGuiRendering(m_resourceManager->GetTexture(m_icon_shader));
			}
			return m_icon_shader_id;
		} else if (ext == ".obj") {
			if (!m_icon_object_id) {
				m_icon_object_id = m_window->PrepareTextureForGuiRendering(m_resourceManager->GetTexture(m_icon_object));
			}
			return m_icon_object_id;
		} else if (ext == ".png" || ext == ".dds") {

			std::string textureFolder = m_resourceManager->GetAssetPath() + TEXTURE_FODLER_NAME;
			std::string clickedPath = clickedResource.string();
			std::string textureName = clickedPath.substr(textureFolder.length(), clickedPath.length());

			Texture* tex = m_resourceManager->GetTexture(textureName);
			if (tex && tex->IsLoaded()) {
				return m_window->PrepareTextureForGuiRendering(tex);
			} else {
				return SelectFileIcon("x.x");
			}
		}

		if (!m_icon_unknown_id) {
			m_icon_unknown_id = m_window->PrepareTextureForGuiRendering(m_resourceManager->GetTexture(m_icon_unknown));
		}
		return m_icon_unknown_id;
	}
}

std::filesystem::path EditorGUI::DrawRecursiveDirectoryList(const FileSystem::Directory& path, const std::string& selectedItem, const bool isMenuBar, unsigned int currDepth) {
	std::filesystem::path clicked = "";
	if (path.files.empty() && path.directories.empty()) {
		ImGui::Text("-Empty Directory-");
		return "";
	}

	std::string depthStr;
	for (size_t i = 0; i < currDepth; i++) {
		depthStr += "-";
	}

	for (auto& e : path.directories) {
		if (isMenuBar) {
			if (ImGui::BeginMenu((e.path.filename().string()).c_str())) {
				//clicked = RecursiveDirectoryList(e, selectedItem, isMenuBar, currDepth + 1);
				ImGui::EndMenu();
			}
		} else {
			if (ImGui::CollapsingHeader((depthStr + e.path.filename().string()).c_str())) {
				//clicked = RecursiveDirectoryList(e, selectedItem, isMenuBar, currDepth + 1);
			}
		}
	}

	for (auto& e : path.files) {
		std::string name = e.path.filename().string().substr(0, e.path.filename().string().find_last_of("."));
		if (!isMenuBar) {
			name = depthStr + name;
		}
		if (ImGui::Selectable(name.c_str(), selectedItem == name)) {
			clicked = e.path;
		}
	}

	return clicked;
}
