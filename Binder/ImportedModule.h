#pragma once

#include <string>
#include <variant>
#include <vector>

/**
 * @brief Represents an imported function.
 */
class ImportedFunction
{
public:
	using FunctionData = std::variant<std::string, uint64_t>;

	/**
	 * @brief Constructs an ImportedFunction object with a name.
	 * @param name The name of the function.
	 * @param isOriginal Flag indicating if the function is original to the file.
	 */
	ImportedFunction(std::string name, bool isOriginal) :
		m_data(name), m_original(isOriginal), m_isOrdinal(false)
	{
	}

	/**
	 * @brief Constructs an ImportedFunction object with an ordinal.
	 * @param ordinal The ordinal of the function.
	 * @param isOriginal Flag indicating if the function is original to the file.
	 */
	ImportedFunction(uint64_t ordinal, bool isOriginal) :
		m_data(ordinal), m_original(isOriginal), m_isOrdinal(true)
	{
	}

	/**
	 * @brief Gets the name of the function.
	 * @return The name of the function.
	 */
	std::string_view getName() const { return std::get<std::string>(m_data); }

	/**
	 * @brief Gets the ordinal of the function.
	 * @return The ordinal of the function.
	 */
	uint64_t getOrdinal() const { return std::get<uint64_t>(m_data); }

	/**
	 * @brief Checks if the function is original to the file.
	 * @return True if the function is original, false otherwise.
	 */
	bool getOriginal() const { return m_original; }

	/**
	 * @brief Checks if the function is referenced by ordinal.
	 * @return True if the function is referenced by ordinal, false otherwise.
	 */
	bool getIsOrdinal() const { return m_isOrdinal; }

	/**
	 * @brief Sets whether the function is original to the file.
	 * @param original Flag indicating if the function is original.
	 */
	void setOriginal(bool original) { m_original = original; }

private:
	FunctionData m_data; /**< Variant representing the function name or ordinal. */
	bool m_original; /**< Flag indicating if the function is original to the file. */
	bool m_isOrdinal; /**< Flag indicating if the function is referenced by ordinal. */
};

/**
 * @brief Represents an imported module.
 */
class ImportedModule
{
public:
	/**
	 * @brief Default constructor.
	 */
	explicit ImportedModule() = default;

	/**
	 * @brief Constructs an ImportedModule object with a name.
	 * @param name The name of the imported module.
	 */
	explicit ImportedModule(std::string_view name) :
		m_name(name)
	{
	}

	/**
	 * @brief Gets the name of the imported module.
	 * @return The name of the imported module.
	 */
	std::string_view getName() const { return m_name; }

	/**
	 * @brief Gets the vector of imported functions.
	 * @return The vector of imported functions.
	 */
	const std::vector<ImportedFunction>& getFunctions() const { return m_functions; }

	/**
	 * @brief Adds a function to the imported module by name.
	 * @param name The name of the function.
	 * @param isOriginal Flag indicating if the function is original to the file.
	 */
	void addFunction(std::string name, bool isOriginal = false)
	{
		for (auto& function : m_functions)
		{
			if (!function.getIsOrdinal() && function.getName() == name)
			{
				return;
			}
		}

		m_functions.emplace_back(name, isOriginal);
	}

	/**
	 * @brief Adds a function to the imported module by ordinal.
	 * @param ordinal The ordinal of the function.
	 * @param isOriginal Flag indicating if the function is original to the file.
	 */
	void addFunction(uint64_t ordinal, bool isOriginal = false)
	{
		for (auto& function : m_functions)
		{
			if (function.getIsOrdinal() && function.getOrdinal() == ordinal)
			{
				return;
			}
		}

		m_functions.emplace_back(ordinal, isOriginal);
	}

	/**
	 * @brief Gets a pointer to an imported function by name.
	 * @param name The name of the function.
	 * @return A pointer to the imported function, or nullptr if not found.
	 */
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
	std::string m_name; /**< Name of the imported module. */
	std::vector<ImportedFunction> m_functions; /**< Vector of imported functions. */
};
