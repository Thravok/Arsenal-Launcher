# Arsenal operations

Arsenal-specific behavior that Prism’s docs do not cover: Microsoft login, in-place updates, and The Altening. Hack-client installs stay in [resources/hackclients/README.md](resources/hackclients/README.md). Local builds stay in [BUILD.md](BUILD.md).

## Microsoft account login

Microsoft login is off unless a client ID is present. The tree default is an empty string so public builds do not ship Prism’s registration.

Resolution order in `Application::getMSAClientID()`:

1. Settings override `MSAClientIDOverride` (Settings → Services, Microsoft Authentication).
2. Otherwise the build-time value `Launcher_MSA_CLIENT_ID`.

```bash
make configure MSA_CLIENT_ID="your-client-id"
# equivalent
cmake --preset linux -D Launcher_MSA_CLIENT_ID="your-client-id"
```

CI bakes the repository **variable** `MSA_CLIENT_ID` (a public desktop client ID, not a secret) via `-D Launcher_MSA_CLIENT_ID` in `.github/workflows/build.yml`. An empty variable leaves login disabled (`SupportsMSA` stays off).

### OAuth callback

`MSAStep` uses Microsoft’s consumer endpoints:

- Authorize: `https://login.microsoftonline.com/consumers/oauth2/v2.0/authorize`
- Token: `https://login.microsoftonline.com/consumers/oauth2/v2.0/token`
- Scope: `XboxLive.SignIn XboxLive.offline_access`
- Interactive login adds `prompt=select_account`

The reply handler is a localhost HTTP server when any of these is true: macOS, a portable layout, an AppImage mount (`/tmp/.mount_`), or the custom URL scheme is not registered. macOS always takes this path. The custom scheme `arsenal://oauth/microsoft` is used only on Linux and Windows when the desktop/registry handler points at this install.

`Launcher_LOGIN_CALLBACK_URL` defaults to empty. Empty means the localhost handler serves a self-contained “Login successful” page and does not redirect to Prism. A non-empty value is used as a redirect target. CMake rewrites a cached Prism login URL back to empty on the next configure (`arsenal_migrate_cached_url`).

### Refresh and Xbox errors

A silent refresh is disabled when the stored `msa-client-id` does not match the client ID in use, or when the refresh token is empty. Changing the baked ID or the settings override forces those accounts to sign in again.

Xbox XSTS failures (`https://xsts.auth.xboxlive.com/xsts/authorize`) map known `XErr` values to a user-facing message, including Xbox bans, family restrictions, and unsigned Xbox terms. The Minecraft entitlements call sends a `requestId` and rejects a response whose `requestId` / `entitlementsRequestId` does not match. Signature fields that look like JWTs are checked for structure only; they are not cryptographically verified.

## Updates

In-place update checks run only when `BuildConfig::isPublicRelease()` is true: `GIT_TAG` equals `major.minor.patch` or `v` plus that version. Branch and CI builds that are not an exact version tag skip the updater. **Help → Update** (also the toolbar **Update** button) then explains that this is a development build and can open `UPDATER_GITHUB_REPO/releases`.

CMake also rewrites cached Prism or HackerMC values for the GitHub repo URL, wiki/help links, and the Sparkle feed so an old build directory does not offer Prism updates. Reconfigure after pulling those changes; do not rely on a stale `CMakeCache.txt`.

### Windows and Linux

The external updater is `arsenal_updater` (`arsenal_updater.exe` on Windows, `bin/arsenal_updater` elsewhere). It is compiled only when `Launcher_BUILD_ARTIFACT` is non-empty. Presets set that from the `ARTIFACT_NAME` environment variable. The Makefile default is `Local-Qt6`. CI sets `ARTIFACT_NAME` to `<matrix artifact-name>-Qt6` (for example `Linux-Qt6`, `Windows-MSVC-Qt6`).

`Application::updaterEnabled()` also requires that binary to exist next to the install. The launcher then runs:

```text
arsenal_updater --check-only --dir <data-dir> --debug [--pre-release]
```

Exit codes:

| Code | Meaning |
| --- | --- |
| 0 | No newer release, or a completed run with nothing to do |
| 1 | Updater error (also: failed to start within 5s, or still running after 60s) |
| 100 | Update available. Stdout is `Name:`, `Version:` (the git tag), `TimeStamp:`, then the release body |

Accepting the update starts `arsenal_updater --dir <data-dir> --install-version <tag>` and quits the launcher. Other flags: `--update-url`, `--list`, `--select-ui`, `--force`, `--allow-downgrade`, `--prism-version`.

