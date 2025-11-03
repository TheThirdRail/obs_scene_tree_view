#!/usr/bin/env bash
set -euo pipefail

BUILD_DIR=${1:-build}
OUT_DIR=${2:-dist}

STAGE="${OUT_DIR}/obs-scene-tree-view-linux-x86_64"
BIN_DIR="${STAGE}/usr/lib/obs-plugins"
LOCALE_DIR="${STAGE}/usr/share/obs/obs-plugins/obs_scene_tree_view/locale"

rm -rf "${STAGE}" && mkdir -p "${BIN_DIR}" "${LOCALE_DIR}"

# Locate built .so
PLUGIN_SO=$(find "${BUILD_DIR}" -type f -name "obs_scene_tree_view.so" | head -n 1 || true)
if [[ -z "${PLUGIN_SO}" ]]; then
  echo "ERROR: obs_scene_tree_view.so not found under ${BUILD_DIR}" >&2
  exit 1
fi

cp -v "${PLUGIN_SO}" "${BIN_DIR}/"

# Locales
cp -v data/locale/*.ini "${LOCALE_DIR}/"

# INSTALL instructions
cat > "${STAGE}/INSTALL.txt" << 'EOF'
Installation Instructions for Linux (System-Level)
=================================================

1) Close OBS Studio completely.
2) Extract this archive.
3) As root, copy the contents to system directories:
   sudo cp -r usr/lib/obs-plugins/* /usr/lib/obs-plugins/
   sudo cp -r usr/share/obs/* /usr/share/obs/
4) Start OBS Studio.
5) Enable the dock via: View → Docks → Scene Tree View (Reset UI if needed).

Notes:
- This is a system-level install and requires root privileges.
- OBS 32.x is required.
EOF

# Create ZIP
mkdir -p "${OUT_DIR}"
ZIP_PATH="${OUT_DIR}/obs-scene-tree-view-linux-x86_64.zip"
(cd "${OUT_DIR}" && zip -r "$(basename "${ZIP_PATH}")" "$(basename "${STAGE}")")

echo "Packaged: ${ZIP_PATH}"

