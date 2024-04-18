#include "ImguiDraw.h"
#include <iostream>
#include "imgui.h"
#include "ImGuiFileDialog.h"


static ImVec2 g_windowSize;

ImguiDraw::ImguiDraw() : m_binder(new Binder())
{
}

void ImguiDraw::draw()
{
	// Begin drawing the window
	ImGui::Begin("Binder", nullptr, ImGuiWindowFlags_NoResize);

	// Get window size
	ImGuiIO& io = ImGui::GetIO();
	g_windowSize = io.DisplaySize;

	// File picker
	if (ImGui::Button("Select File"))
	{
		IGFD::FileDialogConfig config;
		config.path = ".";
		config.countSelectionMax = 1;
		config.flags = ImGuiFileDialogFlags_Modal;

		std::cout << "Opening file dialog" << std::endl;
		ImGuiFileDialog::Instance()->OpenDialog("ChooseFileDlgKey", "Choose File", ".exe", config);
	}

	ImVec2 maxSize = g_windowSize; // The full display area
	ImVec2 minSize = {g_windowSize.x * 0.5f, g_windowSize.y * 0.5f}; // Half the display area

	// If the file dialog is open, display it
	if (ImGuiFileDialog::Instance()->Display("ChooseFileDlgKey", 0, minSize, maxSize))
	{
		if (!ImGuiFileDialog::Instance()->IsOk())
		{
			setError("Failed to select file");
		}

		std::string filePathName = ImGuiFileDialog::Instance()->GetFilePathName();
		std::cout << "Selected file: " << filePathName << std::endl;

		// Attempt to load and attach to the executable
		if (!attemptBind(filePathName))
		{
			setError("Failed to bind file");
		}

		ImGuiFileDialog::Instance()->Close();
	}

	// Input text for watermark
	static char text[1024] = "";
	ImGui::InputText("Watermark", text, IM_ARRAYSIZE(text));

	// Embed watermark button
	if (ImGui::Button("Embed Watermark"))
	{
		embedWatermark(text);
	}

	// Exit button
	if(ImGui::Button("Exit"))
	{
		exit(0);
	}

	// If there is an error or success message, display a popup
	displayErrorPopup();
	displaySuccessPopup();

	ImGui::End();
}

void ImguiDraw::setError(const std::string& msg)
{
	m_errorMsg = msg;
}

void ImguiDraw::setSuccess(const std::string& msg)
{
	m_successMsg = msg;
}

bool ImguiDraw::attemptBind(const std::string& path)
{
	if (m_binder->bind(path))
	{
		return true;
	}
	setError("Failed to bind file");
	return false;
}

void ImguiDraw::embedWatermark(const std::string& watermarkText)
{
	std::cout << "Embedding watermark\n";

	if (!m_binder->isReadyToBind())
	{
		setError("No file selected");
	}
	else
	{
		if (!m_binder->save("output.exe", watermarkText))
		{
			setError("Failed to save file");
		}
		else
		{
			setSuccess("Successfully added watermark.\nOutput folder contains output.exe and watermark.dll");
		}
	}
}

void ImguiDraw::displayErrorPopup()
{
	if (!m_errorMsg.empty())
	{
		ImGui::OpenPopup("Error");
		if (ImGui::BeginPopupModal("Error", nullptr, ImGuiWindowFlags_AlwaysAutoResize))
		{
			ImGui::Text(m_errorMsg.c_str());
			if (ImGui::Button("OK"))
			{
				ImGui::CloseCurrentPopup();
				m_errorMsg.clear();
			}
			ImGui::EndPopup();
		}
	}
}

void ImguiDraw::displaySuccessPopup()
{
	if (!m_successMsg.empty())
	{
		ImGui::OpenPopup("Success");
		if (ImGui::BeginPopupModal("Success", nullptr, ImGuiWindowFlags_AlwaysAutoResize))
		{
			ImGui::Text(m_successMsg.c_str());
			if (ImGui::Button("OK"))
			{
				ImGui::CloseCurrentPopup();
				m_successMsg.clear();
			}
			ImGui::EndPopup();
		}
	}
}
