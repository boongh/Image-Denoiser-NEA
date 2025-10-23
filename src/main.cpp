#include <Application.h>

#pragma region Debug data
#define TESTDATA "E:/cdrive backup/downloads/25.9.6/Downloads/Firefox bg/117646530_p1.jpg"
	
#pragma endregion



int main() {
	Application app;
	app.DEBUGRUN(TESTDATA);
	app.Run();
	return 0;
}