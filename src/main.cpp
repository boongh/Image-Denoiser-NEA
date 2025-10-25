#include <Application.h>

#pragma region Debug data
#define TESTDATA "E:/cdrive backup/downloads/25.9.6/Downloads/Firefox bg/117646530_p1.jpg"
#define TESTDATA2 "E:/Programming/Projects/Image Denoising NEA/Image Denoising NEA/testdata/cbsd68/CBSD68/noisy35/0000.png"
	
#pragma endregion



int main() {
	Application app;
	app.DEBUGRUN(TESTDATA2);
	app.Run();
	return 0;
}