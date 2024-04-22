# Watermarker

## Overview
The aim of this project is to provide a proof-of-concept solution for developers to apply customizable watermarks to their DirectX 11 executables. 

## Contents
This project contains 2 subprojects: `Binder` and `Watermark`.

`Binder` is a program that applies a watermark to an DirectX 11 executable file, rewriting the executable to import the `Watermark` library.
It contains a GUI that allows the user to select the executable file to be watermarked and the text to embed.

`Watermark` is a library that provides a function to modify the rendering process of a DirectX 11 application to display a watermark

In addition, a public domain test application is provided `DX11TestApplication`
`DX11TestApplication` is a simple DirectX 11 application, that can be targetted using the binder. 
The original source for `DX11TestApplication` can be found [here](https://github.com/kevinmoran/BeginnerDirect3D11/)

## Building the Project (Visual Studio)
The following are instructions for building the project using Visual Studio

### Prerequisites
- CMake 3.19 or later
- Windows 10 SDK
- DirectX 11

### Build guide
- Clone the repository
- Open the project in Visual Studio
- Open the build menu and select `Build All`
- The compiled binaries will be located in the `bin` folder (typically `bin\AMD64\RelWithDebInfo`)

## Usage Instructions
- Run the `Binder.exe` executable 
- Select the executable file to be watermarked
- Enter the watermark text
- Click the `Watermark` button
- The watermarked executable will be saved in the same directory as the original executable as `output.exe`
- Run the watermarked executable or replace the original executable

## Folder Overview
<pre>
.
|
├── .old                    # Unused source files, kept for reference. reassembly approach
├── bin                     # Compiled executable files 
├── Binder                  # The Binder subproject source code 
├── docs                    # Documentation files
├── DX11TestApplication     # The DX11TestApplication source code
├── Licenses                # Contains the copyright/licenses of libraries used within project
├── Watermark               # The Watermark subproject source code
|
├── .clang-uml              # Instructions for clang-uml to build diagrams
├── .gitignore              # Files for git to ignore tracking
├── CMakeLists.txt          # Describes the CMake project format
├── CMakePresets.json       # Describes CMAKE build processes for the project
└── README.md               # This file!
</pre>

## Libraries used
[ImGui](https://github.com/ocornut/imgui/)
[ImGuiFileDialog](https://github.com/aiekick/ImGuiFileDialog/)
[MinHook](https://github.com/TsudaKageyu/minhook)
[CImg](https://github.com/GreycLab/CImg/)
[Zasm](https://github.com/zyantific/zasm/)