# Hack client installs

Arsenal adds one-click Minecraft clients and a few Fabric utilities on top of Prism’s instance creator. Download URLs and “what is in scope” live in [resources/hackclients/README.md](../../resources/hackclients/README.md). This page is the code map: where a new client is registered, which task actually creates the instance, and the constraints that differ per client.

## Two install shapes

**New Instance** tasks subclass `InstanceCreationTask` and write only into `m_stagingPath`. `InstanceList::wrapInstanceTask` runs them under `InstanceStaging`:

- Success commits the staging directory into the instances folder (with backoff retries; Windows antivirus is why).
- Failure or abort deletes the staging directory (`InstanceStaging::childFailed` / `childAborted`).
- `createInstance()` should call `setError` with the real reason and return `false`. The dialog the user sees is the generic string from `InstanceCreationTask::executeTask` (“Error while creating new instance.”). The `setError` text is only `qWarning`’d. Read the log when an install fails.

**Mods-tab** tasks subclass `Task` and write into an instance that already exists. A failed download must not delete jars that were already there. `BaritoneModInstallTask` and `AnarchyUtilityInstallTask` download to a temp file and replace the destination only after the download succeeds.

The loader Mods page is the only page that offers those actions. `InstancePageProvider` constructs it as `ModFolderPage(..., true, true, true)`. `CoreModFolderPage` and `NilModFolderPage` leave the flags at their defaults, so they do not show Install Baritone, Anarchy Utils, or Meteor Addons.

## New Instance UI

`NewInstanceDialog` builds one shared `HackClientsPageContext` and one `HackClientsPage` per category (`hackclients-anarchy`, `hackclients-pvp`, `hackclients-utility`). The context owns every provider. Opening a page calls `refresh(category, false)`, which fetches only the clients in that category. The Refresh button passes `force = true`.

Providers cache in memory: 6 hours, except Impact (`http://impactclient.net/releases.json`) at 24 hours. A refresh that returns while `m_job` is already set does nothing. `force` still clears that client’s “ok” status first, so the page stays on “loading…” until the in-flight job emits `refreshed` or `failed`.

`suggestCurrent()` is what enables OK. It constructs the install task and calls `setSuggestedPack` / `setSuggestedIcon` / `setSuggestedGroup`. If the provider is not ok, or a client that needs a version has an empty combo, it calls `setSuggestedPack()` with no task and OK stays disabled.

LiquidBounce is the exception: `clientNeedsVersion` is false, the combo stays disabled, and the task always uses `LiquidBounceProvider::latestRelease()`.

## Catalog membership

`HackClientCatalog.cpp` `orderByCategory()` is the only membership list. `clientMeta()` supplies the display name and icon key. A client missing from `clientMeta()` is skipped. Ids are the `HackClientId` enum in `HackClientTypes.h` (`ClientNone` is 0 and must stay unused).

| Category | Page id | Client ids, in order |
| --- | --- | --- |
| Anarchy | `hackclients-anarchy` | Meteor, Lambda, Impact, LiquidBounce, Wurst, Epsilon |
| PvP | `hackclients-pvp` | LiquidBounce, Wurst, Epsilon, FDP |
| Utility | `hackclients-utility` | Baritone, Meteor, Lambda, Wurst, LiquidBounce, Epsilon |

Impact, FDP, and Baritone each appear on one page. The same client on two pages shares one provider in the dialog context.

## Adding a client

1. Add a `HackClientId` and put it in `orderByCategory()` and `clientMeta()`.
2. Add a provider (`refresh` / `refreshed` / `failed`, same shape as `MeteorProvider`) and an `InstanceCreationTask`.
3. List both `.h` and `.cpp` in `launcher/CMakeLists.txt`. Sources are not globbed.
4. In `HackClientsPage`: construct and `bind` the provider, refresh it from `HackClientsPageContext::refresh`, and add branches in `suggestCurrent`, `populateVersionCombo`, `clientNeedsVersion`, and `statusLineForKind`.
5. Instance name, icon, and group come from the dialog (`name()`, `m_instIcon`). The task applies them with `inst.setName` / `setIconKey` before `saveNow()`.

Creation tasks that build a Fabric or Forge profile themselves follow the same sequence: `INISettingsObject` at `m_stagingPath/instance.cfg`, `buildingFromScratch()`, `setComponentVersion("net.minecraft", …, true)`, then the loader component, then downloads under `gameRoot()/mods`.

## Per-client create path

