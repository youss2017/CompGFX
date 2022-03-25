#pragma once
#include <Graphics.hpp>

namespace Application {

	extern bool Quit;

	/*
		If the following three functions return false, it means that cirtical resources could be created or initalized.
	*/
	bool Initalize(ConfigurationSettings* configuration, bool RenderDOC);
	bool LoadAssets();
	bool CreateResources();
	/*
		True/False determine whether to render or not.
	*/
	bool Update(double dTimeFromStart, double dTime, double FrameRate, bool UpdateUIInfo);
	void Render(double dTimeFromStart, double dTime, bool UpdateUITime);
	void Destroy();

}