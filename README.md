# BinaryEditorViewer

 The BinaryEditorViewer allows the visualisation of Binary data, in binary, hexedecimal and ascii format
 It allows the editing of binary data in hex format currently and ascii and binary displays are updated 
 accordingly. Inspired by Winfred Simon's QHexEdit. 
 
![bineditorviewer](https://github.com/takavarasha-desire/BinaryEditorViewer/assets/94230493/81bea4f4-126b-4709-a0d2-1e93a5e35775)

## Compiling from Source Code
To compile from the source: 
* Qt for the specific platform is required and can be found here: https://ww.qt.io.download
After downloading Qt.
* Clone this repository
* Open Qt Creator and Select "Open File or Project", then navigate to the .pro file in the cloned repository
* Choose the C++ MinGW compiler
* Click the green "Run" button or select "Build" from the build menu to compile the project
* The platform specific executable can be found in the build-folder
* macdeployqt or linuxdeployqt can be used to bundle the application with all its dependencies in the release folder
* The bundled application image for Windows 64 bit can be found here: 
