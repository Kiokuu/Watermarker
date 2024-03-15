#pragma once
#include "../Binder.h"

class ImguiDraw
{
public:
	ImguiDraw();
	void Draw();

private:
	Binder* m_binder;
	std::string m_errorMsg;
	bool m_error;

	bool attemptBind(std::string path);

};