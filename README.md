# Image-Denoiser-NEA
This is the repository for my Year-12 Computer Science A Level NEA project.

<img width="1920" height="1140" alt="image" src="https://github.com/user-attachments/assets/0d1bd762-42fc-49ad-8c7f-c824ede3a58d" />

## What does it do?

It is an incredibly simple utility program that allows you to load noisy images, denoise them with a small selection of algorithms, and batch output them into files and folders using built-in file name formatting system.

## Supported Algorithms

The program currently supports only bilateral filter, smooth linear filter, and discrete wavelet transform filter. All with adjustable parameters.

<img width="332" height="137" alt="image" src="https://github.com/user-attachments/assets/3083136e-9bb1-4628-a150-61ea92d2fcaa" />

## Features

- In memory compression for loaded images
- lazy loading at load time
- Zoomable Preview with hover tooltips
- Built in terminal for feedback from the program
- File path formatter
- Infinite zoom and smooth zoom animation
- Save workspace layout config
- Windows scale setting

Dependencies
- [ImGui](https://github.com/ocornut/imgui)
- [tinyfiledialogs](https://github.com/native-toolkit/libtinyfiledialogs)
- [FileReader](https://github.com/boongh/FileReader)
- [glfw](https://www.glfw.org/)
- [stb image](https://github.com/nothings/stb)
- [glad](https://github.com/dav1dde/glad)

