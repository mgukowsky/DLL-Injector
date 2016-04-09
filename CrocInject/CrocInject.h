#pragma once

#ifdef CROCINJECT_EXPORTS
#define DECLSPEC		__declspec(dllexport)
#else
#define DECLSPEC		__declspec(dllimport)
#endif

namespace CrocMod {
DECLSPEC int init();
DECLSPEC void my_func();
DECLSPEC void my_func_close();

}