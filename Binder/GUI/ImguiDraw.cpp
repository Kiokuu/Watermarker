#include "ImguiDraw.h"
#include <iostream>
#include "imgui.h"
#include "ImGuiFileDialog.h"

static ImVec2 g_windowSize;

ImguiDraw::ImguiDraw() : m_binder(new Binder()), m_errorMsg(""), m_error(false)
{
}

void ImguiDraw::Draw()
{
	ImGui::Begin("Binder" , 0 , ImGuiWindowFlags_NoResize);

    // Get window size
    ImGuiIO& io = ImGui::GetIO();
    g_windowSize = io.DisplaySize;

	// File picker
    if(ImGui::Button("Select File"))
    {
        IGFD::FileDialogConfig config;
        config.path = ".";
        config.countSelectionMax = 1;
        config.flags = ImGuiFileDialogFlags_Modal;

        std::cout << "Opening file dialog" << std::endl;
        ImGuiFileDialog::Instance()->OpenDialog("ChooseFileDlgKey", "Choose File", ".exe", config);
    }

    ImVec2 maxSize = g_windowSize;  // The full display area
    ImVec2 minSize = {g_windowSize.x * 0.5f, g_windowSize.y * 0.5f };  // Half the display area

    if (ImGuiFileDialog::Instance()->Display("ChooseFileDlgKey", 0, minSize, maxSize)) {

	    if (!ImGuiFileDialog::Instance()->IsOk()) { // action if OK
        	m_error = true;
            m_errorMsg = "Failed to select file";
        }

	    std::string filePathName = ImGuiFileDialog::Instance()->GetFilePathName();

        std::cout << "Selected file: " << filePathName << std::endl;

        if(!attemptBind(filePathName))
        {
        	m_error = true;
            m_errorMsg = "Failed to bind file";
        }

	    // close
	    ImGuiFileDialog::Instance()->Close();
    }

    // Text input
    static char text[1024] = "";
    ImGui::InputText("Watermark", text, IM_ARRAYSIZE(text));

    // Button
    if(ImGui::Button("Embed Watermark"))
    {
        std::cout << "Embedding watermark" << std::endl;

        if(!m_binder->isReadyToBind())
        {
        	m_error = true;
			m_errorMsg = "No file selected";
		}
        else
        {
        	if(!m_binder->save("output.exe", text))
	        {
        		m_error = true;
				m_errorMsg = "Failed to save file";
			}
        }
	}

    if(m_error)
    {
	    ImGui::OpenPopup("Error");
        m_error = false;
    }

    if(ImGui::BeginPopupModal("Error", 0, ImGuiWindowFlags_AlwaysAutoResize))
    {
    	ImGui::Text(m_errorMsg.c_str());
		if(ImGui::Button("OK"))
		{
			ImGui::CloseCurrentPopup();
		}
		ImGui::EndPopup();
    }
	ImGui::End();
}


bool ImguiDraw::attemptBind(std::string path)
{
    return m_binder->bind(path);
}

