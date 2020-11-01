#include "Editor.h"

#include "FusionReactor/src/ResourceManager.h"
#include "FusionReactor/src/WindowInput.hpp"
#include "FusionReactor/src/Utills/Utills.h"
#include "FusionReactor/src/Texture.hpp"
#include "FusionReactor/src/Material.hpp"
#include "FusionReactor/src/Window.hpp"
#include "FusionReactor/src/Mesh.hpp"
#include "FusionReactor/src/GameObject.h"
#include "FusionReactor/src/Scene.h"

#include "FusionReactor/src/External/IMGUI/imgui.h"
#include "FusionReactor/src/External/IMGUI/imgui_internal.h"

#include <algorithm>
#include <unordered_set>
#include <iterator> 

using namespace FusionReactor;

Editor::Editor() {
	m_icon_blueprint_id = nullptr;
	m_icon_material_id = nullptr;
	m_icon_scene_id = nullptr;
	m_icon_shader_id = nullptr;
	m_icon_up_folder_id = nullptr;
	m_icon_down_folder_id = nullptr;
	m_icon_object_id = nullptr;
	m_icon_unknown_id = nullptr;
}

Editor::~Editor() {
}

void Editor::RenderGUI() {
	m_windows[0]->BeginUIRendering();
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
	RenderDebugWindow();
	RenderMenuBar();
	RenderSceneWindow();
	RenderPropertiesWindow();
	RenderResourceWindow();
}

int Editor::Initialize() {
	int res = Game::Initialize();
	if (res != 0) {
		return res;
	}

	FileSystem::ListDirectory(m_resourceFileBrowser, m_rm->GetAssetPath(), 1);

	//Initialize a blueprint for rendering/vizualizing lightsources (an actual mesh, not the light itself)
	Blueprint* light_bp = m_rm->GetBlueprint(m_lightsourceBlueprintName);
	if (!light_bp) {
		light_bp = m_rm->CreateBlueprint(m_lightsourceBlueprintName);
		Mesh* light_mesh = m_rm->GetMesh("sphere.obj");
		if (!light_mesh) {
			//TODO: Create mesh from code. (Mesh->makeSphere())
			return -1;
		}
		Material* light_material = m_rm->GetMaterial(m_lightsourceMaterialName);
		if (!light_material) {
			//TODO: Create material from code
			return -1;
		}

		light_bp->mesh = (light_mesh);
		light_bp->materials.push_back(light_material);
	}

	return 0;
}

void Editor::SubmitObjectsForRendering() {
	Game::SubmitObjectsForRendering();

	if (m_selectedResourceType == ResourceType::Light) {
		Blueprint* light_bp = m_rm->GetBlueprint(m_lightsourceBlueprintName);
		if (light_bp) {
			for (auto& e : m_selectedResources) {
				Transform t;
				t.pos = m_scene.m_lights[(int)e].m_position_center;
				m_renderer->Submit({ light_bp, t, (int)RenderFlag::Dont_Cast_Shadows }, m_scene.m_cameras[0]);
			}
		}
	}
}

void Editor::RegisterEditedResource(void* res, ResourceType type) {
	m_unSavedResources.insert({ res, type });
}

void Editor::SaveUnsavedFiles() {
	std::string name;
	for (auto& e : m_unSavedResources) {
		switch (e.second) {
		case ResourceType::Blueprint:
		{
			Blueprint* resource = static_cast<Blueprint*>(e.first);
			name = m_rm->GetBlueprintName(resource);
			if (!m_rm->SaveBlueprintToFile(resource, name)) {
				//TODO: Log Error
			}
		}
		break;
		case ResourceType::Material:
		{
			Material* resource = static_cast<Material*>(e.first);
			name = m_rm->GetMaterialName(resource);
			resource->SaveToFile((m_rm->GetAssetPath() + MATERIAL_FOLDER_NAME + name).c_str(), *m_rm);
			break;
		}
		default:
		{
			//TODO: Log Error
		}
		break;
		}
	}

	m_unSavedResources.clear();
}

bool Editor::ClearSelectedResources() {
	m_selectedResources.clear();
	m_selectedSceneObjectBlueprints.clear();
	m_selectedResourceType = ResourceType::None;
	m_selectedFileNames.clear();
	return true;
}

bool Editor::IsResourceSelected(void* res, ResourceType type) {
	if (m_selectedResourceType == type) {
		return Contains<std::vector, void*>(m_selectedResources, res);
	}
	return false;
}

bool Editor::SetSelectedResource(void* res, ResourceType type) {
	ClearSelectedResources();
	m_selectedResourceType = type;
	m_selectedResources.push_back(res);
	if (type == ResourceType::Object) {
		m_selectedSceneObjectBlueprints.insert(static_cast<Object*>(res)->blueprint);
	}
	if (m_lastClickedFileName != "") {
		m_selectedFileNames.push_back(m_lastClickedFileName);
		m_lastClickedFileName = "";
	}
	return true;
}

bool Editor::AddSelectedResource(void* res, ResourceType type) {
	if (m_selectedResourceType == type) {
		if (!Contains<std::vector, void*>(m_selectedResources, res)) {
			m_selectedResources.push_back(res);
			if (type == ResourceType::Object) {
				m_selectedSceneObjectBlueprints.insert(static_cast<Object*>(res)->blueprint);
			}
			if (m_lastClickedFileName != "") {
				m_selectedFileNames.push_back(m_lastClickedFileName);
				m_lastClickedFileName = "";
			}
			return true;
		}
		return false;
	} else {
		return SetSelectedResource(res, type);
	}
}

