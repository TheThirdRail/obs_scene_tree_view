# Releases and CI/CD

This project ships cross‑platform ZIPs via GitHub Actions. Pushing a tag like `v0.1.6` triggers builds for Windows, macOS, and Linux, packages system‑level folder layouts, and publishes a GitHub Release with notes.

## How to cut a release

1. Ensure `master` is green and ready.
2. Update version numbers (if any) in CMake/README as needed.
3. Create and push a tag:

   ```bash
   git tag v0.1.6 -m "obs_scene_tree_view v0.1.6"
   git push origin v0.1.6
   ```

4. GitHub Actions will:
   - Build on Windows, macOS, Linux (RelWithDebInfo)
   - Package ZIPs with system-level paths
   - Create a GitHub Release and attach ZIP assets
   - Auto‑generate release notes from commits

## ZIP contents (system‑level layouts)

- Windows (unzip into `C:\\Program Files`):
  - `obs-studio/obs-plugins/64bit/obs_scene_tree_view.dll`
  - `obs-studio/obs-plugins/64bit/obs_scene_tree_view.pdb` (debug symbols)
  - `obs-studio/data/obs-plugins/obs_scene_tree_view/locale/*.ini`

- macOS (unzip into `/`):
  - `Library/Application Support/obs-studio/plugins/obs_scene_tree_view.plugin/Contents/MacOS/obs_scene_tree_view`
  - `Library/Application Support/obs-studio/plugins/obs_scene_tree_view/locale/*.ini`

- Linux (unzip into `/`):
  - `usr/lib/obs-plugins/obs_scene_tree_view.so`
  - `usr/share/obs/obs-plugins/obs_scene_tree_view/locale/*.ini`

## Release notes

The workflow uses GitHub’s auto‑generated release notes and includes install instructions inside each ZIP (INSTALL.txt). If you maintain a `CHANGELOG.md`, GitHub will include the relevant section when using annotated tags.

## Troubleshooting CI

- Dependency discovery (libobs/Qt) may vary across runners. If a platform fails to configure:
  - Linux: ensure `libobs-dev` and Qt6 dev packages are present; consider pinning Ubuntu version in the workflow.
  - macOS: Homebrew paths may require exporting `CMAKE_PREFIX_PATH` to include `$(brew --prefix qt)/lib/cmake`.
  - Windows: Qt is installed via `install-qt-action`. If `libobs` isn’t discovered, we may need to fetch an OBS SDK/deps bundle and set `CMAKE_PREFIX_PATH` accordingly.

Open an issue with the failing job logs and we’ll adjust the workflow.

