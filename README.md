# Scene Tree View Plugin for OBS Studio

A powerful OBS Studio plugin that adds a hierarchical scene tree view dock, enabling efficient scene and folder management with drag-and-drop support.

![Screenshot](images/obs_scene_tree_view_example.png)

## Features

- **Hierarchical Scene Organization**: Organize scenes into folders for better project structure
- **Drag-and-Drop Support**: Easily reorder scenes and folders with intuitive drag-and-drop
- **Scene Management**: Add, remove, rename, and manage scenes directly from the tree view
- **Folder Support**: Create and organize scenes into logical groups
- **Per-Scene Transitions**: Configure custom transitions for individual scenes
- **Scene Collection Support**: Automatically saves and restores scene tree structure with scene collections
- **Cross-Platform**: Works on Windows, macOS, and Linux

## Requirements

### Minimum Versions
- **OBS Studio**: 32.0.0 or later
- **Qt**: 6.x (included with OBS 32+)
- **CMake**: 3.25 or later
- **C++ Standard**: C++17 or later

### Platform-Specific Requirements

#### Windows
- Visual Studio 2019 or later (with C++ workload)
- OBS Studio 32 SDK

#### macOS
- Xcode 13 or later
- OBS Studio 32 SDK
- Supports both Intel (x86_64) and Apple Silicon (arm64)

#### Linux
- GCC 9+ or Clang 10+
- OBS Studio 32 development headers
- Qt 6 development headers

## Installation

### Pre-built Binaries