bool Editor::AddRemoveSelectedResource(void* res, ResourceType type) {
	if (m_selectedResourceType == type) {

		int i = 0;
		bool found = false;
		for (auto& e : m_selectedResources) {
			if (e == res) {
				found = true;
				break;
			}
			i++;
		}
		if (found) {
			m_selectedResources.erase(m_selectedResources.begin() + i);
			if (i < m_selectedFileNames.size()) {
				m_selectedFileNames.erase(m_selectedFileNames.begin() + i);
			} else {
				m_selectedFileNames.clear();
			}

			if (m_selectedResources.empty()) {
				ClearSelectedResources();
			}
		} else {
			m_selectedResources.push_back(res);
			if (type == ResourceType::Object) {
				m_selectedSceneObjectBlueprints.insert(static_cast<Object*>(res)->blueprint);
			}
			if (m_lastClickedFileName != "") {
				m_selectedFileNames.push_back(m_lastClickedFileName);
				m_lastClickedFileName = "";
			}
		}
		return true;
	} else {
		return SetSelectedResource(res, type);
	}
}

void Editor::RenderMenuBar() {
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
				m_scene.NewScene();
			}

			if (ImGui::MenuItem("Save Scene", "", nullptr, m_scene.m_currentSceneName != "")) {
				m_scene.SaveScene(false, m_rm);
			}

			if (ImGui::MenuItem("Save Scene as New", "")) {
				m_scene.SaveScene(true, m_rm);
				m_rm->RefreshFileSystemResourceLists();
			}

			if (ImGui::BeginMenu("Load Scene")) {
				std::filesystem::path clickedItem = DrawRecursiveDirectoryList(m_rm->m_foundScenes, m_scene.m_currentSceneName, true);
				if (clickedItem != "") {
					size_t len1 = m_rm->m_foundScenes.path.string().length();
					size_t len2 = clickedItem.string().length() - len1;
					std::string s = clickedItem.string().substr(len1, len2);

					ClearSelectedResources();
					bool b = m_scene.LoadScene(s, m_rm, m_renderAPI, m_windows[0]->GetDimensions());
				}
				ImGui::EndMenu();
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

void Editor::RenderSceneWindow() {
	ImGui::SetNextWindowSize(ImVec2(200, 500), ImGuiCond_Once);
	ImGui::SetNextWindowPos(ImVec2(10, 20), ImGuiCond_Once);

	if (ImGui::Begin("Scene", NULL, ImGuiWindowFlags_MenuBar)) {
		RenderSceneWindow_AddMenu();
		//=====Top Part=====
		ImGui::BeginChild("Scene Object List", ImVec2(0, ImGui::GetWindowHeight() - 120), true);

		int i = 0;
		for (auto& e : m_scene.m_lights) {
			if (ImGui::Selectable(("light#" + std::to_string(i)).c_str(), IsResourceSelected((void*)i, ResourceType::Light))) {
				//m_selectedObjects.push_back(i);
			}

			if (ImGui::IsItemClicked(0) && ImGui::IsItemHovered()) {
				if (!m_selectedResources.empty() && m_globalWindowInput->IsKeyDown(WindowInput::KEY_CODE_SHIFT)) {
					if (m_selectedResourceType == ResourceType::Light) {
						int min = (int)(m_selectedResources.front());
						int max = i;
						if (min > max) {
							std::swap(min, max);
						}

						ClearSelectedResources();
						for (int a = min; a <= max; a++) {
							AddSelectedResource((void*)a, ResourceType::Light);
						}
					} else {
						SetSelectedResource((void*)i, ResourceType::Light);
					}
				} else if (m_globalWindowInput->IsKeyDown(WindowInput::KEY_CODE_CTRL)) {
					AddRemoveSelectedResource((void*)i, ResourceType::Light);
				} else {
					SetSelectedResource((void*)i, ResourceType::Light);
				}
			}
			i++;
		}
		ImGui::Separator();
		i = 0;
		for (auto& e : m_scene.m_objects) {
			if (ImGui::Selectable(("obj#" + std::to_string(i) + " : " + m_rm->GetBlueprintName(e->blueprint)).c_str(), IsResourceSelected(e, ResourceType::Object))) {
				//m_selectedObjects.push_back(i);
			}

			if (ImGui::IsItemClicked(0) && ImGui::IsItemHovered()) {
				if (!m_selectedResources.empty() && m_globalWindowInput->IsKeyDown(WindowInput::KEY_CODE_SHIFT)) {
					if (m_selectedResourceType == ResourceType::Object) {
						int min = std::distance(m_scene.m_objects.begin(), std::find(m_scene.m_objects.begin(), m_scene.m_objects.end(), static_cast<Object*>(m_selectedResources.front())));
						int max = i;
						if (min > max) {
							std::swap(min, max);
						}

						ClearSelectedResources();
						for (int a = min; a <= max; a++) {
							AddSelectedResource(m_scene.m_objects.at(a), ResourceType::Object);
						}
					} else {
						SetSelectedResource(e, ResourceType::Object);
					}
				} else if (m_globalWindowInput->IsKeyDown(WindowInput::KEY_CODE_CTRL)) {
					AddRemoveSelectedResource(e, ResourceType::Object);
				} else {
					SetSelectedResource(e, ResourceType::Object);
				}
			}
			i++;
		}
		ImGui::EndChild();
		//=====Bottom Part=====
		//ImGui::SetNextWindowPos(ImVec2(0, 500));
		ImGui::BeginChild("Actions", ImVec2(0, 0), true);

		size_t nSelected = (int)m_selectedResources.size();

		if (nSelected > 0) {
			if (m_selectedResourceType == ResourceType::Object) {
				size_t numberOfObjects = (int)m_scene.m_objects.size();
				if (ImGui::Button("Copy")) {
					for (auto i : m_selectedResources) {
						Object* obj = new Object();
						memcpy(obj, i, sizeof(Object));
						m_scene.m_objects.push_back(obj);
					}
					m_selectedResources.clear();
					for (size_t i = numberOfObjects; i < numberOfObjects + nSelected; i++) {
						AddSelectedResource(m_scene.m_objects.at(i), ResourceType::Object);
					}
				}
				ImGui::SameLine();

				if (ImGui::Button("Delete")) {
					for (auto i : m_selectedResources) {
						auto e = std::find(m_scene.m_objects.begin(), m_scene.m_objects.end(), i);
						m_scene.m_objects.erase(e);
						delete i;
					}

					ClearSelectedResources();
				}

				if (ImGui::Button("Select All Identical")) {
					auto bplist = m_selectedSceneObjectBlueprints;
					ClearSelectedResources();
					for (auto i : m_scene.m_objects) {
						if (bplist.count(i->blueprint) > 0) {
							AddSelectedResource(i, ResourceType::Object);
						}
					}
				}
			} else if (m_selectedResourceType == ResourceType::Light) {
				size_t numberOfLights = (int)m_scene.m_lights.size();

				if (ImGui::Button("Copy")) {
					for (auto i : m_selectedResources) {
						LightSource ls = m_scene.m_lights[(int)i];
						m_scene.m_lights.push_back(ls);
					}
					m_selectedResources.clear();
					for (size_t i = numberOfLights; i < numberOfLights + nSelected; i++) {
						AddSelectedResource((void*)i, ResourceType::Light);
					}
				}
				ImGui::SameLine();

				if (ImGui::Button("Delete")) {
					std::sort(m_selectedResources.begin(), m_selectedResources.end());
					std::reverse(m_selectedResources.begin(), m_selectedResources.end());
					for (auto i : m_selectedResources) {
						m_scene.m_lights.erase(m_scene.m_lights.begin() + (int)i);
					}

					ClearSelectedResources();
				}
			}
		}
		ImGui::EndChild();

	}
	ImGui::End();
}

void Editor::RenderSceneWindow_AddMenu() {
	// ===========Menu==============
	if (ImGui::BeginMenuBar()) {
		if (ImGui::BeginMenu("Add new")) {
			if (ImGui::BeginMenu("Light Source")) {
				if (ImGui::MenuItem("Point Light", "")) {
					LightSource ls;
					ls.m_color = Float3(1, 1, 1);
					ls.m_reachRadius = 500;
					m_scene.m_lights.push_back(ls);
					SetSelectedResource((void*)(m_scene.m_lights.size() - 1), ResourceType::Light);
				}
				ImGui::EndMenu();

			}
			if (ImGui::BeginMenu("Object")) {
				ImGui::EndMenu();

			}
			//ShowExampleMenuFile();
			ImGui::EndMenu();
		}
		ImGui::EndMenuBar();
	}
}

void Editor::RenderResourceWindow() {
	ImGui::SetNextWindowSize(ImVec2(1900, 200), ImGuiCond_Once);
	ImGui::SetNextWindowPos(ImVec2(0, 1080 - 250), ImGuiCond_Once);

	//Helperfunction used to draw clickable file/folder icons with text
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
				//Draw the name of the currently active directory and the hovered file.
				ImGui::Text(("Path: " + m_resourceFileBrowser.path.string() + " : " + m_hoveredFile).c_str());
				//Draw the name of the currently selected files, if any
				std::string selectedFileText = "";
				if (!m_selectedFileNames.empty()) {
					for (size_t i = 0; i + 1 < m_selectedFileNames.size(); i++) {
						selectedFileText += m_selectedFileNames[i] + ", ";
					}
					selectedFileText += m_selectedFileNames.back();
				}
				ImGui::Text(("File(s): " + selectedFileText).c_str());

				ImGuiStyle& style = ImGui::GetStyle();
				float window_visible_x2 = ImGui::GetWindowPos().x + ImGui::GetWindowContentRegionMax().x;

				//Size of each icon with its text
				ImVec2 imgSize(64, 94);
				int nEntries = m_resourceFileBrowser.files.size() + m_resourceFileBrowser.directories.size();
				std::filesystem::path assetFolderPath = m_rm->GetAssetPath();

				int i = 0;
				/*Render the button to move up one folder from the current folder, if the root (assetFolderPath) is not reached*/
				if (m_resourceFileBrowser.path.string() != assetFolderPath) {
					nEntries++;
					ImGui::PushID(i);
					//Draw the parent-folder icon
					bool clicked = drawIcon(SelectFileIcon(""), imgSize, "..", false);
					ImGui::PopID();
					//Decide if there is room for another file to the left of this one. If there is, call ImGui::SameLine()
					float last_button_x2 = ImGui::GetItemRectMax().x;
					float next_button_x2 = last_button_x2 + style.ItemSpacing.x + imgSize.x; // Expected position if next button was on same line
					if ((i + 1) < nEntries && next_button_x2 < window_visible_x2)
						ImGui::SameLine();

					if (clicked) {
						//User clicked on the parrent directory. Update the filebrowser.
						std::filesystem::path parrent = m_resourceFileBrowser.path.parent_path().parent_path();
						FileSystem::ListDirectory(m_resourceFileBrowser, parrent.string() + "/", 1);
					}

					i++;
				}

				/*Render buttons for all found subfolders in the current folder*/
				for (auto& e : m_resourceFileBrowser.directories) {
					ImGui::PushID(i);
					//Draw the folder icon
					bool clicked = drawIcon(SelectFileIcon(e.path), imgSize, e.path.stem().string(), false);
					ImGui::PopID();
					//Decide if there is room for another file to the left of this one. If there is, call ImGui::SameLine()
					float last_button_x2 = ImGui::GetItemRectMax().x;
					float next_button_x2 = last_button_x2 + style.ItemSpacing.x + imgSize.x; // Expected position if next button was on same line
					if ((i + 1) < nEntries && next_button_x2 < window_visible_x2)
						ImGui::SameLine();

					if (clicked) {
						//User clicked a directory. Update the filebrowser.
						FileSystem::ListDirectory(m_resourceFileBrowser, e.path.string() + "/", 1);
					}

					i++;
				}

				/*Render buttons for all found files in the current folder*/
				for (auto& e : m_resourceFileBrowser.files) {
					ImGui::PushID(i);
					std::string fName = e.path.filename().string();
					//Draw the file icon
					bool clicked = drawIcon(SelectFileIcon(e.path), imgSize, fName, Contains<std::vector, std::string>(m_selectedFileNames, fName));
					ImGui::PopID();
					//Decide if there is room for another file to the left of this one. If there is, call ImGui::SameLine()
					float last_button_x2 = ImGui::GetItemRectMax().x;
					float next_button_x2 = last_button_x2 + style.ItemSpacing.x + imgSize.x; // Expected position if next button was on same line
					if ((i + 1) < nEntries && next_button_x2 < window_visible_x2)
						ImGui::SameLine();
					if (clicked) {
						//User clicked a file. Handle it.
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

void Editor::RenderDebugWindow() {
	ImGui::SetNextWindowSize(ImVec2(400, 250), ImGuiCond_Once);
	//ImGui::SetNextWindowPos(ImVec2(1920 - 420, 20), ImGuiCond_Once);
	if (ImGui::Begin("Debug", NULL, ImGuiWindowFlags_None)) {

		if (ImGui::BeginTabBar("debugTabar")) {
			if (ImGui::BeginTabItem("Setting")) {
				static float lightReach = 100;
				if (ImGui::DragFloat("Light Reach", &lightReach, 1, 1, 10000)) {
					for (auto& e : m_scene.m_lights) {
						e.m_reachRadius = lightReach;
					}
				}

				if (ImGui::Button("Recompile Shader")) {
					m_rm->RecompileShaders();
				}
				ImGui::EndTabItem();
			}

			if (ImGui::BeginTabItem("Unsaved Files")) {
				if (m_unSavedResources.empty()) {
					ImGui::Text("No changed Files.");
				} else {
					ImGui::BeginChild("File List", ImVec2(0, ImGui::GetWindowHeight() - 120), true);
					for (auto& e : m_unSavedResources) {
						std::string name = "";
						std::string type = "";
						switch (e.second) {
						case ResourceType::Blueprint:
							name = m_rm->GetBlueprintName(static_cast<Blueprint*>(e.first));
							type = "Blueprint";
							break;
						case ResourceType::Material:
							name = m_rm->GetMaterialName(static_cast<Material*>(e.first));
							type = "Material";
							break;
						default:
							name = "unkown";
							type = "unkown";
							break;
						}

						if (ImGui::Selectable((type + ": " + name).c_str(), false)) {

						}
					}
					ImGui::EndChild();
					ImGui::BeginChild("Save Buttons", ImVec2(0, 0), true);
					if (ImGui::Button("Save All")) {
						SaveUnsavedFiles();
					}
					ImGui::EndChild();
				}

				ImGui::EndTabItem();
			}
		}

		ImGui::EndTabBar();
	}

	ImGui::End();
}

void Editor::RenderPropertiesWindow() {
	ImGui::SetNextWindowSize(ImVec2(400, 500), ImGuiCond_Once);
	ImGui::SetNextWindowPos(ImVec2(1920 - 420, 20), ImGuiCond_Once);

	if (ImGui::Begin("Properties", NULL, ImGuiWindowFlags_None)) {
		switch (m_selectedResourceType) {
		case ResourceType::None:
			break;
		case ResourceType::Object:
			RenderPropertyWindowSceneObject();
			break;
		case ResourceType::Light:
			RenderPropertyWindowLights();
			break;
		case ResourceType::Blueprint:
			RenderPropertyWindowBlueprint();
			break;
		case ResourceType::Material:
			RenderPropertyWindowMaterial();
			break;
		case ResourceType::Other:
			RenderPropertyUnimplementedResourceType();
			break;
		default:
			RenderPropertyUnimplementedResourceType();
			break;
		}

	}
	ImGui::End();
}

void Editor::RenderPropertyWindowSceneObject() {
	size_t nSelected = m_selectedResources.size();
	Object* firstSelectedObject = static_cast<Object*>(m_selectedResources.front());

	if (nSelected == 1) {

		std::string name = "#Object Name#";// +std::to_string((int)m_selectedResources.front());
		ImGui::Text(("Selected: " + name).c_str());
	} else {
		ImGui::Text(("Selected: " + std::to_string(nSelected) + " Objects").c_str());
	}
	ImGui::Separator();

	std::string name = "";
	if (m_selectedSceneObjectBlueprints.size() == 1) {
		name = m_rm->GetBlueprintName(firstSelectedObject->blueprint);
		if (ImGui::Button("->")) {
			Blueprint* bp = m_rm->GetBlueprint(name);
			if (bp) {
				SetSelectedResource(bp, ResourceType::Blueprint);
			}
		}
		name = name.substr(0, name.find_last_of("."));
		if (ImGui::IsItemHovered()) {
			ImGui::SetTooltip("Go to Item");
		}
		ImGui::SameLine();
		// slider will be 65% of the window width (this is the default)
		ImGui::PushItemWidth(ImGui::GetWindowWidth() - 200);
		if (ImGui::BeginCombo("Blueprint Select", name.c_str())) {
			std::filesystem::path clickedItem = DrawRecursiveDirectoryList(m_rm->m_foundBluePrints, name);
			if (clickedItem != "") {
				size_t len1 = m_rm->m_foundBluePrints.path.string().length();
				size_t len2 = clickedItem.string().length() - len1;

				std::string s = clickedItem.string().substr(len1, len2);
				Blueprint* bp = m_rm->GetBlueprint(s);
				m_selectedSceneObjectBlueprints.clear();
				m_selectedSceneObjectBlueprints.insert(bp);
				for (auto object : m_selectedResources) {
					static_cast<Object*>(object)->blueprint = bp;
				}
			}
			ImGui::EndCombo();
		}
	}

	ImGui::Separator();
	if (ImGui::CollapsingHeader("Transform")) {
		if (m_selectedResources.size() == 1) {
			ImGui::DragFloat3("Pos", (float*)& firstSelectedObject->transform.pos, 0.1, -500, 500);
			ImGui::DragFloat3("Rot", (float*)& firstSelectedObject->transform.rotation, 0.1, -100, 100);
			ImGui::DragFloat3("Scale", (float*)& firstSelectedObject->transform.scale, 0.1, -100, 100);
		}
		if (ImGui::Button("Zero Rotation")) {
			for (auto i : m_selectedResources) {
				static_cast<Object*>(i)->transform.rotation = { 0,0,0 };
			}
		}

		if (ImGui::Button("Random X-Rot")) {
			for (auto i : m_selectedResources) {
				static_cast<Object*>(i)->transform.rotation.x = (rand() / (float)RAND_MAX) * 2 * 3.15;
			}
		}

		ImGui::SameLine();

		if (ImGui::Button("Random Y-Rot")) {
			for (auto i : m_selectedResources) {
				static_cast<Object*>(i)->transform.rotation.y = (rand() / (float)RAND_MAX) * 2 * 3.15;
			}
		}

		ImGui::SameLine();

		if (ImGui::Button("Random Z-Rot")) {
			for (auto i : m_selectedResources) {
				static_cast<Object*>(i)->transform.rotation.z = (rand() / (float)RAND_MAX) * 2 * 3.15;
			}
		}

		/////////////

		if (ImGui::Button("Move to origin")) {
			for (auto i : m_selectedResources) {
				static_cast<Object*>(i)->transform.pos = { 0,0,0 };
			}
		}

		ImGui::DragFloat("Spread", &m_maxRandomPos, 1, 0, 10000);

		if (ImGui::Button("Random X-Pos")) {
			for (auto i : m_selectedResources) {
				static_cast<Object*>(i)->transform.pos.x = (rand() / (float)RAND_MAX) * m_maxRandomPos - m_maxRandomPos / 2;
			}
		}

		ImGui::SameLine();

		if (ImGui::Button("Random Y-Pos")) {
			for (auto i : m_selectedResources) {
				static_cast<Object*>(i)->transform.pos.y = (rand() / (float)RAND_MAX) * m_maxRandomPos - m_maxRandomPos / 2;
			}
		}

		ImGui::SameLine();

		if (ImGui::Button("Random Z-Pos")) {
			for (auto i : m_selectedResources) {
				static_cast<Object*>(i)->transform.pos.z = (rand() / (float)RAND_MAX) * m_maxRandomPos - m_maxRandomPos / 2;
			}
		}
	}


	ImGui::Separator();
}

void Editor::RenderPropertyWindowLights() {
	size_t nSelected = m_selectedResources.size();
	LightSource& fistSelected = m_scene.m_lights[(int)m_selectedResources.front()];
	if (nSelected == 1) {

		std::string name = "#Light Name#";// +std::to_string((int)m_selectedResources.front());
		ImGui::Text(("Selected: " + name).c_str());
	} else {
		ImGui::Text(("Selected: " + std::to_string(nSelected) + " Lights").c_str());
	}

	if (nSelected == 1) {
		ImGui::DragFloat3("Position", (float*)& fistSelected.m_position_center, 0.1, -500, 500);
	}

	if (ImGui::DragFloat3("Color", (float*)& fistSelected.m_color, 0.1, 0, 1)) {
		for (size_t i = 1; i < nSelected; i++) {
			m_scene.m_lights[(int)m_selectedResources[i]].m_color = m_scene.m_lights[(int)m_selectedResources.front()].m_color;
		}
	}
	if (ImGui::DragFloat("Light Reach", (float*)& fistSelected.m_reachRadius, 100, 0, 10000)) {
		for (size_t i = 1; i < nSelected; i++) {
			m_scene.m_lights[(int)m_selectedResources[i]].m_reachRadius = m_scene.m_lights[(int)m_selectedResources.front()].m_reachRadius;
		}
	}
	if (ImGui::Checkbox("Enable", &fistSelected.m_enabled)) {
		for (size_t i = 1; i < nSelected; i++) {
			m_scene.m_lights[(int)m_selectedResources[i]].m_enabled = m_scene.m_lights[(int)m_selectedResources.front()].m_enabled;
		}
	}

	ImGui::Separator();
}

void Editor::RenderPropertyWindowBlueprint() {
	Blueprint* bp = nullptr;
	std::string bp_name = "";

	size_t nSelected = m_selectedResources.size();
	if (nSelected == 1) {
		bp = static_cast<Blueprint*>(m_selectedResources.front());
		bp_name = m_rm->GetBlueprintName(bp);
		ImGui::Text(("Selected Blueprint: " + bp_name).c_str());
	} else {
		ImGui::Text(("Selected: " + std::to_string(nSelected) + " Blueprints").c_str());
		return;
	}

	std::string name = "";
	name = m_rm->GetMeshName(bp->mesh);
	name = name.substr(0, name.find_last_of("."));
	ImGui::PushItemWidth(ImGui::GetWindowWidth() - 200);
	if (ImGui::BeginCombo("Mesh Select", name.c_str())) {
		std::filesystem::path clickedItem = DrawRecursiveDirectoryList(m_rm->m_foundMeshes, name);
		if (clickedItem != "") {
			size_t len1 = m_rm->m_foundMeshes.path.string().length();
			size_t len2 = clickedItem.string().length() - len1;

			std::string s = clickedItem.string().substr(len1, len2);

			Mesh* mesh = m_rm->GetMesh(s);
			if (mesh) {
				RegisterEditedResource(bp, ResourceType::Blueprint);
				bp->hasChanged = true;
				bp->mesh = mesh;
				int nNeededMaterials = bp->mesh->GetNumberOfSubMeshes();
				for (size_t i = bp->materials.size(); i < nNeededMaterials; i++) {
					bp->materials.push_back(bp->materials.back());
				}

				if (bp->materials.size() > nNeededMaterials) {
					bp->materials.erase(bp->materials.begin() + nNeededMaterials, bp->materials.end());
				}
			} else {
				//TODO: Log Error
			}

		}
		ImGui::EndCombo();
	}
	ImGui::Separator();

	int nMaterials = bp->materials.size();
	for (int i = 0; i < nMaterials; i++) {
		ImGui::PushID(i);

		name = m_rm->GetMaterialName(bp->materials[i]);
		if (ImGui::Button("->")) {
			Material* resource = m_rm->GetMaterial(name);
			if (resource) {
				SetSelectedResource(resource, ResourceType::Material);
			}
		}
		if (ImGui::IsItemHovered()) {
			ImGui::SetTooltip("Go to Item");
		}
		ImGui::SameLine();

		name = name.substr(0, name.find_last_of("."));
		ImGui::PushItemWidth(ImGui::GetWindowWidth() - 200);
		if (ImGui::BeginCombo(("Select Material #" + std::to_string(i)).c_str(), name.c_str())) {
			std::filesystem::path clickedItem = DrawRecursiveDirectoryList(m_rm->m_foundMaterials, name);
			if (clickedItem != "") {
				size_t len1 = m_rm->m_foundMaterials.path.string().length();
				size_t len2 = clickedItem.string().length() - len1;
				std::string s = clickedItem.string().substr(len1, len2);
				Material* material = m_rm->GetMaterial(s);
				if (material) {
					bp->hasChanged = true;
					RegisterEditedResource(bp, ResourceType::Blueprint);
					bp->materials[i] = material;
				} else {
					//TODO: Log Error Message
				}
			}
			ImGui::EndCombo();
		}

		ImGui::PopID();
	}


}

void Editor::RenderPropertyWindowMaterial() {
	size_t nSelected = m_selectedResources.size();
	Material* material = nullptr;
	std::string materialName = "";
	if (nSelected == 1) {
		material = (Material*)m_selectedResources.front();
		materialName = m_rm->GetMaterialName(material);
		ImGui::Text(("Selected Material: " + materialName).c_str());
	} else {
		ImGui::Text(("Selected: " + std::to_string(nSelected) + " Materials").c_str());
		return;
	}
	ImGui::Separator();
	std::string shaderProgramName = m_rm->GetShaderProgramName(material->GetShaderProgram());
	//Edit Shader Program
	ImGui::PushItemWidth(ImGui::GetWindowWidth() - 200);
	if (ImGui::BeginCombo("Shader Program##sp", shaderProgramName.c_str())) {
		std::filesystem::path clickedItem = DrawRecursiveDirectoryList(m_rm->m_foundShaderPrograms, shaderProgramName);
		if (clickedItem != "") {
			size_t len1 = m_rm->m_foundShaderPrograms.path.string().length();
			size_t len2 = clickedItem.string().length() - len1;
			std::string s = clickedItem.string().substr(len1, len2);

			ShaderProgramHandle sph = m_rm->GetShaderProgramHandle(s);
			if (sph >= 0) {
				RegisterEditedResource(material, ResourceType::Material);
				material->SetShaderProgram(sph);
			} else {
				//TODO: Log Error
			}
		}
		ImGui::EndCombo();
	}
	ImGui::Separator();
	std::string colorTextureName = m_rm->GetTextureName(material->m_materialData.pbrData.color);
	std::string normalTextureName = m_rm->GetTextureName(material->m_materialData.pbrData.normal);
	std::string metalTextureName = m_rm->GetTextureName(material->m_materialData.pbrData.metalness);
	std::string roughnessTextureName = m_rm->GetTextureName(material->m_materialData.pbrData.roughness);

	//Edit Color Texture
	ImGui::PushItemWidth(ImGui::GetWindowWidth() - 200);
	if (ImGui::BeginCombo("Color Texture##color", colorTextureName.c_str())) {
		std::filesystem::path clickedItem = DrawRecursiveDirectoryList(m_rm->m_foundTextures, colorTextureName);
		if (clickedItem != "") {
			size_t len1 = m_rm->m_foundTextures.path.string().length();
			size_t len2 = clickedItem.string().length() - len1;
			std::string s = clickedItem.string().substr(len1, len2);

			Texture* texture = m_rm->GetTexture(s);
			if (texture) {
				RegisterEditedResource(material, ResourceType::Material);
				material->m_materialData.pbrData.color = texture;
			} else {
				//TODO: Log Error
			}
		}
		ImGui::EndCombo();
	}
	//Edit Normal Texture
	ImGui::PushItemWidth(ImGui::GetWindowWidth() - 200);
	if (ImGui::BeginCombo("Normal Texture##norm", normalTextureName.c_str())) {
		std::filesystem::path clickedItem = DrawRecursiveDirectoryList(m_rm->m_foundTextures, normalTextureName);
		if (clickedItem != "") {
			size_t len1 = m_rm->m_foundTextures.path.string().length();
			size_t len2 = clickedItem.string().length() - len1;
			std::string s = clickedItem.string().substr(len1, len2);

			Texture* texture = m_rm->GetTexture(s);
			if (texture) {
				RegisterEditedResource(material, ResourceType::Material);
				material->m_materialData.pbrData.normal = texture;
			} else {
				//TODO: Log Error
			}
		}
		ImGui::EndCombo();
	}
	//Edit Metal Texture
	//ImGui::PushItemWidth(ImGui::GetWindowWidth() - 200);
	//if (ImGui::BeginCombo("Metal Texture##metal", metalTextureName.c_str())) {
	//	std::filesystem::path clickedItem = DrawRecursiveDirectoryList(m_rm->m_foundTextures, metalTextureName);
	//	if (clickedItem != "") {
	//		size_t len1 = m_rm->m_foundTextures.path.string().length();
	//		size_t len2 = clickedItem.string().length() - len1;
	//		std::string s = clickedItem.string().substr(len1, len2);
	//		Texture* texture = m_rm->GetTexture(s);
	//		if (texture) {
	//			RegisterEditedResource(material, ResourceType::Material);
	//			material->m_materialData.pbrData.metalness = texture;
	//		} else {
	//			//TODO: Log Error
	//		}
	//	}
	//	ImGui::EndCombo();
	//}
	//Edit Roughness Texture
	ImGui::PushItemWidth(ImGui::GetWindowWidth() - 200);
	if (ImGui::BeginCombo("RAOM Texture##rough", roughnessTextureName.c_str())) {
		std::filesystem::path clickedItem = DrawRecursiveDirectoryList(m_rm->m_foundTextures, roughnessTextureName);
		if (clickedItem != "") {
			size_t len1 = m_rm->m_foundTextures.path.string().length();
			size_t len2 = clickedItem.string().length() - len1;
			std::string s = clickedItem.string().substr(len1, len2);
			Texture* texture = m_rm->GetTexture(s);
			if (texture) {
				RegisterEditedResource(material, ResourceType::Material);
				material->m_materialData.pbrData.roughness = texture;
			} else {
				//TODO: Log Error
			}
		}
		ImGui::EndCombo();
	}

}

void Editor::RenderPropertyUnimplementedResourceType() {
	size_t nSelected = m_selectedResources.size();

	ImGui::Text(("Selected: " + std::to_string(nSelected) + " resources.").c_str());
	ImGui::Text("Property window have not been implemented for this Resource type yet.");
}

void Editor::HandleResourceClick(std::filesystem::path clickedResource) {
	m_lastClickedFileName = clickedResource.filename().string();
	std::string ext = clickedResource.extension().string();

	for (auto& c : ext) {
		c = std::tolower(c);
	}

	ResourceType resourceType = ResourceType::None;
	void* resource = nullptr;

	if (ext == ".bp") {
		std::string blueprintFolder = m_rm->GetAssetPath() + BLUEPRINT_FOLDER_NAME;
		std::string name = clickedResource.string().substr(blueprintFolder.length(), clickedResource.string().length());
		resource = m_rm->GetBlueprint(name);

		if (resource) {
			resourceType = ResourceType::Blueprint;
		} else {
			ClearSelectedResources();
		}
	} else if (ext == ".mat") {
		std::string materialFolder = m_rm->GetAssetPath() + MATERIAL_FOLDER_NAME;
		std::string name = clickedResource.string().substr(materialFolder.length(), clickedResource.string().length());
		resource = m_rm->GetMaterial(name);

		if (resource) {
			resourceType = ResourceType::Material;
		} else {
			ClearSelectedResources();
		}
	} else if (ext == ".scene") {
	} else if (ext == ".hlsl" || ext == ".hlsli") {
	} else if (ext == ".png" || ext == ".dds") {
	} else {
		ClearSelectedResources();
	}

	if (resourceType != ResourceType::None) {
		if (m_globalWindowInput->IsKeyDown(WindowInput::KEY_CODE_CTRL) || m_globalWindowInput->IsKeyDown(WindowInput::KEY_CODE_SHIFT)) {
			AddRemoveSelectedResource(resource, resourceType);
		} else {
			SetSelectedResource(resource, resourceType);
		}
	}
}

void* Editor::SelectFileIcon(std::filesystem::path clickedResource) {

	if (clickedResource == "") {
		if (!m_icon_up_folder_id) {
			m_icon_up_folder_id = m_windows[0]->PrepareTextureForGuiRendering(m_rm->GetTexture(m_icon_up_folder));
		}
		return m_icon_up_folder_id;
	} else {
		std::string ext = clickedResource.extension().string();
		for (auto& c : ext) {
			c = std::tolower(c);
		}

		if (ext == "") {
			if (!m_icon_down_folder_id) {
				m_icon_down_folder_id = m_windows[0]->PrepareTextureForGuiRendering(m_rm->GetTexture(m_icon_down_folder));
			}
			return m_icon_down_folder_id;
		} else if (ext == ".bp") {
			if (!m_icon_blueprint_id) {
				m_icon_down_folder_id = m_windows[0]->PrepareTextureForGuiRendering(m_rm->GetTexture(m_icon_blueprint));
			}
			return m_icon_down_folder_id;
		} else if (ext == ".scene") {
			if (!m_icon_scene_id) {
				m_icon_scene_id = m_windows[0]->PrepareTextureForGuiRendering(m_rm->GetTexture(m_icon_scene));
			}
			return m_icon_scene_id;
		} else if (ext == ".mat") {
			if (!m_icon_material_id) {
				m_icon_material_id = m_windows[0]->PrepareTextureForGuiRendering(m_rm->GetTexture(m_icon_material));
			}
			return m_icon_material_id;
		} else if (ext == ".sp" || ext == ".hlsl" || ext == ".hlsli") {
			if (!m_icon_shader_id) {
				m_icon_shader_id = m_windows[0]->PrepareTextureForGuiRendering(m_rm->GetTexture(m_icon_shader));
			}
			return m_icon_shader_id;
		} else if (ext == ".obj") {
			if (!m_icon_object_id) {
				m_icon_object_id = m_windows[0]->PrepareTextureForGuiRendering(m_rm->GetTexture(m_icon_object));
			}
			return m_icon_object_id;
		} else if (ext == ".png" || ext == ".dds") {

			std::string textureFolder = m_rm->GetAssetPath() + TEXTURE_FODLER_NAME;
			std::string clickedPath = clickedResource.string();
			std::string textureName = clickedPath.substr(textureFolder.length(), clickedPath.length());

			Texture* tex = m_rm->GetTexture(textureName);
			if (tex && tex->IsLoaded()) {
				return m_windows[0]->PrepareTextureForGuiRendering(tex);
			} else {
				return SelectFileIcon("x.x");
			}
		}

		if (!m_icon_unknown_id) {
			m_icon_unknown_id = m_windows[0]->PrepareTextureForGuiRendering(m_rm->GetTexture(m_icon_unknown));
		}
		return m_icon_unknown_id;
	}
}

std::filesystem::path Editor::DrawRecursiveDirectoryList(const FileSystem::Directory& path, const std::string& selectedItem, const bool isMenuBar, unsigned int currDepth) {
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
				clicked = DrawRecursiveDirectoryList(e, selectedItem, isMenuBar, currDepth + 1);
				ImGui::EndMenu();
			}
		} else {
			if (ImGui::CollapsingHeader((depthStr + e.path.filename().string()).c_str())) {
				clicked = DrawRecursiveDirectoryList(e, selectedItem, isMenuBar, currDepth + 1);
				if (clicked != "") {
					return clicked;
				}
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
			if (clicked != "") {
				return clicked;
			}
		}
	}

	return clicked;
}
