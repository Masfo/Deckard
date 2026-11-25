module;
#include <windows.h>

export module deckard.ipc;

import deckard.types;
import std;

namespace deckard::ipc
{

	export enum class message_type : u8 {
		unknown  = 0,
		request  = 1,
		response = 2,
		log      = 3,

	};

	export struct message
	{
		u32             id = 0;
		std::vector<u8> data;
		message() = default;

		message(u32 id, std::vector<u8> data)
			: id(id)
			, data(std::move(data))
		{
		}
	};

	export struct SharedMemory
	{
		void* hFileMap;
		void* pData;
		std::string MapName;
		u32   Size;
	};

	export bool CreateMemoryMap(SharedMemory* shm)
	{
		if ((shm->hFileMap = CreateFileMappingA(INVALID_HANDLE_VALUE, NULL, PAGE_READWRITE, 0, shm->Size, shm->MapName.c_str()))
			== NULL)
		{
			return false;
		}

		if ((shm->pData = MapViewOfFile(shm->hFileMap, FILE_MAP_ALL_ACCESS, 0, 0, shm->Size)) == NULL)
		{
			CloseHandle(shm->hFileMap);
			return false;
		}
		return true;
	}

	export bool FreeMemoryMap(SharedMemory* shm)
	{
		if (shm and shm->hFileMap)
		{
			if (shm->pData)
			{
				UnmapViewOfFile(shm->pData);
			}

			if (shm->hFileMap)
			{
				CloseHandle(shm->hFileMap);
			}
			return true;
		}
		return false;
	}
}; // namespace deckard::ipc
