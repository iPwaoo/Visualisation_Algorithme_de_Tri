# Sorting Visualization Project

This project aims to **visualize the different steps of sorting algorithms** in real time using **SDL2**.  
It allows you to graphically observe how various sorting algorithms work through a simple and interactive interface.

---

## 🧰 Requirements

Before compiling the project, make sure the following libraries are installed:

- **SDL2**
- **SDL2_ttf**
- **SDL2_image**

On Ubuntu/Debian, you can install them with:

```bash
sudo apt install libsdl2-dev libsdl2-ttf-dev libsdl2-image-dev
```

---

## 🗂️ Project Structure

Project folder organization:

```
.
├── main.c               # Entry point of the program
├── Makefile             # Compilation file
├── ressources/          # Contains images used by the program
├── out/                 # Folder where compiled executables are stored
├── police.otf           # Font file used for text rendering
├── main (executable / when compiled)    # Main executable file located at the project root
└── other .c/.h files for modules (visual, stats, utils, etc.)
```

---

## ⚙️ Compilation and Execution

The project uses a **Makefile** to simplify the compilation process.

### 🔨 Compile
```bash
make
```
> Automatically compiles all source files and places the executables in `out/`.

### 🧹 Clean
```bash
make clean
```
> Deletes all executables and removes the `out/` folder.

### ▶️ Run
Once compiled, simply run:
```bash
./main
```

#### 🖼️ Use a Custom Background Image

You can also run the program with a **custom background image**.  
Just place your image (either `.png` or `.jpeg`) inside the `ressources/` folder, then run:

```bash
./main image.png
```
or
```bash
./main image.jpeg
```

> ⚠️ The image **must** be located in the `ressources/` folder to be displayed.

---

## 🎮 Features and Controls

The program allows you to **visualize different sorting algorithms** and interact with them in real time.

### 🧩 Main Features
- Generates a **random array** of values
- Step-by-step visualization of sorting algorithms
- Adjustable array size and animation speed
- Simple SDL2-based interface

### ⌨️ Keyboard Controls

| Key | Action |
|-----|---------|
| **Space** | Pause / Resume sorting animation |
| **B** | Start **Bubble Sort** |
| **S** | Start **Selection Sort** |
| **I** | Start **Insertion Sort** |
| **Q** | Start **Quick Sort** |
| **M** | Start **Merge Sort** |
| **R** | Generate a **new random array** |
| **↑ / ↓** | **Increase / Decrease** array size |
| **Escape** | Close the window and exit the program |

---

## 💡 Notes

- All resources (images, fonts, etc.) should be placed in the `ressources/` directory.
- The `main` executable is located in the project root after compilation.
- The project is designed for ease of use: a single `make` command is enough to build everything.

---

## 🧑‍💻 Author

Matteo Bigot & Quentin Bouetel

Project created as part of a C programming exercise for sorting algorithm visualization using SDL2.

## © Copyright

© 2025 Matteo Bigot & Quentin Bouetel.  
All rights reserved.