| Client | Loader version | What must succeed |
| --- | --- | --- |
| LiquidBounce | Version on the build (`fabricLoaderVersion`) | Queue page yields `filePid`. ZIP from `api.liquidbounce.net/api/v3/file/{filePid}` is SHA-256 checked when the build has a checksum, then every `.jar` inside is copied into `mods/`. Fabric API and Fabric Language Kotlin are separate Maven downloads when those version fields are set. |
| Meteor | `resolveRecommendedFabricLoader()` (launcher meta recommended, else first entry) | Meteor JAR from `meteorclient.com/api/download?version=`. Meteor Baritone (`downloadBaritone`) is optional: a failed download deletes that partial file and the instance still succeeds. |
| Lambda | `fabricLoaderVersion` from the tag’s `gradle.properties` on `lambda-client/lambda` | Keys `fabricLoaderVersion`, `fabricApiVersion`, `kotlinFabricVersion`, `kotlinVersion` are required. `minecraftVersion` in that file overrides the tag parse. `+` in the tag is percent-encoded. Baritone via `BaritoneMaven::downloadBaritoneForMinecraft` is required; a Baritone failure fails the instance. |
| Impact | Whatever the upstream installer writes | Installer `0.9.5` is cached in the metacache as `hackclients/ImpactInstaller-0.9.5.jar` and reused when the file is larger than 100000 bytes. It is run with `JavaPath` (or `java`) as `--no-gui -m MultiMC -i {tag} --mmc-dir {temp} --no-ga`. The instance folder under that temp MultiMC root is copied into staging. |
| Baritone (new instance) | Recommended Fabric Loader | JAR is downloaded straight into `mods/baritone-fabric-{mc}.jar` (no `.part` file; staging is discarded on failure). |
| FDP | Hardcoded Forge `11.15.1.2318` on Minecraft `1.8.9` | GitHub release JAR only. The Minecraft version on the release struct is not what the profile uses. |
| Wurst | `loader_version` from `Wurst-Imperium/Wurst7` `gradle.properties` for the `Wurst-MCX2` tag | Also requires `fabric_api_version`. `minecraft_version` overrides the JAR parse. Optional “Also install Baritone” is fatal if that download fails. Property names are snake_case here and camelCase for Lambda. |
| Epsilon | Recommended Fabric Loader | Combo data is `selectionKey()`: the tag, a pipe character, then the Minecraft version. One GitHub release can ship several Fabric JARs. Fabric API comes from Modrinth for that Minecraft version. Asset names containing `neoforge`, `-sources`, or `syink` are skipped. GitHub page size is 40. |

`resolveRecommendedFabricLoader()` loads the meta index online when it is not loaded, then reads `net.fabricmc.fabric-loader`. Meteor, Baritone, and Epsilon all share that one recommended loader. It is not taken from the client’s own build files.

## Baritone resolution

`BaritoneMaven` lists `https://maven.2b2t.vc/releases/com/github/rfresh2/baritone-fabric/`. For a Minecraft version that has both a release directory and a `-SNAPSHOT` directory, the snapshot directory wins. Download then reads `maven-metadata.xml` for the snapshot jar name and falls back to `baritone-fabric-{mc}.jar` and `baritone-fabric-{mavenVersion}.jar`.

If Maven has no build, or the Maven download fails, `downloadBaritoneForMinecraft` tries the cabaletta/baritone standalone Fabric jar. The mapping is fixed in `officialFabricBuildVersions()` (1.16.5 through 1.20.1). Versions outside both the Maven index and that map fail with a message that lists supported versions.

Mods-tab **Install Baritone** (`BaritoneModInstallTask`) is Fabric-only and disabled while the instance is running. If `modsFolderHasMeteorClient` is true (a `*.jar` whose name contains `meteor-client` or `meteor_client`), it downloads Meteor’s fork to `baritone-meteor-{mc}.jar` instead of the standalone Fabric jar. The file lands as `{dest}.part` and is renamed only after the download succeeds; `replaceOtherBaritoneJars` then deletes other `*baritone*.jar` files. A failed download removes the `.part` file and leaves the previous jars in place.

## Mods-tab utilities

**Anarchy Utils** (`AnarchyUtilsDialog` → `resolveAndInstallAnarchyUtilities`) is Fabric-only. The catalog is `AnarchyUtilityCatalog::all()`:

| Id | Source |
| --- | --- |
| `litematica` | Modrinth slug `litematica`, jar hint `litematica-fabric` |
| `litematica-printer` | Modrinth slug `litematica-printer` |
| `arsenal-nameprotect` | GitHub `Thravok/Arsenal-Name-Protect` |
| `viafabricplus` | Modrinth |
| `bobby` | Modrinth |
| `seedcrackerx` | GitHub `19MisterX98/SeedcrackerX`, hints `seedcrackerx` and `seedcrackerX` |

`ModrinthModDependencyResolver` walks Modrinth `required` dependencies and lists dependencies before the mods that need them. `fileMatchesJarHint` matches `{hint}.jar` or `{hint}-` followed by a loader name or a digit. Hints must not be prefixes of each other: `litematica` would also match `litematica-printer`.

The Mods-tab path shows `ReviewMessageBox` (`requireReview` defaults to true). Exact filename matches start unchecked. GitHub lookups use `releases?per_page=30`, prefer an asset whose name contains `-mc{version}`, then accept any jar that matches the hints. Modrinth `.index` metadata is written only when `projectId` does not contain `/`. GitHub entries use `owner/repo` as `projectId`, so that write is skipped and Check for Updates does not see them. A failed index write is non-fatal; the jar stays installed.

**Meteor Addons** is shown only when the instance is Fabric and `modsFolderHasMeteorClient` is true. The catalog is `https://raw.githubusercontent.com/cqb13/meteor-addon-scanner/refs/heads/addons/addons.json` (6 hour cache). `MeteorAddonEntry::pickJarDownloadUrl` prefers `latestReleaseUrl` when it is a `.jar` whose name does not contain `-dev.jar` or `-sources.jar`, otherwise the first such entry in `downloadUrls`. `MeteorAddonInstallTask` writes that URL directly into `mods/`.

## Name Protect after create

`MainWindow::addInstance` reads `NewInstanceDialog::installArsenalUtilities()` (checkbox, on by default) and, after the creation task succeeds, calls `HackClients::installArsenalUtilities` on the committed instance. That runs only for Fabric. Non-Fabric returns true and does nothing. It installs utility id `arsenal-nameprotect` with `requireReview = false`. A failure shows a warning and does not delete the new instance. Copy-instance does not call this function.
