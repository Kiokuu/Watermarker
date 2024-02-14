#pragma once
#include <string>

// Plan: Need to keep track of the dll its from, the function, and if it's original to the file or not


class ImportedFunction
{
public:
	explicit ImportedFunction(std::string_view name, bool isOriginal) :
		m_name(name), m_original(isOriginal)
	{
	}

	std::string_view getName() const { return m_name; }
	const bool getOriginal() const { return m_original; }

	void setOriginal(bool original) { m_original = original; }

private:
	std::string m_name;
	bool m_original;
};

class ImportedModule
{
public:
	explicit ImportedModule() = default;
	explicit ImportedModule(std::string_view name) :
		m_name(name)
	{
	}

	std::string_view getName() const { return m_name; }
	const std::vector<ImportedFunction>& getFunctions() const { return m_functions; }

	void addFunction(std::string_view name, bool isOriginal=false)
	{
		for (auto& function : m_functions)
		{
			if (function.getName() == name)
			{
				return;
			}
		}

		m_functions.emplace_back(name, isOriginal);
	}

	ImportedFunction* getFunction(std::string_view name)
	{
		for (auto& function : m_functions)
		{
			if (function.getName() == name)
			{
				return &function;
			}
		}
	
		return nullptr;
	}

private:
	std::string m_name;
	std::vector<ImportedFunction> m_functions;
};
