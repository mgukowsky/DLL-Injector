#pragma once

#include <cstdio>
#include <Windows.h>

namespace CrocMod {

class Injector {
public:
	Injector();
	~Injector();

	bool inject(HANDLE hCrocProc, DWORD pID);

private:
};

}