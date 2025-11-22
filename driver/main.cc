#include <Windows.h>
#include <common/debugcon.hh>

int WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nShowCmd) {
	UnusedVariable(hInstance);
	UnusedVariable(hPrevInstance);
	UnusedVariable(lpCmdLine);
	UnusedVariable(nShowCmd);

	cDebugConsole::instance().success("Allocated console!");

	cDebugConsole::instance().error("Unimplemented!");
	std::abort();
}
