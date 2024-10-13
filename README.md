
# Adventure Game with Tux Controller Integration

## Overview

This project is an extension of an adventure game that integrates graphical features and a serial port device (Tux Controller) into a game framework. The goal of this project is to enhance the game by improving the graphics, controlling a status bar, and enabling a Tux Controller for game input. The project spans across kernel and user space, requiring modifications to both.

Key skills covered include:
- Device interaction
- Data format conversion and manipulation
- Thread synchronization with `pthread` and mutexes
- VGA graphics and color palette manipulation

## Project Structure

The source tree for this project includes several files that manage different aspects of the game:
- **`adventure.c`**: Main game logic, event loop, input processing, and display control.
- **`input.c``**: Handles user inputs, including keyboard and Tux Controller inputs.
- **`modex.c`**: Manages VGA mode X for graphics, including double-buffering and screen updates.
- **`photo.c`**: Supports drawing vertical and horizontal lines, along with loading images.
- **`text.c`**: Converts ASCII text into pixelized images for display.
- **`world.c`**: Defines the game world, objects, and player interactions.
- **`tuxctl-ioctl.c`**: Kernel module that manages communication with the Tux Controller.
- **`mp2photo.c`**: Converts BMP images to game photos.
- **`mp2object.c`**: Converts BMP images to game objects.

## Key Features

- **Device Interaction**: Control the game using a Tux Controller connected through a serial port.
- **Graphics Manipulation**: Modify images for display using VGA mode X, leveraging a double-buffered strategy.
- **Text to Graphics Conversion**: Convert text into graphical images and display them on the status bar.
- **Game Controller Integration**: Integrate Tux Controller buttons and LEDs into the game, alongside keyboard input.
- **Color Optimization**: Implement a color quantization algorithm to map high-quality colors from images into VGA’s 256-color palette using octrees.

## Getting Started

### Prerequisites

- **Linux Kernel (QEMU)**: Set up the development environment to run and test the game in QEMU.
- **Git**: The project is managed using Git, ensure your repository is properly set up for version control.

### Setup

1. Clone the repository:
   ```bash
   git clone https://gitlab.engr.illinois.edu/ece391_fa22/mp2_<YOUR_NETID>.git mp2
   ```

2. Download the required module for Tux Controller integration:
   ```bash
   wget https://courses.grainger.illinois.edu/ece391/fa2022/secure/assignments/mp/mp2/phototour/module.zip
   unzip module.zip -d module/
   ```

3. To compile the game and module, run:
   ```bash
   make
   ```

4. For testing, ensure QEMU is set up to communicate with the Tux Controller via serial port, and load the driver:
   ```bash
   sudo /sbin/insmod ./module/tuxctl.ko
   ```

### Running the Game

To run the game with QEMU and the Tux Controller:
```bash
./adventure
```

To reset the display to text mode in case of a crash or failure:
```bash
./tr
```

## Key Components

### Mode X Graphics
The game operates in VGA **mode X**, a 256-color graphics mode with 320x200 resolution. This mode provides a powerful technique for double-buffering, allowing smooth visual transitions by switching between video memory pages. 

### Tux Controller Integration
The Tux Controller provides a physical input device for controlling the game. You will implement the necessary drivers in the kernel module to handle button inputs, LED controls, and managing the Tux Controller's communication protocol.

### IOCTLs for Tux Controller
The driver includes several ioctl commands to interact with the Tux Controller:
- **TUX_INIT**: Initializes the Tux Controller.
- **TUX_SET_LED**: Controls the 7-segment LED displays.
- **TUX_BUTTONS**: Reads the button states from the Tux Controller.

### Status Bar and Text Graphics
The game features a status bar at the bottom of the screen. You will implement text-to-graphics conversion to display messages, player inputs, and other information on the status bar.

## Compilation & Debugging

### Compiling the Game
To compile the game and test harness:
```bash
make
```

### Kernel Module Debugging
Use `printk()` in the kernel module to debug driver-related issues. For GDB debugging of the module:
```bash
cat /proc/modules  # Find the loaded module address
gdb ./module/tuxctl.o
add-symbol-file ./module/tuxctl.o <address_from_proc_modules>
```

### Debugging VGA Display
To restore the display after a crash:
```bash
./tr
```