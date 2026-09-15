# Building Arsenal Launcher

Arsenal Launcher follows [Prism Launcher](https://prismlauncher.org/wiki/development/build-instructions/) build requirements: **CMake 3.28+**, **Ninja**, **Qt 6.8+**, **C++23**, and dependencies resolved via the bundled **vcpkg** submodule.

## Makefile (recommended locally)

```bash
git submodule update --init --recursive   # or: make submodules
export CMAKE_PREFIX_PATH="$(brew --prefix qt)"   # macOS Homebrew (Qt 6.8+)

If `brew install qt` fails with a **qt@5 conflict**, run `brew unlink qt@5` first (you can `brew link qt@5` again later for other projects).

Remove any placeholder from your shell config (`/path/to/Qt/...`) — `make configure` will refuse it. In fish: `set -e CMAKE_PREFIX_PATH`.
make build
make run
```

Useful variables: `BUILD_TYPE=Release`, `MSA_CLIENT_ID=…`, `UNIVERSAL=1` (macOS universal preset), `JOBS=8`. Run `make help` for the full list.

## CMake only

```bash
cmake --preset macos -D VCPKG_HOST_TRIPLET=arm64-osx -D VCPKG_TARGET_TRIPLET=arm64-osx
cmake --build build --config Debug
```

Presets: `linux`, `macos`, `macos_universal`, `windows_msvc`, `windows_mingw` (see [CMakePresets.json](CMakePresets.json)).

Microsoft account login requires your own app registration:

```bash
make configure MSA_CLIENT_ID="your-client-id"
```
