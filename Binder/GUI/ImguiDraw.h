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

	void setError(std::string msg);
	bool attemptBind(std::string path);
};
