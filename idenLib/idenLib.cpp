//
// author: Lasha Khasaia
// contact: @_qaz_qaz
//

#include "utils.h"

void ProcessFile(const fs::path& sPath);

int main(int argc, char *argv[])
{
	if (argc < 2)
	{
		printf(
			"Usage: \n\
		./idenLib.exe /path/to/sample\n\
		./idenLib.exe /path/to/dir\n\
		./idenLib.exe /path/to/dir filename\
		\n");
		return STATUS_UNSUCCESSFUL;
	}

	fs::path sPath{ argv[1] };


	if (!exists(sPath))
	{
		wprintf(L"[!] Invalid path: %s\n", sPath.c_str());
		return STATUS_UNSUCCESSFUL;
	}

	if (!exists(symExPath))
	{
		create_directory(symExPath);
	}

	if (is_regular_file(sPath))
	{
		ProcessFile(sPath);
	}
	else
	{
		std::error_code ec{};
		for (auto& p : fs::recursive_directory_iterator(sPath, fs::directory_options::skip_permission_denied, ec))
		{
			if (ec.value() != STATUS_SUCCESS)
			{
				continue;
			}
			const auto& currentPath = p.path();
			if (is_regular_file(currentPath, ec))
			{
				if (ec.value() != STATUS_SUCCESS)
				{
					continue;
				}
				if (argc == 3)
				{
					if (std::wstring::npos != currentPath.filename().string().find(argv[2]))
					{
						ProcessFile(currentPath);
					}
				}
				else
				{
					ProcessFile(currentPath);
				}
			}
		}
	}

	printf("------------- EOF -------------");
	return STATUS_SUCCESS;
}


void ProcessArchiveFile(const fs::path& sPath)
{
	Lib lib{ sPath };
	USER_CONTEXT userContext{};

	const auto fileName = sPath.filename();
	auto sigPath{ symExPath };
	sigPath += L"\\";
	sigPath += fileName;
	sigPath += SIG_EXT;

	if (exists(sigPath))
	{
		PBYTE decompressedData{};
		if (!DecompressFile(sigPath, decompressedData) || !decompressedData)
		{
			wprintf_s(L"[idenLib] failed to decompress the file: %s\n", sigPath.c_str());
			return;
		}
		char seps[] = "\n";
		char* next_token = nullptr;
		char* line = strtok_s(reinterpret_cast<char*>(decompressedData), seps, &next_token);
		while(line != nullptr)
		{
			std::vector<std::string> vec{};
			Split(line, vec);
			if (vec.size() != 2)
			{
				wprintf(L"[!] SIG file contains a malformed data, SIG path: %s\n", sigPath.c_str());
				return;
			}
			// vec[0] opcode
			// vec[1] name
			userContext.funcSignature[vec[0]] = vec[1];
			line = strtok_s(nullptr, seps, &next_token);
		}


		delete[] decompressedData;
	}

	userContext.Dirty = false;

	if (lib.GetSignature(&userContext) && userContext.Dirty)
	{
		fs::path sigPathTmp = sigPath;
		sigPathTmp += L".tmp";
		if (fs::exists(sigPathTmp))
		{
			fs::remove(sigPathTmp);
		}

		FILE* hFile = nullptr;
		fopen_s(&hFile, sigPathTmp.string().c_str(), "wb");
		if (!hFile)
		{
			printf("[idenLib] failed to create sig file: %s\n", sigPath.string().c_str());
			return;
		}
		for (const auto& n : userContext.funcSignature)
		{
			const auto bothSize = n.first.size() + n.second.size() + 3; // space + \n + 0x00
			const auto opcodesName = new CHAR[bothSize];
			sprintf_s(opcodesName, bothSize, "%s %s\n", n.first.c_str(), n.second.c_str());
			fwrite(opcodesName, bothSize, 1, hFile);
		}
		fclose(hFile);

		if (CompressFile(sigPathTmp, sigPath)) {
			wprintf(L"Created SIG file: %s based on %s\n", sigPath.c_str(), sPath.c_str());
		}
		else {
			wprintf(L"[idenLib] compression failed\n");
		}
		if (fs::exists(sigPathTmp))
			fs::remove(sigPathTmp);
	}
	else
	{
		printf("No SIG file for : %s (maybe no functions)\n", sPath.string().c_str());
	}

	if (exists(sigPath) && is_empty(sigPath))
	{
		DeleteFile(sigPath.c_str());
	}
}

void ProcessFile(const fs::path& sPath)
{
	if (sPath.extension().compare(".lib") == 0) // Should we check signature instead?
	{
		ProcessArchiveFile(sPath);
	}


	// parse PE files based on pdb - REMOVED
}
