#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <cstdio>
#include <unordered_map>
#include <mutex>
#include <queue>

std::unordered_map<unsigned long long, bool> GuardedPages;
std::queue<unsigned long long> LastUnlockedPages;

char SecretXorKey[] = {0x13, 0x37};

bool IsGuardedPage(unsigned long long Address)
{
	return GuardedPages.count(Address);
}

void SetPageFlag(unsigned long long address, bool encrypted)
{
	GuardedPages[address] = encrypted;

	LastUnlockedPages.push(address);
}

void EncryptPage(unsigned long long PageAddress)
{
	if (GuardedPages[PageAddress])
		return;

	DWORD oldProtect;

	unsigned char* PagePtr = (unsigned char*)PageAddress;
	VirtualProtect(PagePtr, 0x1000, PAGE_READWRITE, &oldProtect);

	for (int i = 0; i < 0x1000; i++)
	{
		PagePtr[i] ^= SecretXorKey[i % (sizeof(SecretXorKey) / sizeof(char))];
	}

	VirtualProtect(PagePtr, 0x1000, PAGE_NOACCESS, &oldProtect);
	SetPageFlag(PageAddress, true);
}

void DecryptPage(unsigned long long PageAddress)
{
	if (!IsGuardedPage(PageAddress) || !GuardedPages[PageAddress])
		return;

	DWORD oldProtect;

	unsigned char* PagePtr = (unsigned char*)PageAddress;
	VirtualProtect(PagePtr, 0x1000, PAGE_READWRITE, &oldProtect);

	for (int i = 0; i < 0x1000; i++)
	{
		PagePtr[i] ^= SecretXorKey[i % (sizeof(SecretXorKey) / sizeof(char))];;
	}

	VirtualProtect(PagePtr, 0x1000, PAGE_EXECUTE_READ, &oldProtect);
	SetPageFlag(PageAddress, false);
}

void EncryptOldPages()
{
	if (LastUnlockedPages.size() > 0x100)
	{
		auto address = LastUnlockedPages.front();
		LastUnlockedPages.pop();
		EncryptPage(address);
	}
}

void EncryptTextSection()
{
	auto imageBase = GetModuleHandle(nullptr);
	auto dosHeader = (PIMAGE_DOS_HEADER)imageBase;
	auto ntHeader = (PIMAGE_NT_HEADERS)((unsigned char*)imageBase + dosHeader->e_lfanew);
	auto fileHeader = (PIMAGE_FILE_HEADER)((unsigned char*)&ntHeader->FileHeader);
	auto optionalHeader = (PIMAGE_OPTIONAL_HEADER)((unsigned char*)&ntHeader->OptionalHeader);

	auto sectionLocation = (unsigned char*)imageBase + dosHeader->e_lfanew + sizeof(IMAGE_NT_HEADERS);
	auto sectionSize = sizeof(IMAGE_SECTION_HEADER);

	auto sectionHeader = (PIMAGE_SECTION_HEADER)sectionLocation;

	void* textLocation = nullptr;
	unsigned long long textSize;

	for (int i = 0; i < fileHeader->NumberOfSections; i++)
	{
		if (strcmp((char*)sectionHeader->Name, ".text") == 0)
		{
			textLocation = (unsigned char*)imageBase + sectionHeader->VirtualAddress;
			textSize = sectionHeader->SizeOfRawData;
			break;
		}

		sectionHeader++;
	}

	if (textLocation == nullptr)
		return;

	auto pageCount = textSize / 0x1000;

	for (unsigned long long i = 0; i < pageCount; i++)
	{
		auto pageAddress = (unsigned long long)textLocation + i * 0x1000;
		EncryptPage(pageAddress);
	}
}

std::mutex decryptionMutex;

LONG DecryptionExceptionHandler(_EXCEPTION_POINTERS* ExceptionInfo)
{
	std::lock_guard<std::mutex> guard(decryptionMutex);
	if (ExceptionInfo->ExceptionRecord->ExceptionCode == EXCEPTION_ACCESS_VIOLATION)
	{
		auto pageAddress = ExceptionInfo->ExceptionRecord->ExceptionInformation[1] / 0x1000 * 0x1000;
		auto nextPageAddress = pageAddress + 0x1000;
		if (IsGuardedPage(pageAddress))
		{
			DecryptPage(pageAddress);
			EncryptOldPages();
			return EXCEPTION_CONTINUE_EXECUTION;
		}
	}

	return EXCEPTION_CONTINUE_SEARCH;
}

extern "C" __declspec(dllexport) 
int tdghjjgyfjtyr()
{
	EncryptTextSection();

	AddVectoredExceptionHandler(1, DecryptionExceptionHandler);

	return 3790 - 13 - 42 + 3;
}