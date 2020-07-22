#include "../D3D12Engine/RenderAPI.hpp"
#include "../D3D12Engine/Utills/FileSystem.h"
#include "../D3D12Engine/Light/LightSource.h"
#include "Game.h"
#include <vector>

class Object;
class WindowInput;
enum class ResourceTypes {
	None = 0,
	Scene,
	Object,
	Light,
	Blueprint,
	Material,
	Texture,
	Shader,
	ShaderProgram,
	Other,
	///////////////////////
	SIZE_KEEP_THIS_LAST,
};

class Editor : public Game {
public:
	Editor();
	~Editor();

	// Inherited via GUI
	virtual void RenderGUI() override;
	virtual int Initialize() override;
private:
	std::string m_lightsourceBlueprintName = "_lightsource";
	std::string m_lightsourceMaterialName = "_lightsource";

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

	//TODO: Keep track of modifyed resources so that they can be saved.
	std::vector<std::string>    m_unSavedBlueprints;

	ResourceTypes m_selectedResourceType = ResourceTypes::None;
	std::vector<void*> m_selectedResources;
	std::unordered_set<Blueprint*> m_selectedSceneObjectBlueprints;
	std::vector<std::string> m_selectedFileNames;
	std::string m_lastClickedFileName;
	FileSystem::Directory m_resourceFileBrowser;
	std::string m_hoveredFile = "";

	float m_maxRandomPos = 100;

	virtual void SubmitObjectsForRendering() override;

	bool ClearSelectedResources();
	bool IsResourceSelected(void* res, ResourceTypes type);
	bool SetSelectedResource(void* res, ResourceTypes type);
	bool AddSelectedResource(void* res, ResourceTypes type);
	bool AddRemoveSelectedResource(void* res, ResourceTypes type);

	////////////////////
	void RenderMenuBar();
	void RenderSceneWindow();
	void RenderSceneWindow_AddMenu();
	void RenderResourceWindow();

	//Debug, Sandbox window
	void RenderDebugWindow();

	//PropertyWindow
	void RenderPropertiesWindow();
	void RenderPropertyWindowSceneObject();
	void RenderPropertyWindowLights();
	void RenderPropertyWindowBlueprint();
	void RenderPropertyWindowMaterial();
	void RenderPropertyUnimplementedResourceType();

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

	/*
	Save the current scene to file. The filename of the scene will be decided by m_currentSceneName.

	@param saveAsNew, if true m_currentSceneName will be set a non existing generated filename
		before saving in order to avoid overwriting a previusly saved scene.
		If Set to false m_currentSceneName will be used as the filename and potentially overwrite
		any previusly saved scene.

	@return true of succeeded
*/
	bool SaveScene(bool saveAsNew);
	/*
	Calls ClearScene() and sets up basic scene elements.
*/
	void NewScene();
};