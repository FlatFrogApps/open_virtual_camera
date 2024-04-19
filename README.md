# Open Virtual Camera

## Runtime

### Dependencies
* [Microsoft Visual C++ Redistributable](https://learn.microsoft.com/en-US/cpp/windows/latest-supported-vc-redist?view=msvc-170#visual-studio-2015-2017-2019-and-2022)

### Configuration
The camera to be virtualized can be specified in the camera_name.txt file. If the camera is not found, the first camera available will be chosen.
vcam.exe needs to be restarted for the changes to camera_name.txt to take effect.

### Install
* Download the latest [release](https://github.com/FlatFrogApps/open_virtual_camera/releases) and unzip
* Run install.bat as administrator
* Choose if you want Open Virtual Camera to start with Windows

This will create a soft link from the startup folder to the current directory so if vcam.exe is later moved the virtual camera will no longer work on startup.
To fix this just uninstall and install again.

### Uninstall
* Run uninstall.bat as administrator

### Run
If needed vcam.exe can be run from the command line with parameters.
More information about these parameters can be found by running
```Batchfile
vcam.exe -h
```

## Build 
### Build Dependencies
* [CMake](https://cmake.org/download/) is required to build this project
* [OpenCV 4.9.0](https://github.com/opencv/opencv/releases/download/4.9.0/opencv-4.9.0-windows.exe) needs to be dynamically linked

### Build Instructions
* Download and extract [OpenCV 4.9.0](https://github.com/opencv/opencv/releases/download/4.9.0/opencv-4.9.0-windows.exe). 
* Move all the contents to third_party/opencv
* Place yourself in the root of the repo and run:

```Batchfile
cmake . -B build/
```

* Compile the .sln file created in the build directory with Visual Studio

## License
Code has been copied and modified from following repositories with GPL-2.0 licenses
* [pyvirtualcam](https://github.com/letmaik/pyvirtualcam)
* [obs-studio](https://github.com/obsproject/obs-studio)

The modified code of obs-studio is available in the forked repository [obs-studio-fork](https://github.com/FFGustaf/obs-studio-fork)

## Future Improvements
* Implement codec conversion like in pyvirtualcam
* Specify camera params in camera_name.txt file

