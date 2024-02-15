#pragma once
#include <string>
#include <variant>

// Plan: Need to keep track of the dll its from, the function, and if it's original to the file or not


class ImportedFunction
{
public:
	using FunctionData = std::variant<std::string, uint64_t>;

	ImportedFunction(std::string name, bool isOriginal) :
		m_data(name), m_original(isOriginal), m_isOrdinal(false)
	{
	}

	ImportedFunction(uint64_t ordinal, bool isOriginal) :
		m_data(ordinal), m_original(isOriginal), m_isOrdinal(true)
	{
	}

    std::string_view getName() const { return std::get<std::string>(m_data); }
    uint64_t getOrdinal() const { return std::get<uint64_t>(m_data); }
    bool getOriginal() const { return m_original; }
    bool getIsOrdinal() const { return m_isOrdinal; }

	void setOriginal(bool original) { m_original = original; }

private:
	FunctionData m_data;
	bool m_original;
	bool m_isOrdinal;
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

	void addFunction(std::string name, bool isOriginal=false)
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

	void addFunction(uint64_t ordinal, bool isOriginal=false)
	{
		for (auto& function : m_functions)
		{
			if (function.getIsOrdinal() && function.getOrdinal()==ordinal)
			{
				return;
			}
		}

		m_functions.emplace_back(ordinal, isOriginal);
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
