# Arsenal Launcher

Arsenal Launcher is a Minecraft launcher fork based on [Prism Launcher](https://github.com/PrismLauncher/PrismLauncher). It adds a **Hack Clients** tab when creating instances for one-click installs of clients such as LiquidBounce, Meteor, Lambda, Impact, and Baritone, plus optional **The Altening** account support.

This project is **not** affiliated with or endorsed by Prism Launcher, PolyMC, or MultiMC.

## Upstream

- **Git upstream:** `https://github.com/PrismLauncher/PrismLauncher` (`develop`)
- **Game metadata:** `https://meta.prismlauncher.org/v1/`
- **Archive branch:** `archive/polymc-hackermc` (pre-migration PolyMC-era snapshot, tag `polymc-era-hackermc`)

## Build

Requires **Qt 6.8+** and a C++23-capable compiler (same as Prism Launcher). See [BUILD.md](BUILD.md) and Prism’s [build instructions](https://prismlauncher.org/wiki/development/build-instructions/).

```bash
make submodules
export CMAKE_PREFIX_PATH="$(brew --prefix qt@6)"   # adjust for your Qt install
make build
```

Optional Microsoft login: `make configure MSA_CLIENT_ID="your-client-id"`. The binary name is `arsenal` (`Arsenal.app` on macOS).

## Fork policy

- Do not ship Prism’s bundled Microsoft or CurseForge API keys in public builds; this tree defaults them to empty strings.
- Set `Launcher_MSA_CLIENT_ID` at configure time for Microsoft login.

## Hack Clients

See [resources/hackclients/README.md](resources/hackclients/README.md).