Visit the [Releases](https://github.com/DigitOtter/obs_scene_tree_view/releases) page to download pre-built binaries for your platform.

#### Windows
1. Download the latest Windows release
2. Extract the contents to your OBS plugins directory:
   - `%APPDATA%\obs-studio\plugins\obs_scene_tree_view\bin\64bit\`
3. Restart OBS Studio
4. Enable the plugin in OBS: Tools → Plugins → Scene Tree View

#### macOS
1. Download the latest macOS release
2. Extract the contents to your OBS plugins directory:
   - `~/Library/Application Support/obs-studio/plugins/obs_scene_tree_view/bin/`
3. Restart OBS Studio
4. Enable the plugin in OBS: Tools → Plugins → Scene Tree View

#### Linux

##### Arch Linux
Available via the `obs-scene-tree-view-git` AUR package:
```bash
pikaur -S obs-scene-tree-view-git
```

##### Other Distributions
1. Download the latest Linux release
2. Extract to your OBS plugins directory:
   - `~/.config/obs-studio/plugins/obs_scene_tree_view/lib/`
3. Restart OBS Studio
4. Enable the plugin in OBS: Tools → Plugins → Scene Tree View

## Building from Source

### Windows

```powershell
# Set up environment variables
$OBS_SDK_DIR = "C:\path\to\obs-studio-32-sdk"

# Create build directory
mkdir build
cd build

# Configure with CMake
cmake -S .. -B . -G "Visual Studio 16 2019" -A x64 `
  -DOBS_SDK_DIR="$OBS_SDK_DIR"

# Build
cmake --build . --config Release

# Install (optional)
cmake --install . --config Release
```

### macOS

```bash
# Set up environment
export OBS_SDK_DIR="/path/to/obs-studio-32-sdk"

# Create build directory
mkdir build
cd build

# Configure for Intel (x86_64)
cmake -S .. -B . -G Ninja \
  -DOBS_SDK_DIR="$OBS_SDK_DIR" \
  -DCMAKE_OSX_ARCHITECTURES="x86_64"

# OR configure for Apple Silicon (arm64)
cmake -S .. -B . -G Ninja \
  -DOBS_SDK_DIR="$OBS_SDK_DIR" \
  -DCMAKE_OSX_ARCHITECTURES="arm64"

# OR configure for Universal (both architectures)
cmake -S .. -B . -G Ninja \
  -DOBS_SDK_DIR="$OBS_SDK_DIR" \
  -DCMAKE_OSX_ARCHITECTURES="x86_64;arm64"

# Build
cmake --build . --config Release

# Install (optional)
cmake --install . --config Release
```

### Linux

```bash
# Set up environment
export OBS_SDK_DIR="/path/to/obs-studio-32-sdk"

# Install dependencies
# Arch Linux
sudo pacman -S obs-studio cmake ninja qt6-base

# Ubuntu/Debian
sudo apt-get install obs-studio libobs-dev cmake ninja-build qt6-base-dev

# Fedora
sudo dnf install obs-studio obs-devel cmake ninja-build qt6-base-devel

# Create build directory
mkdir build
cd build

# Configure
cmake -S .. -B . -G Ninja \
  -DOBS_SDK_DIR="$OBS_SDK_DIR" \
  -DCMAKE_BUILD_TYPE=Release

# Build
cmake --build . --config Release

# Install
sudo cmake --install . --config Release
```

## Usage

### Accessing the Scene Tree View

1. Open OBS Studio
2. Go to **Docks** menu
3. Select **Scene Tree View**
4. The Scene Tree View dock will appear (typically on the left side)

### Basic Operations

#### Adding Scenes
- Right-click in the Scene Tree View → **New Scene**
- Or use the standard OBS scene creation method

#### Creating Folders
- Right-click in the Scene Tree View → **New Folder**
- Folders help organize related scenes

#### Organizing Scenes
- **Drag and Drop**: Click and drag scenes to reorder or move them into folders
- **Rename**: Right-click a scene/folder → **Rename**
- **Delete**: Right-click a scene/folder → **Delete**

#### Scene Selection
- Click a scene in the tree to select it as the current scene
- Double-click to switch to preview mode (if enabled)

#### Per-Scene Transitions
- Right-click a scene → **Transition** to set a custom transition for that scene

### Keyboard Shortcuts
- **Delete**: Remove selected scene or folder
- **F2**: Rename selected item
- **Drag & Drop**: Reorder scenes and folders

## Troubleshooting

### Plugin Not Appearing in OBS

**Problem**: The Scene Tree View dock doesn't appear in the Docks menu.

**Solutions**:
1. Verify OBS Studio version is 32.0.0 or later:
   - Help → About OBS Studio
2. Check that the plugin is installed in the correct location:
   - Windows: `%APPDATA%\obs-studio\plugins\obs_scene_tree_view\`
   - macOS: `~/Library/Application Support/obs-studio/plugins/obs_scene_tree_view/`
   - Linux: `~/.config/obs-studio/plugins/obs_scene_tree_view/`
3. Check OBS logs for errors:
   - Help → Log Files
4. Restart OBS Studio

### Build Errors

**Problem**: CMake configuration fails with "OBS SDK not found"

**Solutions**:
1. Verify OBS_SDK_DIR environment variable is set correctly
2. Ensure the SDK path contains `include/obs.h`
3. Download the correct OBS 32 SDK from [OBS GitHub Releases](https://github.com/obsproject/obs-studio/releases)

**Problem**: Compilation errors related to Qt6

**Solutions**:
1. Ensure Qt 6 development headers are installed
2. On Linux, install: `qt6-base-dev` (Ubuntu/Debian) or `qt6-base-devel` (Fedora)
3. Verify CMake can find Qt6: `cmake --find-package Qt6`

**Problem**: "C++17 or later required" error

**Solutions**:
1. Update your compiler:
   - Windows: Visual Studio 2019 or later
   - macOS: Xcode 13 or later
   - Linux: GCC 9+ or Clang 10+
2. Verify CMake is using the correct compiler

### Runtime Issues

**Problem**: Plugin crashes when adding/removing scenes

**Solutions**:
1. Update OBS Studio to the latest 32.x version
2. Check OBS logs for specific error messages
3. Try disabling other plugins to isolate the issue
4. Report the issue with logs attached

**Problem**: Scene tree doesn't update when scenes are added externally

**Solutions**:
1. This is expected behavior - refresh by switching scenes
2. Scene tree updates automatically when using the tree view UI
3. Check that the plugin is enabled in Tools → Plugins

## Known Issues

- [ ] Undo/Redo scene rename does not update Scene Tree
- [ ] Add Fullscreen Viewport Projector option to scene context menu

## Contributing

Contributions are welcome! Please follow these guidelines:

1. **Fork** the repository
2. **Create a feature branch**: `git checkout -b feature/your-feature`
3. **Make your changes** and test thoroughly
4. **Commit** with clear, descriptive messages
5. **Push** to your fork
6. **Create a Pull Request** with a detailed description

### Development Setup

1. Clone the repository
2. Follow the "Building from Source" section above
3. Make your changes
4. Test on all supported platforms if possible
5. Submit a pull request

### Code Style

- Follow the existing code style in the repository
- Use `.clang-format` for C++ formatting
- Keep commits atomic and well-documented

## License

This project is licensed under the **GNU General Public License v2.0** - see the [LICENSE](LICENSE) file for details.

## Credits

- Original author: DigitOtter
- OBS Studio: https://obsproject.com
- Qt Framework: https://www.qt.io

## Support

For issues, questions, or suggestions:

1. Check the [Troubleshooting](#troubleshooting) section
2. Search existing [GitHub Issues](https://github.com/DigitOtter/obs_scene_tree_view/issues)
3. Create a new issue with:
   - OBS Studio version
   - Plugin version
   - Operating system and version
   - Detailed description of the problem
   - Steps to reproduce
   - OBS log file (Help → Log Files)

