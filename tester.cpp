#include <iostream>
#include <Windows.h>

int main()
{
	LoadLibrary(L"prot.dll");
	std::cout << "test" << std::endl;
}
