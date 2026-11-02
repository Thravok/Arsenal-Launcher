# Hack clients resources

Impact Installer is **not** vendored in this repository.

`ImpactInstallTask` downloads the official installer on first use:

- Upstream: [Impact Installer 0.9.5](https://github.com/ImpactDevelopment/Installer/releases/download/0.9.5/installer-0.9.5.jar)
- Cached under the launcher metacache base `hackclients/`

LiquidBounce artifacts are fetched from liquidbounce.net / api.liquidbounce.net at install time.

Meteor Client builds are listed from `https://meteorclient.com/api/stats` and JARs (plus optional Baritone) are downloaded from meteorclient.com at install time.

Lambda releases are listed from GitHub (`lambda-client/lambda`). Install pulls companion versions from that tag's `gradle.properties` (Fabric API, Fabric Language Kotlin, Baritone from rfresh2 Maven).

FDPClient releases are listed from GitHub (`SkidderMC/FDPClient`). Install creates Minecraft 1.8.9 with Forge `11.15.1.2318` (per upstream install docs) and downloads the release JAR into `mods/`.

Wurst Client v7 JARs are listed from GitHub (`Wurst-Imperium/Wurst-MCX2` releases). Install pulls Fabric Loader and Fabric API versions from the matching `Wurst-Imperium/Wurst7` tag's `gradle.properties`, then downloads the Wurst JAR and Fabric API into `mods/`.

**Epsilon** — lists Fabric JARs from GitHub (`NekoyaHouse/Epsilon` releases; skips NeoForge / sources / unrelated assets). Creates a Fabric instance with the recommended Fabric Loader, downloads the selected Epsilon Fabric JAR, and resolves Fabric API from Modrinth for that Minecraft version. Upstream development is currently paused; only public release assets are used (no auth / closed downloaders).

**Baritone** — lists Minecraft versions from [rfresh2 Baritone Maven](https://maven.2b2t.vc/releases/com/github/rfresh2/baritone-fabric/) (newer releases) plus official [cabaletta/baritone](https://github.com/cabaletta/baritone/releases) standalone Fabric builds for older versions (e.g. 1.16.5). Creates a Fabric instance and downloads Baritone. Existing Fabric instances can use **Install Baritone** on the Mods tab.

**Create Instance categories** — the New Instance sidebar lists **Anarchy**, **PvP**, and **Utility** as separate pages (`hackclients-anarchy`, `hackclients-pvp`, `hackclients-utility`; Custom and other platforms follow). Membership and display order come only from `orderByCategory()` in `launcher/hackclients/HackClientCatalog.cpp`. A client in more than one list shows on each of those pages.

| Category | Clients (display order) |
| --- | --- |
| Anarchy | Meteor, Lambda, Impact, LiquidBounce, Wurst, Epsilon |
| PvP | LiquidBounce, Wurst, Epsilon, FDPClient |
| Utility | Baritone, Meteor, Lambda, Wurst, LiquidBounce, Epsilon |

Impact is Anarchy-only, FDPClient is PvP-only, and Baritone is Utility-only.

**Not automated here** — paid / closed / auth-gated clients (e.g. RusherHack, Future, CatLean) and unofficial leak mirrors (e.g. Shoreline) are intentionally out of scope. Prefer public GitHub / documented CDN installs only.

**Anarchy Utils** — on Fabric instances, the Mods tab shows **Anarchy Utils**. Catalog entries (`AnarchyUtilityCatalog::all()`):

- **Litematica** + **Litematica Printer** (Modrinth; MaLiLib via required deps)
- **Arsenal NameProtect** ([Thravok/Arsenal-Name-Protect](https://github.com/Thravok/Arsenal-Name-Protect) GitHub Releases) — see [Name Protect](#name-protect) below
- **ViaFabricPlus** (Modrinth) — join other Minecraft protocol versions from a modern Fabric client
- **Bobby** (Modrinth) — client-side chunk cache for render distance beyond the server view-distance
- **SeedcrackerX** ([19MisterX98/SeedcrackerX](https://github.com/19MisterX98/SeedcrackerX) GitHub Releases) — client-side world-seed / structure finder

The Mods-tab install walks Modrinth `required` dependencies and shows a review dialog (`requireReview`). **New Instance → Install Arsenal NameProtect** does not. Downloads go to `filename.part`, and only after that file exists are older jars whose names match the catalog hint removed and the part renamed into `mods/`. A failed download leaves the previous jar in place.

Jar hints must not be prefixes of each other. `fileMatchesJarHint` treats `hint.jar`, `hint-<loader>-…`, and `hint-<digit>…` as the same mod, and it deliberately does not treat `litematica-printer-…` as Litematica. A new catalog id needs its own hint.

Modrinth installs write `.index` metadata so Check for Updates can see them. GitHub installs (`projectId` of the form `owner/repo`, including NameProtect and SeedcrackerX) skip that index. Lookup uses the first 30 GitHub releases, skips `-sources`, `-dev`, and `-javadoc` assets, and prefers a `.jar` whose name contains `-mc{minecraft version}`. If none of those 30 releases has that token, the first hint-matching jar is used anyway, which can be a build for a different Minecraft version. No match fails the install with “No JAR for Minecraft …”.

## Name Protect

Settings → **Name Protect** (`NameProtectPage`, id `name-protect`) is separate from installing the mod. Online-mode servers still receive the real profile name. In-game filtering and launcher-side name masking are independent:

| Setting key | Default | What it does |
| --- | --- | --- |
| `NameProtectEnabled` | true | `enabled` in the instance JSON the Fabric mod reads |
| `NameProtectManageFromLauncher` | true | On each launch, overwrite `config/arsenal-nameprotect.json` from these settings |
| `NameProtectSelfAlias` | `Hidden` | Alias for the account that is launching. The settings field is capped at 32 characters; an empty field is stored as `Hidden` |
| `NameProtectRules` | `{}` | Extra `"RealName": "Alias"` pairs. Apply refuses anything that is not a JSON object |
| `NameProtectMaskAccounts` | false | Launcher UI only. The toolbar **Hide names** button writes this same key |

`MinecraftInstance::createLaunchTask` calls `NameProtect::writeInstanceConfig` before the rest of the launch steps. With **Manage config from launcher** off, nothing is written and launch continues. A write failure is a `qWarning` only (`NameProtect config sync failed: …`); the game still starts. The file is written even when filtering is off, so the mod can observe `"enabled": false`.

Path: `<instance game directory>/config/arsenal-nameprotect.json`.

```json
{
    "schemaVersion": 1,
    "enabled": true,
    "selfAlias": "Hidden",
    "rules": {
        "ActualPlayerName": "Hidden",
        "FriendName": "Buddy"
    }
}
```

`rules` starts as the stored object. Invalid JSON outside the settings page is ignored and the object starts empty. When the launch session has a non-empty `player_name` and the self alias is non-empty, that name is inserted into `rules` and overwrites any extra rule with the same key. `selfAlias` in the file is the trimmed setting, or `Hidden` when the setting is empty.

**Hide names** / `NameProtectMaskAccounts` does not edit that file. It masks profile names in the accounts menu, lists, and instance picker: an empty name becomes `***`, a one-character name becomes that character plus `***`, and longer names keep the first and last character (`Steve` → `S***e`). Independently, the launch log censor replaces the whole `player_name` with `<PLAYER NAME>` when either filtering or account masking is on.

**New Instance** includes **Install Arsenal NameProtect**, checked by default. After the creation task succeeds, `MainWindow::addInstance` calls `installArsenalUtilities` on the new instance. Non-Fabric loaders return immediately. An unknown Minecraft version shows a warning and skips the download. There is no review dialog. An exact filename already in `mods/` is left alone; a different filename that only matches the `arsenal-nameprotect` hint is treated as “maybe installed” and still replaced after a successful download. Copying an instance does not run this install.

**Meteor addons** — on a Fabric instance that already has a Meteor Client JAR in `mods/`, the Mods tab shows **Meteor Addons**. The catalog is fetched from the community list maintained by [meteor-addon-scanner](https://github.com/cqb13/meteor-addon-scanner) (`addons/addons.json`). Direct release JAR URLs are downloaded into `mods/`. Skip `-dev.jar` and `-sources.jar` assets when picking downloads.

**Wurst + Baritone** — when creating a Wurst instance, optional **Also install Baritone** adds the same standalone Fabric Baritone build used elsewhere.

LiquidBounce scripts/themes and Impact companion mods are not automated here: LB marketplace installs run in-game; Impact bundles Baritone via its installer.

All clients are third-party software; Arsenal Launcher only automates their documented install flows.
