#pragma once
#include "../Binder.h"


/**
 * @brief Class for handling Imgui drawing and interactions.
 */
class ImguiDraw
{
public:
	/**
	 * @brief Default constructor.
	 */
	ImguiDraw();

	/**
	 * @brief Draws the Imgui interface.
	 */
	void draw();

private:
	Binder* m_binder; /**< Pointer to the Binder object. */
	std::string m_errorMsg; /**< Error message string. */
	std::string m_successMsg; /**< Success message string. */

	/**
	 * @brief Displays the error popup if there's an error message.
	 */
	void displayErrorPopup();

	/**
	 * @brief Displays the success popup if there's a success message.
	 */
	void displaySuccessPopup();

	/**
	 * @brief Sets the error message.
	 * @param msg The error message to be set.
	 */
	void setError(const std::string& msg);

	/**
	 * @brief Sets the success message.
	 * @param msg The success message to be set.
	 */
	void setSuccess(const std::string& msg);

	/**
	 * @brief Attempts to bind a file.
	 * @param path The path of the file to bind.
	 * @return True if binding is successful, false otherwise.
	 */
	bool attemptBind(const std::string& path);

	/**
	 * @brief Embeds a watermark into a file.
	 * @param watermarkText The text of the watermark.
	 */
	void embedWatermark(const std::string& watermarkText);
};
