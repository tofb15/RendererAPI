#include "Editor.h"
#include <crtdbg.h>

/*This main is only an exemple of how this API could/should be used to render a scene.*/
int main() {
	_CrtSetDbgFlag(_CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF);

	Editor editor;
	if (editor.Initialize() < 0) {
		return 0;
	}
	editor.Run();

	return 0;
}