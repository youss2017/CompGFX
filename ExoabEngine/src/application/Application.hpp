#pragma once
#include <Graphics.hpp>

namespace Application {

	extern bool Quit;

	/*
		If the following three functions return false, it means that cirtical resources could be created or initalized.
	*/
	bool Initalize(ConfigurationSettings* configuration);
	bool LoadAssets();
	bool CreateResources();
	/*
		True/False determine whether to render or not.
	*/
	bool Update(double dTime, double FrameRate);
	void Render();
	void Destroy();

}