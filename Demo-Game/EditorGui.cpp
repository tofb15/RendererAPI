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
#include <iterator> 

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

bool EditorGUI::ClearSelectedResources() {
	m_selectedResources.clear();
	m_selectedSceneObjectBlueprints.clear();
	m_selectedResourceType = ResourceTypes::None;
	m_selectedFileNames.clear();
	return true;
}

bool EditorGUI::IsResourceSelected(void* res, ResourceTypes type) {
	if (m_selectedResourceType == type) {
		return Contains<std::vector, void*>(m_selectedResources, res);
	}
	return false;
}

bool EditorGUI::SetSelectedResource(void* res, ResourceTypes type) {
	ClearSelectedResources();
	m_selectedResourceType = type;
	m_selectedResources.push_back(res);
	if (type == ResourceTypes::Object) {
		m_selectedSceneObjectBlueprints.insert(static_cast<Object*>(res)->blueprint);
	}
	if (m_lastClickedFileName != "") {
		m_selectedFileNames.push_back(m_lastClickedFileName);
		m_lastClickedFileName = "";
	}
	return true;
}

bool EditorGUI::AddSelectedResource(void* res, ResourceTypes type) {
	if (m_selectedResourceType == type) {
		if (!Contains<std::vector, void*>(m_selectedResources, res)) {
			m_selectedResources.push_back(res);
			if (type == ResourceTypes::Object) {
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

bool EditorGUI::AddRemoveSelectedResource(void* res, ResourceTypes type) {
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
			if (type == ResourceTypes::Object) {
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
		int i = 0;
		for (auto& e : *m_objects) {
			if (ImGui::Selectable(("obj#" + std::to_string(i) + " : " + m_resourceManager->GetBlueprintName(e->blueprint)).c_str(), IsResourceSelected(e, ResourceTypes::Object))) {
				//m_selectedObjects.push_back(i);
			}

			if (ImGui::IsItemClicked(0) && ImGui::IsItemHovered()) {
				if (!m_selectedResources.empty() && m_windowInput->IsKeyDown(WindowInput::KEY_CODE_SHIFT)) {
					int min = std::distance(m_objects->begin(), std::find(m_objects->begin(), m_objects->end(), static_cast<Object*>(m_selectedResources.front())));
					int max = i;
					if (min > max) {
						std::swap(min, max);
					}

					ClearSelectedResources();
					for (int a = min; a <= max; a++) {
						AddSelectedResource(m_objects->at(a), ResourceTypes::Object);
					}
				} else if (m_windowInput->IsKeyDown(WindowInput::KEY_CODE_CTRL)) {
					AddRemoveSelectedResource(e, ResourceTypes::Object);
				} else {
					SetSelectedResource(e, ResourceTypes::Object);
				}
			}
			i++;
		}
		ImGui::EndChild();
		//=====Bottom Part=====
		//ImGui::SetNextWindowPos(ImVec2(0, 500));
		ImGui::BeginChild("Actions", ImVec2(0, 0), true);

		size_t numberOfObjects = (int)m_objects->size();
		size_t nSelected = (int)m_selectedResources.size();

		if (nSelected > 0 && m_selectedResourceType == ResourceTypes::Object) {
			if (ImGui::Button("Copy")) {
				for (auto i : m_selectedResources) {
					Object* obj = new Object();
					memcpy(obj, i, sizeof(Object));
					m_objects->push_back(obj);
				}
				m_selectedResources.clear();
				for (size_t i = numberOfObjects; i < numberOfObjects + nSelected; i++) {
					AddSelectedResource(m_objects->at(i), ResourceTypes::Object);
				}
			}
			ImGui::SameLine();

			if (ImGui::Button("Delete")) {
				for (auto i : m_selectedResources) {
					auto e = std::find(m_objects->begin(), m_objects->end(), i);
					m_objects->erase(e);
					delete i;
				}

				ClearSelectedResources();
			}

			if (ImGui::Button("Select All Identical")) {
				auto bplist = m_selectedSceneObjectBlueprints;
				ClearSelectedResources();
				for (auto i : *m_objects) {
					if (bplist.count(i->blueprint) > 0) {
						AddSelectedResource(i, ResourceTypes::Object);
					}
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
				std::filesystem::path assetFolderPath = m_resourceManager->GetAssetPath();

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

void EditorGUI::RenderPropertiesWindow() {
	ImGui::SetNextWindowSize(ImVec2(400, 500), ImGuiCond_Once);
	ImGui::SetNextWindowPos(ImVec2(1920 - 420, 20), ImGuiCond_Once);

	if (ImGui::Begin("Properties", NULL, ImGuiWindowFlags_None)) {
		switch (m_selectedResourceType) {
		case ResourceTypes::None:
			break;
		case ResourceTypes::Scene:
			break;
		case ResourceTypes::Object:
			RenderPropertyWindowSceneObject();
			break;
		case ResourceTypes::Blueprint:
			RenderPropertyWindowBlueprint();
			break;
		case ResourceTypes::Material:
			RenderPropertyWindowMaterial();
			break;
		case ResourceTypes::Texture:
			break;
		case ResourceTypes::Shader:
			break;
		case ResourceTypes::ShaderProgram:
			break;
		case ResourceTypes::Other:
			break;
		case ResourceTypes::SIZE_KEEP_THIS_LAST:
			break;
		default:
			break;
		}

	}
	ImGui::End();
}

void EditorGUI::RenderPropertyWindowSceneObject() {
	size_t nSelected = m_selectedResources.size();
	if (nSelected == 1) {

		std::string name = "#Object Name#";// +std::to_string((int)m_selectedResources.front());
		ImGui::Text(("Selected: " + name).c_str());
	} else {
		ImGui::Text(("Selected: " + std::to_string(nSelected) + " Objects").c_str());
	}
	ImGui::Separator();

	ImGui::Separator();
	if (ImGui::CollapsingHeader("Transform")) {
		if (m_selectedResources.size() == 1) {
			Object* firstSelectedObject = static_cast<Object*>(m_selectedResources.front());
			ImGui::DragFloat3("Pos", (float*)& firstSelectedObject->transform.pos, 0.1, -100, 100);
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

void EditorGUI::RenderPropertyWindowBlueprint() {
	Blueprint* bp = nullptr;
	std::string bp_name = "";

	size_t nSelected = m_selectedResources.size();
	if (nSelected == 1) {
		bp = static_cast<Blueprint*>(m_selectedResources.front());
		bp_name = m_resourceManager->GetBlueprintName(bp);
		ImGui::Text(("Selected Blueprint: " + bp_name).c_str());
	} else {
		ImGui::Text(("Selected: " + std::to_string(nSelected) + " Blueprints").c_str());
		return;
	}

	std::string name = "";
	name = m_resourceManager->GetMeshName(bp->mesh);
	name = name.substr(0, name.find_last_of("."));
	if (ImGui::BeginCombo("Mesh Select", name.c_str())) {
		std::filesystem::path clickedItem = DrawRecursiveDirectoryList(m_resourceManager->m_foundMeshes, name);
		if (clickedItem != "") {
			size_t len1 = m_resourceManager->m_foundMeshes.path.string().length();
			size_t len2 = clickedItem.string().length() - len1;

			std::string s = clickedItem.string().substr(len1, len2);

			bp->hasChanged = true;
			bp->mesh = m_resourceManager->GetMesh(s);
			int nNeededMaterials = bp->mesh->GetNumberOfSubMeshes();
			for (size_t i = bp->materials.size(); i < nNeededMaterials; i++) {
				bp->materials.push_back(bp->materials.back());
			}

			if (bp->materials.size() > nNeededMaterials) {
				bp->materials.erase(bp->materials.begin() + nNeededMaterials, bp->materials.end());
			}
		}
		ImGui::EndCombo();
	}
	ImGui::Separator();

	int nMaterials = bp->materials.size();
	for (int i = 0; i < nMaterials; i++) {
		ImGui::PushID(i);

		name = m_resourceManager->GetMaterialName(bp->materials[i]);
		name = name.substr(0, name.find_last_of("."));
		if (ImGui::BeginCombo(("Select Material #" + std::to_string(i)).c_str(), name.c_str())) {
			std::filesystem::path clickedItem = DrawRecursiveDirectoryList(m_resourceManager->m_foundMaterials, name);
			if (clickedItem != "") {
				size_t len1 = m_resourceManager->m_foundMaterials.path.string().length();
				size_t len2 = clickedItem.string().length() - len1;

				std::string s = clickedItem.string().substr(len1, len2);

				bp->hasChanged = true;
				bp->materials[i] = m_resourceManager->GetMaterial(s);
			}
			ImGui::EndCombo();
		}

		ImGui::PopID();
	}


}

void EditorGUI::RenderPropertyWindowMaterial() {
	size_t nSelected = m_selectedResources.size();
	Material* material = nullptr;
	std::string materialName = "";
	if (nSelected == 1) {
		material = (Material*)m_selectedResources.front();
		materialName = m_resourceManager->GetMaterialName(material);
		ImGui::Text(("Selected Material: " + materialName).c_str());
	} else {
		ImGui::Text(("Selected: " + std::to_string(nSelected) + " Materials").c_str());
		return;
	}
	ImGui::Separator();
	std::string shaderProgramName = m_resourceManager->GetShaderProgramName(material->GetShaderProgram());
	//Edit Shader Program
	if (ImGui::BeginCombo("Shader Program##sp", shaderProgramName.c_str())) {
		std::filesystem::path clickedItem = DrawRecursiveDirectoryList(m_resourceManager->m_foundShaderPrograms, shaderProgramName);
		if (clickedItem != "") {
			size_t len1 = m_resourceManager->m_foundShaderPrograms.path.string().length();
			size_t len2 = clickedItem.string().length() - len1;
			std::string s = clickedItem.string().substr(len1, len2);
			material->SetShaderProgram(m_resourceManager->GetShaderProgramHandle(s));
		}
		ImGui::EndCombo();
	}
	ImGui::Separator();
	std::string colorTextureName = m_resourceManager->GetTextureName(material->m_materialData.pbrData.color);
	std::string normalTextureName = m_resourceManager->GetTextureName(material->m_materialData.pbrData.normal);
	std::string metalTextureName = m_resourceManager->GetTextureName(material->m_materialData.pbrData.metalness);
	std::string roughnessTextureName = m_resourceManager->GetTextureName(material->m_materialData.pbrData.roughness);

	//Edit Color Texture
	if (ImGui::BeginCombo("Color Texture##color", colorTextureName.c_str())) {
		std::filesystem::path clickedItem = DrawRecursiveDirectoryList(m_resourceManager->m_foundTextures, colorTextureName);
		if (clickedItem != "") {
			size_t len1 = m_resourceManager->m_foundTextures.path.string().length();
			size_t len2 = clickedItem.string().length() - len1;
			std::string s = clickedItem.string().substr(len1, len2);
			material->m_materialData.pbrData.color = m_resourceManager->GetTexture(s);
		}
		ImGui::EndCombo();
	}
	//Edit Normal Texture
	if (ImGui::BeginCombo("Normal Texture##norm", normalTextureName.c_str())) {
		std::filesystem::path clickedItem = DrawRecursiveDirectoryList(m_resourceManager->m_foundTextures, normalTextureName);
		if (clickedItem != "") {
			size_t len1 = m_resourceManager->m_foundMaterials.path.string().length();
			size_t len2 = clickedItem.string().length() - len1;
			std::string s = clickedItem.string().substr(len1, len2);
			material->m_materialData.pbrData.normal = m_resourceManager->GetTexture(s);
		}
		ImGui::EndCombo();
	}
	//Edit Metal Texture
	if (ImGui::BeginCombo("Metal Texture##metal", metalTextureName.c_str())) {
		std::filesystem::path clickedItem = DrawRecursiveDirectoryList(m_resourceManager->m_foundTextures, metalTextureName);
		if (clickedItem != "") {
			size_t len1 = m_resourceManager->m_foundTextures.path.string().length();
			size_t len2 = clickedItem.string().length() - len1;
			std::string s = clickedItem.string().substr(len1, len2);
			material->m_materialData.pbrData.metalness = m_resourceManager->GetTexture(s);
		}
		ImGui::EndCombo();
	}
	//Edit Roughness Texture
	if (ImGui::BeginCombo("Roughness Texture##rough", roughnessTextureName.c_str())) {
		std::filesystem::path clickedItem = DrawRecursiveDirectoryList(m_resourceManager->m_foundTextures, roughnessTextureName);
		if (clickedItem != "") {
			size_t len1 = m_resourceManager->m_foundTextures.path.string().length();
			size_t len2 = clickedItem.string().length() - len1;
			std::string s = clickedItem.string().substr(len1, len2);
			material->m_materialData.pbrData.roughness = m_resourceManager->GetTexture(s);
		}
		ImGui::EndCombo();
	}

}

void EditorGUI::HandleResourceClick(std::filesystem::path clickedResource) {
	m_lastClickedFileName = clickedResource.filename().string();
	std::string ext = clickedResource.extension().string();

	for (auto& c : ext) {
		c = std::tolower(c);
	}

	ResourceTypes resourceType = ResourceTypes::None;
	void* resource = nullptr;

	if (ext == ".bp") {
		std::string blueprintFolder = m_resourceManager->GetAssetPath() + BLUEPRINT_FOLDER_NAME;
		std::string name = clickedResource.string().substr(blueprintFolder.length(), clickedResource.string().length());
		resource = m_resourceManager->GetBlueprint(name);

		if (resource) {
			resourceType = ResourceTypes::Blueprint;
		} else {
			ClearSelectedResources();
		}
	} else if (ext == ".mat") {
		std::string materialFolder = m_resourceManager->GetAssetPath() + MATERIAL_FOLDER_NAME;
		std::string name = clickedResource.string().substr(materialFolder.length(), clickedResource.string().length());
		resource = m_resourceManager->GetMaterial(name);

		if (resource) {
			resourceType = ResourceTypes::Material;
		} else {
			ClearSelectedResources();
		}
	} else if (ext == ".scene") {
	} else if (ext == ".hlsl" || ext == ".hlsli") {
	} else if (ext == ".png" || ext == ".dds") {
	} else {
		ClearSelectedResources();
	}

	if (resourceType != ResourceTypes::None) {
		if (m_windowInput->IsKeyDown(WindowInput::KEY_CODE_CTRL) || m_windowInput->IsKeyDown(WindowInput::KEY_CODE_SHIFT)) {
			AddRemoveSelectedResource(resource, resourceType);
		} else {
			SetSelectedResource(resource, resourceType);
		}
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
