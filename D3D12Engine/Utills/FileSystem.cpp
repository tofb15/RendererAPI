#include "stdafx.h"
#include "FileSystem.h"

namespace FileSystem {
	/*
		Recursively fills a FileSystem::Directory with all sub elements untill maxDepth is reached.
	*/
	bool ListDir(FileSystem::Directory& dirPath, const std::unordered_set<std::string>& extList, unsigned int curDepth, unsigned int maxDepth)
	{
		for (auto file : std::filesystem::directory_iterator(dirPath.path))
		{
			if (file.is_directory() && curDepth < maxDepth) {
				dirPath.directories.push_back({ file });
				ListDir(dirPath.directories.back(), extList, curDepth + 1, maxDepth);
			}
			else {
				if (extList.empty() || extList.count(file.path().extension().string())) {
					dirPath.files.push_back({ file });
				}
			}
		}
	
		return true;	
	}
}

bool FileSystem::ListDirectory(FileSystem::Directory& root, const std::unordered_set<std::string>& extList, unsigned int maxDepth)
{
	root.Clear();
	if (!std::filesystem::exists(root.path)) {
		return false;
	}

	return FileSystem::ListDir(root, extList, 0, maxDepth);
}

bool FileSystem::ListDirectory(FileSystem::Directory& root, unsigned int maxDepth)
{
	return FileSystem::ListDirectory(root, std::unordered_set<std::string>(), maxDepth);
}

bool FileSystem::ListDirectory(FileSystem::Directory& root, const std::string& rootPath, const std::unordered_set<std::string>& extList, unsigned int maxDepth) {
	root.path = rootPath;
	return FileSystem::ListDirectory(root, extList, maxDepth);
}

bool FileSystem::ListDirectory(FileSystem::Directory& root, const std::string& rootPath, unsigned int maxDepth) {
	root.path = rootPath;
	return FileSystem::ListDirectory(root, std::unordered_set<std::string>(), maxDepth);
}
