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

**Baritone** — lists Minecraft versions from [rfresh2 Baritone Maven](https://maven.2b2t.vc/releases/com/github/rfresh2/baritone-fabric/) (newer releases) plus official [cabaletta/baritone](https://github.com/cabaletta/baritone/releases) standalone Fabric builds for older versions (e.g. 1.16.5). Creates a Fabric instance and downloads Baritone. Existing Fabric instances can use **Install Baritone** on the Mods tab.

**Meteor addons** — on a Fabric instance that already has a Meteor Client JAR in `mods/`, the Mods tab shows **Meteor Addons**. The catalog is fetched from the community list maintained by [meteor-addon-scanner](https://github.com/cqb13/meteor-addon-scanner) (`addons/addons.json`). Direct release JAR URLs are downloaded into `mods/`. Skip `-dev.jar` and `-sources.jar` assets when picking downloads.

**Wurst + Baritone** — when creating a Wurst instance, optional **Also install Baritone** adds the same standalone Fabric Baritone build used elsewhere.

LiquidBounce scripts/themes and Impact companion mods are not automated here: LB marketplace installs run in-game; Impact bundles Baritone via its installer.

All clients are third-party software; Arsenal Launcher only automates their documented install flows.
