// Binder.cpp : Defines the entry point for the application.
//
#include "Binder.h"
#include "Disassembler.h"

Binder::Binder(ExecutableFile* initialExecutable, ExecutableFile* targetExecutable) : m_initialExecutable(initialExecutable), m_targetExecutable(targetExecutable), m_resultExecutable(), m_disassembler(new Disassembler(m_initialExecutable))
{
    //bind();
}

void Binder::bind()

{
}