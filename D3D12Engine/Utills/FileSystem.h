#pragma once
#include <filesystem>
#include <unordered_set>
#include <vector>


namespace FileSystem {
	struct File
	{
		std::filesystem::path path = "";
	};

	struct Directory : public File
	{
		std::vector<Directory> directories;
		std::vector<File> files;
		/*
			Clears all sub directories and files from memory.
		*/
		void Clear() { directories.clear(); files.clear(); }
		/*
			Clears all sub directories and files from memory and resets the rootPath.
		*/
		void Reset() { Clear(); path = ""; }
	};

	/*
		Populate a FileSystem::Directory with all subdirectories and files with one or multiple specific file exension(s) from the filesystem.

		@param root, the root directory that will be filled with data.
			root.path must be set the the wanted root path.
		@param extList, contains the file extensions that should be listed. Leave empty to list all files.
		@param maxDepth, the maximum directory depth. If set to -1 no maximum depth will be used.
		
		@return true if succeeded.
	*/
	bool ListDirectory(FileSystem::Directory& root, const std::unordered_set<std::string>& extList, unsigned int maxDepth = -1);
	/*
		Populate a FileSystem::Directory with all subdirectories and files with one or multiple specific file exension(s) from the filesystem.

		@param root, the root directory that will be filled with data.
		@param rootPath, the starting path of the root
		@param extList, contains the file extensions that should be listed. Leave empty to list all files.
		@param maxDepth, the maximum directory depth. If set to -1 no maximum depth will be used.

		@return true if succeeded.
	*/
	bool ListDirectory(FileSystem::Directory& root, const std::string& rootPath, const std::unordered_set<std::string>& extList, unsigned int maxDepth = -1);
	/*
		Populate a FileSystem::Directory with all subdirectories and files from the filesystem.

		@param root, the root directory that will be filled with data.
		@param rootPath, the starting path of the root
		@param maxDepth, the maximum directory depth. If set to -1 no maximum depth will be used.

		@return true if succeeded.
	*/
	bool ListDirectory(FileSystem::Directory& root, const std::string& rootPath, unsigned int maxDepth = -1);
	/*
		Populate a FileSystem::Directory with all subdirectories and files from the filesystem.

		@param root, the root directory that will be filled with data.
			root.path must be set the the wanted root path.
		@param maxDepth, the maximum directory depth. If set to -1 no maximum depth will be used.

		@return true if succeeded.
	*/
	bool ListDirectory(FileSystem::Directory& root, unsigned int maxDepth = -1);
}