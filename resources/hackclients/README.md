# Hack clients resources

Impact Installer is **not** vendored in this repository.

`ImpactInstallTask` downloads the official installer on first use:

- Upstream: https://github.com/ImpactDevelopment/Installer/releases/download/0.9.5/installer-0.9.5.jar
- Cached under the launcher metacache base `hackclients/`

LiquidBounce artifacts are fetched from liquidbounce.net / api.liquidbounce.net at install time.

Meteor Client builds are listed from `https://meteorclient.com/api/stats` and JARs (plus optional Baritone) are downloaded from meteorclient.com at install time.

Lambda releases are listed from GitHub (`lambda-client/lambda`). Install pulls companion versions from that tag's `gradle.properties` (Fabric API, Fabric Language Kotlin, Baritone from rfresh2 Maven).

FDPClient releases are listed from GitHub (`SkidderMC/FDPClient`). Install creates Minecraft 1.8.9 with Forge `11.15.1.2318` (per upstream install docs) and downloads the release JAR into `mods/`.

Wurst Client v7 JARs are listed from GitHub (`Wurst-Imperium/Wurst-MCX2` releases). Install pulls Fabric Loader and Fabric API versions from the matching `Wurst-Imperium/Wurst7` tag's `gradle.properties`, then downloads the Wurst JAR and Fabric API into `mods/`.

**Epsilon** — lists Fabric JARs from GitHub (`NekoyaHouse/Epsilon` releases; skips NeoForge / sources / unrelated assets). Creates a Fabric instance with the recommended Fabric Loader, downloads the selected Epsilon Fabric JAR, and resolves Fabric API from Modrinth for that Minecraft version. Upstream development is currently paused; only public release assets are used (no auth / closed downloaders).

**Baritone** — lists Minecraft versions from [rfresh2 Baritone Maven](https://maven.2b2t.vc/releases/com/github/rfresh2/baritone-fabric/) (newer releases) plus official [cabaletta/baritone](https://github.com/cabaletta/baritone/releases) standalone Fabric builds for older versions (e.g. 1.16.5). Creates a Fabric instance and downloads Baritone. Existing Fabric instances can use **Install Baritone** on the Mods tab.

**Create Instance categories** — the New Instance sidebar lists **Anarchy**, **PvP**, and **Utility** as separate pages (Custom and other platforms follow). The same client can appear in more than one section (see `HackClientCatalog`).

**Not automated here** — paid / closed / auth-gated clients (e.g. RusherHack, Future, CatLean) and unofficial leak mirrors (e.g. Shoreline) are intentionally out of scope. Prefer public GitHub / documented CDN installs only.

**Anarchy Utils** — on Fabric instances, the Mods tab shows **Anarchy Utils**. Catalog entries:

- **Litematica** + **Litematica Printer** (Modrinth; MaLiLib via required deps)
- **Arsenal NameProtect** ([Thravok/Arsenal-Name-Protect](https://github.com/Thravok/Arsenal-Name-Protect) GitHub Releases) — client-side name aliases synced from Settings → Name Protect; does not change online Mojang identity (Fabric 1.21.x JARs named `arsenal-nameprotect-mc{version}-*.jar`)

**New Instance** includes **Install Arsenal NameProtect** (on by default). After a Fabric instance is created successfully, Arsenal NameProtect is downloaded automatically. Non-Fabric loaders skip the install silently.

Before download, Arsenal walks Modrinth `required` dependencies and shows a review dialog. Downloads go to a temp file first, then replace older jars; Modrinth-backed installs also write `.index` metadata so Check for Updates can see them. More curated utilities (ViaFabricPlus, SeedcrackerX, Bobby) can be added to the same catalog later.

**Meteor addons** — on a Fabric instance that already has a Meteor Client JAR in `mods/`, the Mods tab shows **Meteor Addons**. The catalog is fetched from the community list maintained by [meteor-addon-scanner](https://github.com/cqb13/meteor-addon-scanner) (`addons/addons.json`). Direct release JAR URLs are downloaded into `mods/`. Skip `-dev.jar` and `-sources.jar` assets when picking downloads.

**Wurst + Baritone** — when creating a Wurst instance, optional **Also install Baritone** adds the same standalone Fabric Baritone build used elsewhere.

LiquidBounce scripts/themes and Impact companion mods are not automated here: LB marketplace installs run in-game; Impact bundles Baritone via its installer.

All clients are third-party software; Arsenal Launcher only automates their documented install flows.
