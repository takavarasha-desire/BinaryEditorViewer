# BinaryEditorViewer
 The BinaryEditorViewer allows the visualisation of Binary data, in binary, hexedecimal and ascii format
 It allows the editing of binary data in hex format currently and ascii and binary displays are updated 
 accordingly. Inspired by Winfred Simon's QHexEdit. 
 
![bineditorviewer](https://github.com/takavarasha-desire/BinaryEditorViewer/assets/94230493/a1db6e21-95ec-4880-a56d-9ccaa98aa5c3)

## Compiling from Source Code
To compile from the source: 
  * Qt for the specific platform is required and can be found here: https://ww.qt.io.download
After downloading Qt.

  * Clone the repository: https://github.com/takavarasha-desire/BinaryEditorViewer
  * Open Qt Creator and Select "Open File or Project", then navigate to the .pro file in the cloned repository
  * Choose the C++ MinGW compiler
  * Click the green "Run" button or select "Build" from the build menu to compile the project
The project will compile and run when the green button has be placed and the application will function as normal.
  * The platform specific executable can be found in the build-folder
  * macdeployqt or linuxdeployqt can be used to bundle the application with all its dependencies in the release folder
  * See https://www.youtube.com/watch?v=2B--nzfoQ9s on using linuxdeployqt.
  * See https://www.youtube.com/watch?v=ANlXFtyARS4 on using macdeployqt.
  * The bundled application image for Windows 64 bit can be found 
  * here: https://github.com/takavarasha-desire/BinaryEditorViewer/blob/main/BinaryEditorViewer.zip
