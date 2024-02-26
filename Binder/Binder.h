#pragma once
#include "ExecutableFile.h"

class Binder
{
public:
	Binder(ExecutableFile* initialExecutable, ExecutableFile* targetExecutable);
	void bind();


private:
	ExecutableFile* m_initialExecutable;
	ExecutableFile* m_targetExecutable;
	ExecutableFile m_resultExecutable;
};