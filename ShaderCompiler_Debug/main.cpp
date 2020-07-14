/*
	Small tool to check if shader code compile as it is beeing edited.
	Each time the shader file is saved in the selected path, this program tries to compile it and reports the result.
*/
#include "../D3D12Engine/D3D12/DXR/DXILShaderCompiler.h"
#include <filesystem>
#include <chrono>
#include <functional>
#include <winerror.h>
#include <thread>
#include <iostream>


//==Shader Names==
const WCHAR* m_shader_rayGenName = L"rayGen";
const WCHAR* m_shader_closestHitName = L"closestHitTriangle";
const WCHAR* m_shader_missName = L"miss";
const WCHAR* m_shader_shadowMissName = L"shadowMiss";

//==Shader Group Names==
const WCHAR* m_group_group1 = L"hitGroupTriangle";

/*
	A class to watch for file status updates in a directory
*/
class FileWatcher
{
public:
	enum class FileStatus { created, modified, erased };

	FileWatcher(std::string path, std::chrono::duration<int, std::milli> delay);
	~FileWatcher();
	/*
		Start watching the directory. If somthing changes, "action" is called.
	*/
	void start(const std::function<void(std::string, FileStatus)>& action);
private:
	std::chrono::duration<int, std::milli> m_delay;
	std::string m_path;
	bool m_running = true;
	std::unordered_map<std::string, std::filesystem::file_time_type> m_paths;

	bool contains(const std::string& key) {
		auto el = m_paths.find(key);
		return el != m_paths.end();
	}
};

FileWatcher::FileWatcher(std::string path, std::chrono::duration<int, std::milli> delay)
{
	m_path = path;
	m_delay = delay;
	m_running = true;

	for (auto& file : std::filesystem::recursive_directory_iterator(m_path)) {
		m_paths[file.path().string()] = std::filesystem::last_write_time(file);
	}
}

FileWatcher::~FileWatcher()
{

}

void FileWatcher::start(const std::function<void(std::string, FileStatus)>& action)
{
	while (m_running) {
		// Wait for "delay" milliseconds
		std::this_thread::sleep_for(m_delay);

		auto it = m_paths.begin();
		while (it != m_paths.end()) {
			if (!std::filesystem::exists(it->first)) {
				action(it->first, FileStatus::erased);
				it = m_paths.erase(it);
			}
			else
			{
				it++;
			}
		}

		// Check if a file was created or modified
		for (auto& file : std::filesystem::recursive_directory_iterator(m_path)) {
			auto current_file_last_write_time = std::filesystem::last_write_time(file);

			// File was created
			if (!contains(file.path().string())) {
				m_paths[file.path().string()] = current_file_last_write_time;
				action(file.path().string(), FileStatus::created);
			}
			else {
				// File was modified
				if (m_paths[file.path().string()] != current_file_last_write_time) {
					m_paths[file.path().string()] = current_file_last_write_time;
					action(file.path().string(), FileStatus::modified);
				}

			}

		}

	}
}


int main() {

	/*
		Init the compiler
	*/
	DXILShaderCompiler dxilCompiler;
	if (FAILED(dxilCompiler.init())) {
		return false;
	}

	/*
		Shader defines that is to be added
	*/
	std::vector<DxcDefine> defines;

	defines.push_back({ L"ddx(x)" , L"x" });
	defines.push_back({ L"ddy(x)" , L"x" });
	defines.push_back({ L"discard" , L"" });
	defines.push_back({ L"Sample(sampler, uv, ...)" , L"SampleLevel(sampler, uv, 0)" });
	defines.push_back({ L"SampleGrad(sampler, uv, ...)" , L"SampleLevel(sampler, uv, 0)" });
	defines.push_back({ L"clip(x)" , L"" });

	/*
		Folder to watch
	*/
	std::string path = "../../Exported_Assets/Shader/CompileWatch/";

	FileWatcher fw(path, std::chrono::milliseconds(500));
	/*
		Start watching
	*/
	fw.start([&defines, &dxilCompiler](std::string p, FileWatcher::FileStatus status) -> void {
		UINT index = p.find_last_of(".");
		if (index != p.npos) {
			if (p.substr(index + 1, p.length() - 1) == "hlsl") {
				//If the file is a HLSL file and it was created or changed. Compile it
				if (status == FileWatcher::FileStatus::created || status == FileWatcher::FileStatus::modified) {
					DXILShaderCompiler::Desc shaderDesc;
					shaderDesc.compileArguments.push_back(L"/Gis"); // Declare all variables and values as precise
#ifdef _DEBUG
					shaderDesc.compileArguments.push_back(L"/Zi"); // Debug info
#endif
					std::wstring stemp = std::wstring(p.begin(), p.end());
					shaderDesc.filePath = stemp.c_str();
					shaderDesc.entryPoint = L"main";
					shaderDesc.targetProfile = L"lib_6_3";
					shaderDesc.defines = defines;

					IDxcBlob* pShaders = nullptr;
					std::wstring error;
					//Compile
					if (SUCCEEDED(dxilCompiler.compile(&shaderDesc, &pShaders, &error))) {
						std::cout << "Compiled OK: " << p << "\n";
					}
					else {
						std::cout << "Compiled FAILED: " << p << "\n";
						std::cout << error.c_str() << std::endl;
					}
				}
			}
		}

		});

	return 0;
}