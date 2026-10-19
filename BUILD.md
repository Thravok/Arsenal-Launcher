# Building Arsenal Launcher

Arsenal Launcher follows [Prism Launcher](https://prismlauncher.org/wiki/development/build-instructions/) build requirements: **CMake 3.28+**, **Ninja**, **Qt 6.8+**, **C++23**, and dependencies resolved via the bundled **vcpkg** submodule.

## Makefile (recommended locally)

```bash
git submodule update --init --recursive   # or: make submodules
export CMAKE_PREFIX_PATH="$(brew --prefix qt)"   # macOS Homebrew Qt 6
make build
make run
```

`make configure` looks for Homebrew Qt 6 at `$(brew --prefix qt)` first, then `$(brew --prefix qt@6)`. If `brew install qt` fails with a **qt@5 conflict**, run `brew unlink qt@5` first (you can `brew link qt@5` again later for other projects).

Remove any placeholder from your shell config (`/path/to/Qt/...`) — `make configure` will refuse it. In fish: `set -e CMAKE_PREFIX_PATH`.

Useful variables: `BUILD_TYPE=Release`, `MSA_CLIENT_ID=…`, `UNIVERSAL=1` (macOS universal preset), `JOBS=8`. Run `make help` for the full list.

## CMake only

```bash
cmake --preset macos -D VCPKG_HOST_TRIPLET=arm64-osx -D VCPKG_TARGET_TRIPLET=arm64-osx
cmake --build build --config Debug
```

Presets: `linux`, `macos`, `macos_universal`, `windows_msvc`, `windows_mingw` (see [CMakePresets.json](CMakePresets.json)).

Microsoft account login requires your own app registration. CI reads the repository variable `MSA_CLIENT_ID` and passes the same CMake option. See [OPERATIONS.md](OPERATIONS.md) for the OAuth callback, client-id refresh behavior, and updater/Sparkle setup.

```bash
make configure MSA_CLIENT_ID="your-client-id"
```

`make` exports `ARTIFACT_NAME` (default `Local-Qt6`) into `Launcher_BUILD_ARTIFACT`. On Windows and Linux a non-empty artifact id builds `arsenal_updater`. In-place update checks still run only for an exact version tag (`v1.0.2` or `1.0.2`). macOS uses Sparkle instead of that binary.

On macOS, `make run` copies helper jars into `Arsenal.app/Contents/MacOS/jars` so a debug bundle can start. Release packaging installs them under `Contents/Resources/jars`; jars beside the executable fail `codesign`.