Checks read GitHub Releases for `Launcher_UPDATER_GITHUB_REPO` (`https://github.com/Thravok/Arsenal-Launcher`). The latest non-draft release wins by FlexVer of `tag_name`. Pre-releases are included only with `--pre-release`.

Automatic checks use `<data>/arsenal_update.cfg` (INI). If that file is missing and `prismlauncher_update.cfg` exists, the Prism file is copied once.

| Key | Read at startup | Default |
| --- | --- | --- |
| `auto_check` | yes | true |
| `update_interval` | yes, seconds | 86400 (Settings stores hours × 3600; `0` checks on launch) |
| `allow_beta` | yes | false |
| `last_check` | yes, ISO date | unset |
| `skip/<tag>` | yes | skip that tag on automatic checks |

`setBetaAllowed()` writes `auto_beta`, not `allow_beta`. **Update** always calls `setBetaAllowed(false)` first, so a manual check never requests pre-releases. A hand-edited `allow_beta=true` still affects the next automatic check until something calls `setBetaAllowed`.

Logs and markers live under the data directory:

- `logs/arsenal_launcher_update.log` — install log
- `logs/ArsenalUpdater-0.log` (rotated to `-1`, `-2`) — process log; check-only runs use `ArsenalUpdater-CheckOnly-*.log`
- `.arsenal_launcher_update.lock` while an install is in progress (a legacy `.prism_launcher_update.lock` is also removed)
- `.arsenal_launcher_update.success` or `.arsenal_launcher_update.fail` after the unpack step
- `.arsenal_launcher_updater_unpack.marker` (fallback `.prism_launcher_updater_unpack.marker`) tells a freshly unpacked tree where to copy itself

Flatpak can check but refuses to install. AppImage updates delegate to `bin/AppImageUpdate.AppImage` and require `APPIMAGE` to be set. The updater binary itself errors on macOS.

Portable layout is detected from `portable.txt` or a `UserData` directory beside the install (not on macOS). `ARSENAL_DATA_DIR` overrides the data path (`LAUNCHER_NAME` uppercased).

### Release asset names the updater will accept

`validReleaseArtifacts()` keeps a GitHub asset only when all of these hold:

- `BuildConfig.BUILD_ARTIFACT` lowercased, split on the first `-qt`, is a substring of the asset name. `Linux-Qt6` matches names containing `linux`. `Windows-MSVC-arm64-Qt6` matches `windows-msvc-arm64`.
- The name contains `arm64` if and only if `QSysInfo::buildCpuArchitecture()` contains `arm64`. The substring `aarch64` does **not** count, so `Arsenal-Linux-aarch64.*` assets are rejected on ARM Linux and are not excluded on x86_64.
- AppImage installs accept only names ending in `appimage`. Other installs reject those names. `.zsync` files are always skipped.
- Portable installs accept only names containing `portable`. Other installs reject those names.
- A non-portable Windows asset that is `.zip` or `.tar.gz` is rejected (the updater wants the setup `.exe`).
- A `legacy` asset is rejected unless the artifact id itself contains `legacy`.
- If the asset name contains `-qtN`, `N` must match the artifact’s Qt suffix.

Published names (`.github/workflows/release.yml`) that follow those rules: `Arsenal-Linux-x86_64.AppImage` for an x86_64 AppImage whose artifact id is `Linux-Qt6`, `Arsenal-Linux-Qt6-Portable-<tag>.tar.gz` for portable x86_64, and `Arsenal-Windows-MSVC-Setup-<tag>.exe` / `Arsenal-Windows-MSVC-Portable-<tag>.zip` (and the MinGW and `arm64` variants). A normal Windows zip with no `Portable` or `Setup` in the name is not selected. A system install that is neither portable nor an AppImage has no matching Linux asset.

Version comparison uses FlexVer on the raw tag (`v1.0.2` → a leading textual `v` plus numeric sections) against `major.minor.patch` with no `v`. A leading `v` sorts after a digit, so tag `v1.0.2` compares **greater** than the running version `1.0.2`. A public build can therefore report an update while it is already on that release. Tags should stay `v*` (the release workflow triggers on `v*`); do not assume FlexVer equality between the tag and `versionString()`.

### macOS (Sparkle)

macOS does not build `arsenal_updater`. Sparkle is enabled when both the feed URL and the Ed25519 public key are non-empty. Defaults in `CMakeLists.txt`:

- Feed: `https://github.com/Thravok/Arsenal-Launcher/releases/latest/download/appcast.xml` (`MACOSX_SPARKLE_UPDATE_FEED_URL`)
- Public key: `MACOSX_SPARKLE_UPDATE_PUBLIC_KEY` (the Arsenal key in that file). A cached Prism public key is replaced on configure.

