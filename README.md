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

## Prerequisites (Windows)

- Windows 10/11 (64-bit)
- OBS Studio 32.0.1+ installed (runtime)
- CMake 3.28+
- Visual Studio 2022 (MSVC, v143) with Desktop development with C++
- Qt 6.8.3 EXACT (provided by the OBS deps pack below)
- Git
- OBS source and dependency pack (SDK paths for headers/libs):
  - OBS source: `C:\OBS-SDK\obs-studio-src`
  - OBS deps (Qt 6.8.3 pack): `C:\OBS-SDK\obs-studio-src\.deps\obs-deps-qt6-2025-08-23-x64`

Notes:
- Qt version must match the OBS runtime’s Qt exactly. For OBS 32.0.1, use Qt 6.8.3.
- Mixing Qt versions (e.g., building against 6.9.x while running OBS with 6.8.3) will prevent the plugin from loading.

## Installation from Release ZIP (Windows)

For end users, the easiest way to install is with a pre-built release ZIP that contains only the plugin binaries.

1) Download the latest release ZIP from this repository's GitHub Releases page.
   - The ZIP includes: `obs_scene_tree_view.dll` and `obs_scene_tree_view.pdb`
2) Close OBS Studio if it is running.
3) Extract the ZIP to a temporary folder.
4) Copy the following files to the OBS 64-bit plugins folder:
   - `obs_scene_tree_view.dll`
   - `obs_scene_tree_view.pdb`
   - Destination: `C:\Program Files\obs-studio\obs-plugins\64bit\`
   - Note: Copying into `Program Files` may require administrator privileges.
     - Example (PowerShell, run as Administrator):
       ```powershell
       $dest = "C:\Program Files\obs-studio\obs-plugins\64bit"
       Copy-Item ".\obs_scene_tree_view.dll" $dest -Force
       Copy-Item ".\obs_scene_tree_view.pdb" $dest -Force
       ```
5) Launch OBS Studio and verify the plugin loaded:
   - View → Docks → Scene Tree View (check it)
   - If it doesn't appear: View → Docks → Reset UI, then re-check the dock
6) If OBS was open during the copy, restart OBS to load the new plugin binaries.

## Installation (Windows)

After building, install the plugin into the system OBS installation:

1) Close OBS Studio
2) Copy the built DLL (v0.1.5) to the 64-bit plugins folder:
   - From: `d:\Coding\obs-plugins\obs_scene_tree_view\build_qt683\RelWithDebInfo\obs_scene_tree_view.dll`
   - To:   `C:\Program Files\obs-studio\obs-plugins\64bit\obs_scene_tree_view.dll`
3) Copy locale files (for translated titles/strings):
   - From: `d:\Coding\obs-plugins\obs_scene_tree_view\data\locale\`
   - To:   `C:\Program Files\obs-studio\data\obs-plugins\obs_scene_tree_view\locale\`
   - Example (PowerShell, run as Admin):
     ```powershell
     robocopy "data\locale" "C:\Program Files\obs-studio\data\obs-plugins\obs_scene_tree_view\locale" /E
     ```
4) Launch OBS Studio and enable the dock:
   - View → Docks → Scene Tree View (check it)
5) If you don’t see it immediately, use View → Docks → Reset UI once, then re-check the dock.

## Building from Source

### Windows

```powershell
# Paths (adjust if different)
$env:OBS_SRC  = "C:\OBS-SDK\obs-studio-src"
$env:OBS_DEPS = "$env:OBS_SRC\.deps\obs-deps-qt6-2025-08-23-x64"
$env:Qt6_DIR  = "$env:OBS_DEPS\qt6\lib\cmake\Qt6"
$env:CMAKE_PREFIX_PATH = "$env:OBS_DEPS;$env:OBS_DEPS\qt6;$env:OBS_DEPS\obs-studio;$env:OBS_DEPS\obs-studio\lib\cmake"

# Configure (Visual Studio 2022, x64)
cmake -S . -B build_qt683 -G "Visual Studio 17 2022" -A x64 `
  -DQt6_DIR="$env:Qt6_DIR" `
  -DCMAKE_PREFIX_PATH="$env:CMAKE_PREFIX_PATH"

# Build (choose one configuration)
cmake --build build_qt683 --config RelWithDebInfo -j 8
cmake --build build_qt683 --config Release       -j 8
cmake --build build_qt683 --config Debug         -j 8
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

#### Reordering with Move Up/Down buttons
- Select a scene or folder, then click Move Up or Move Down to move it exactly one position.
- The selection stays on the moved item after the move.
- When the selected item is at the top, Up is disabled; when it is at the bottom, Down is disabled.
- Disabled icons retain their normal color (non-dimmed) to keep the UI visually stable; only enablement changes.


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
1. Verify the DLL is installed to the system OBS folder (not AppData):
   - `C:\Program Files\obs-studio\obs-plugins\64bit\obs_scene_tree_view.dll`
   - Remove any older copies from `%APPDATA%\obs-studio\plugins\...` that could shadow the system plugin.
2. In OBS, enable the dock:
   - View → Docks → Scene Tree View (check it)
   - If missing: View → Docks → Reset UI, then re-check the dock entry.
3. Check OBS logs for clues (Help → Log Files):
   - Look for lines containing `obs_scene_tree_view` and `registered via`.
4. Ensure OBS Studio is 32.0.1+ (Help → About OBS Studio).

### Build Errors

**Problem**: CMake cannot find OBS libraries (libobs/obs-frontend-api)

**Solutions (Windows)**:
1. Ensure you set these before configuring:
   - `$env:OBS_SRC = "C:\OBS-SDK\obs-studio-src"`
   - `$env:OBS_DEPS = "$env:OBS_SRC\.deps\obs-deps-qt6-2025-08-23-x64"`
   - `$env:CMAKE_PREFIX_PATH = "$env:OBS_DEPS;$env:OBS_DEPS\qt6;$env:OBS_DEPS\obs-studio;$env:OBS_DEPS\obs-studio\lib\cmake"`
2. Re-run CMake configure (see Windows build section).
3. If still failing, verify the deps pack exists and contains `lib/cmake/libobs` and `lib/cmake/obs-frontend-api`.

**Problem**: Qt version mismatch (plugin loads fails or dock missing without clear error)

**Solutions**:
1. OBS 32.0.1 uses Qt 6.8.3. Build the plugin against Qt 6.8.3 exactly (from the obs-deps pack).
2. Confirm CMake is using `-DQt6_DIR="C:\OBS-SDK\obs-studio-src\.deps\obs-deps-qt6-2025-08-23-x64\qt6\lib\cmake\Qt6"`.

**Problem**: "C++17 or later required" error

**Solutions**:
1. Use Visual Studio 2022 (v143) and CMake 3.28+.
2. Ensure your Kit/Generator is "Visual Studio 17 2022" and `-A x64`.

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

