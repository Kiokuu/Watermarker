#pragma once
#include "ExecutableFile.h"

class Disassembler;

class Binder
{
public:
	Binder(ExecutableFile* initialExecutable, ExecutableFile* targetExecutable);
	void bind();


private:
	ExecutableFile* m_initialExecutable;
	ExecutableFile* m_targetExecutable;
	ExecutableFile m_resultExecutable;
	Disassembler* m_disassembler;
};