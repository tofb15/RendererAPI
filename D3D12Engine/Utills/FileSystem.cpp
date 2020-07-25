#include "stdafx.h"
#include "FileSystem.h"
#include <string>

namespace FileSystem {
	/*
		Recursively fills a FileSystem::Directory with all sub elements untill maxDepth is reached.
	*/
	bool ListDir(FileSystem::Directory& dirPath, const std::unordered_set<std::string>& extList, unsigned int curDepth, unsigned int maxDepth) {
		for (auto file : std::filesystem::directory_iterator(dirPath.path)) {
			if (file.is_directory()) {
				dirPath.directories.push_back({ file });
				if (curDepth < maxDepth) {
					ListDir(dirPath.directories.back(), extList, curDepth + 1, maxDepth);
				}
			} else {
				std::string ext = file.path().extension().string();
				for (auto& c : ext) {
					c = std::tolower(c);
				}
				if (extList.empty() || extList.count(ext)) {
					dirPath.files.push_back({ file });
				}
			}
		}

		return true;
	}

	bool FileSystem::ListDirectory(FileSystem::Directory& root, const std::unordered_set<std::string>& extList, unsigned int maxDepth) {
		root.Clear();
		if (!std::filesystem::exists(root.path)) {
			return false;
		}

		return FileSystem::ListDir(root, extList, 0, maxDepth);
	}

	bool FileSystem::ListDirectory(FileSystem::Directory& root, unsigned int maxDepth) {
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
}

