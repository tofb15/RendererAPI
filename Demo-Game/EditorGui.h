#include "../D3D12Engine/RenderAPI.hpp"
#include "../D3D12Engine/Utills/FileSystem.h"

#include <vector>

class Object;
class WindowInput;
enum class FileTypes {
	Scene = 0,
	Blueprint,
	Material,
	Texture,
	Shader,
	ShaderProgram,
	///////////////////////
	SIZE_KEEP_THIS_LAST,
};

class EditorGUI : public GUI {
public:
	EditorGUI(std::vector<Object*>* sceneObjects, WindowInput* windowInput, ResourceManager* resourceManager, Window* w);
	~EditorGUI();

	// Inherited via GUI
	virtual void RenderGUI() override;
private:
	//Icons
	std::string m_icon_blueprint = "Icons/blueprint.png";
	std::string m_icon_material = "Icons/material.png";
	std::string m_icon_scene = "Icons/scene.png";
	std::string m_icon_shader = "Icons/shader.png";
	std::string m_icon_up_folder = "Icons/up_folder.png";
	std::string m_icon_down_folder = "Icons/down_folder.png";
	std::string m_icon_object = "Icons/object.png";
	std::string m_icon_unknown = "Icons/unknown.png";

	void* m_icon_blueprint_id;
	void* m_icon_material_id;
	void* m_icon_scene_id;
	void* m_icon_shader_id;
	void* m_icon_up_folder_id;
	void* m_icon_down_folder_id;
	void* m_icon_object_id;
	void* m_icon_unknown_id;


	bool m_showSceneWindow = false;
	bool m_showPropertiesWindow = true;
	bool m_showResourceWindow = true;

	std::vector<Object*>* m_objects = nullptr;
	ResourceManager* m_resourceManager = nullptr;
	const WindowInput* m_windowInput = nullptr;
	Window* m_window;

	std::vector<int> m_selectedSceneObjects;
	std::unordered_set<Blueprint*> m_selectedSceneObjectBlueprints;
	FileSystem::Directory m_resourceFileBrowser;
	std::string m_hoveredFile = "";
	std::string m_selectedFile = "";
	Blueprint* m_selectedBlueprint = nullptr;

	enum PropertyWindowState {
		None = 0,
		SceneObject,
		BlueprintEditor,
		MaterialEditor,
	};

	PropertyWindowState m_currentPropertyWindowState = SceneObject;
	float m_maxRandomPos = 100;

	////////////////////
	void RenderMenuBar();
	void RenderSceneWindow();
	void RenderResourceWindow();

	//PropertyWindow
	void RenderPropertiesWindow();
	void RenderPropertyWindowSceneObject();
	void RenderPropertyWindowBlueprint();

	//Resource Browser
	void HandleResourceClick(std::filesystem::path clickedResource);
	void* SelectFileIcon(std::filesystem::path clickedResource);

	////////////////////////////////////////

	/**
	Populates a dynamic ImGui container with files listed in a FileSystem::Directory.

	@param path, the directory from which to populate the ImGui Container.
	@param selectedItem, any file with this name will be visualized as selected.
	@param isMenuBar, set this to true if the ImGui container is a menubar, else set this to false.
	@param currDepth, used internally, set this to 0.

	@return the path to the file that was clicked by the user(if any).
		If no file was clicked this will return an empty path: ""
*/
	std::filesystem::path DrawRecursiveDirectoryList(const FileSystem::Directory& path, const std::string& selectedItem, const bool isMenuBar = false, unsigned int currDepth = 0);

};