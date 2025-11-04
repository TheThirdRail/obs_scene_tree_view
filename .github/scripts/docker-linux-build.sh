#!/usr/bin/env bash
set -euo pipefail
export DEBIAN_FRONTEND=noninteractive
export CI=1

# Basic tools required before running the zsh-based build script
apt-get update
apt-get install -y sudo zsh software-properties-common ca-certificates gnupg jq zip wget

# Install modern CMake (>=3.25) from Kitware APT to support CMakePresets.json
if ! command -v cmake >/dev/null 2>&1 || ! cmake --version | awk 'NR==1{exit ($3<3.25)}'; then
  echo "Installing modern CMake from Kitware APT..."
  wget -O - https://apt.kitware.com/keys/kitware-archive-latest.asc | gpg --dearmor -o /usr/share/keyrings/kitware-archive-keyring.gpg
  CODENAME="$(. /etc/os-release && echo ${VERSION_CODENAME:-noble})"
  echo "deb [signed-by=/usr/share/keyrings/kitware-archive-keyring.gpg] https://apt.kitware.com/ubuntu/ ${CODENAME} main" > /etc/apt/sources.list.d/kitware.list
  apt-get update
  apt-get install -y cmake
fi

# Show versions for logs
zsh --version || true
jq --version || true
cmake --version || true

# Run the vendored Ubuntu build (will add OBS PPA, install deps, and build/install into release/RelWithDebInfo)
zsh .github/scripts/build-ubuntu --config RelWithDebInfo

# Package into the exact structure requested
ver=$(jq -r '.version' buildspec.json)
rm -rf stage
mkdir -p stage/usr/lib/obs-plugins
mkdir -p "stage/usr/share/obs/obs-plugins/obs_scene_tree_view/locale"
cp release/RelWithDebInfo/lib/obs-plugins/obs_scene_tree_view.so stage/usr/lib/obs-plugins/
cp release/RelWithDebInfo/share/obs/obs-plugins/obs_scene_tree_view/locale/*.ini "stage/usr/share/obs/obs-plugins/obs_scene_tree_view/locale/"
(
  cd stage
  zip -r "../obs_scene_tree_view-${ver}-linux-x64.zip" usr
)
ls -lah "obs_scene_tree_view-${ver}-linux-x64.zip"

