# Pixl Engine 0.1.1



Simple instructions to build Pixl Engine 0.1.1 on Windows and Linux.

---

## Building

### Windows

Open a terminal in the project folder and run:

```bash
    make windows_nt
```


### Linux

Open a terminal in the project folder and run:

```bash
    make linux64
```

Before building on Linux, install the required libraries:

Arch Linux:

```bash
    sudo pacman -S base-devel glfw mesa
```

Debian / Ubuntu:

```bash
    sudo apt update
    sudo apt install build-essential libglfw3-dev libgl1-mesa-dev libglu1-mesa-dev
```

Fedora:

```bash
    sudo dnf install gcc-c++ glfw-devel mesa-libGL-devel
```