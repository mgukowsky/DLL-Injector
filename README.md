#DLL Injector
A simple, extensible Windows DLL injector for the classic game "Croc: Legend of the Gobbos"

##About
Intended as an illustration of how to implement DLL injection on Windows platforms, without relying on wrapper libraries such as [Microsoft Detours](http://research.microsoft.com/en-us/projects/detours/). All source files are thoroughly commented to describe the structure of the application, and many of the algorithms are adapted from [this]( http://www.codeproject.com/Articles/30140/API-Hooking-with-MS-Detours) article.

##DLL Injection
DLL injection takes advantage of Dynamic Link Libraries (DLLs), Windows's implementation of shared libraries. A DLL contains compiled code which can be shared between processes. The key advantages of DLLs are:

* executables have reduced memory footprints, since they do not each need a copy of the library.
* a distributor can update an application by only changing the DLL, leaving the main executable untouched.

Windows gamers may be familiar with the in-game Steam interface (Shift + Tab, anyone?), which is consistent across games. This is because a given Steam application provides this interface by linking against 'steam_api.dll'. Because this DLL is separate from the executable, it allows for consistent functionality across programs, and Steam can update the DLL without touching the executable!

What DLL injection does is insert 'redirection' code into the beginning of a function. Due to the way DLLs are loaded, we can take advantage of various Win32 functions to load a custom DLL into another process (the '*injection*'). When that DLL is loaded, the process calls the DLL's DllMain() function, which is hard-coded to locate the address of a specific function. DllMain() then overrides the machine code at the target address to make a jump to another function, which is where we can execute our custom code. 

Using MessageBox() as an example, whenever our victim process calls MessageBox(), control flow will be moved to the address of MessageBox() as usual, except now the process finds our custom jump code and instead is redirected to execute our custom code. With care, we can make our custom code complete the original function call to MessageBox() as intended, and send back a return value that the program expects so that it doesn't crash.

DLL injection is an extremely powerful technique to change the behavior of an executable, just remember to use it with care!

##Usage
In Visual Studio 2015 or later, simply open up the solution and you're ready to go. Before building, be sure to change the two strings in main.cpp in the 'CrocMod' project to the directory and path of your computer's Croc.exe, and the strings in Injector.cpp in the 'CrocMoc' project to the directory where the Debug/Release configurations of CrocInject.dll can be found (after building the DLL first).

The default behavior of the injector is to hook onto DefWindowProcA and play a quarter-second beep when the 'B' key is pressed.