The private key is repository secret `SPARKLE_ED25519_KEY` (contents of `arsenal-sparkle-ed25519.pem`). It is not in git. The macOS package job signs the zip and DMG with Sparkle’s `sign_update` and, when `release-tag` is set (tag push or `workflow_dispatch` / `workflow_call` with a tag), writes `appcast.xml`. That step is `continue-on-error`: a missing key or missing Sparkle tools still uploads the zip and DMG. The release job warns when `appcast.xml` is absent.

Regenerate a feed locally with Sparkle’s `generate_appcast` on `PATH` via `SPARKLE_BIN` (a macOS build tree provides `build/frameworks/Sparkle/bin`):

```bash
./scripts/macos/generate_sparkle_appcast.sh \
  --dmg Arsenal.dmg \
  --tag v1.0.2 \
  --key arsenal-sparkle-ed25519.pem \
  --out appcast.xml
```

The script publishes full DMGs only (`--maximum-deltas 0`, `--maximum-versions 5`). The download prefix is `https://github.com/Thravok/Arsenal-Launcher/releases/download/<tag>/`. The default asset name inside the feed is `Arsenal-macOS-<tag>.dmg`, which matches the file the release workflow uploads.

## The Altening

Optional alt-account support. It is not Microsoft login and it does not use Mojang’s session server as the source of truth for authentication.

### Setup

Settings → The Altening stores the panel API key in `TheAlteningApiKey`. **Verify & Save** calls `GET https://api.thealtening.com/v2/license?key=…` and refuses to leave the page with an unverified new key. Clearing the field deletes the setting. The client never logs the key or alt tokens.

`TheAlteningApi::errorMessageForStatus` maps HTTP 401 to an invalid key and HTTP 403 to a plan that cannot call the API (Basic or Premium; Starter is rejected).

Adding an account runs `AuthFlow` for `AccountType::TheAltening`:

1. `TheAlteningGenerateStep` — license check, then `GET /v2/generate?key=…&info=true`. A response with `"limit": true` reuses today’s alt instead of failing.
2. `TheAlteningYggdrasilLoginStep` — `POST http://authserver.thealtening.com/authenticate` with the alt token as `username`. The generate response’s `password` is sent when present; otherwise the password is `anything`.
3. `TheAlteningProfileStep` — `http://sessionserver.thealtening.com/session/minecraft/profile/<id>`. A 404 falls back once to Mojang’s session server for skin URLs.
4. `GetSkinStep` — head renders use `https://cdn.thealtening.com/skins/head/<id>.png` when the profile has no texture URL.

Refresh skips generate. It authenticates again with the stored alt token (`yggdrasilToken.extra["userName"]`). It does not call Yggdrasil `/refresh`: access tokens do not survive an in-game session, and a failed refresh used to leave the account expired. The alt token itself is what stays valid for the day. An account with no stored token must be generated again.

### Launch

`MinecraftAccount` sets the authlib-injector base to the sentinel `thealtening://local`. `ConfigureAuthlibInjector` downloads `latest.json` and the agent JAR from `https://authlib-injector.yushi.moe/artifact/latest.json` (SHA-256 checked, metacache entry `authlibinjector/`). For the sentinel it starts `TheAlteningAuthlibProxy` on `127.0.0.1` with an ephemeral port, parented to the launch task, and points `-javaagent` at that base URL.

The proxy serves authlib-injector metadata locally and forwards `/authserver/` and `/sessionserver/` to The Altening’s hosts. Extra JVM flags, always set for this path:

- `-Dauthlibinjector.yggdrasil.prefetched=<base64 metadata>`
- `-Dauthlibinjector.profileKey=disabled` — The Altening cannot issue Mojang-signed profile keys. `profileKey=enabled` makes the injector attach a dummy signature that 1.19+ servers reject.
- `-Dauthlibinjector.usernameCheck=disabled`

If the proxy cannot listen, launch fails with “Failed to start local The Altening authlib-injector proxy.”

## macOS app bundle jars

`codesign` treats unsigned `.jar` files next to the main Mach-O as nested code and fails with “code object is not signed at all.” Installed bundles therefore put helper jars in `Arsenal.app/Contents/Resources/jars` (`JARS_DEST_DIR` in `CMakeLists.txt`).

`Application::getJarPath()` searches, in order: the Linux `share/Arsenal` data root, `<root>/jars`, `<applicationDir>/jars`, `../Resources/jars`, and `../jars`. `make run` on Darwin copies `build/jars/*.jar` into `Contents/MacOS/jars` so a local debug bundle still launches. That copy is not the layout a sealed release uses. A package or codesign change should keep jars under `Contents/Resources/jars`.
