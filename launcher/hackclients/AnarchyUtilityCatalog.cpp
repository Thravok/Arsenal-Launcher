// SPDX-License-Identifier: GPL-3.0-only
#include "AnarchyUtilityCatalog.h"

namespace HackClients {

QList<AnarchyUtilitySpec> AnarchyUtilityCatalog::all()
{
    // Litematica + printer from Modrinth. Arsenal NameProtect from GitHub Releases.
    // jarNameHints must not overlap (e.g. bare "litematica" would match the printer jar).
    return {
        AnarchyUtilitySpec{
            QStringLiteral("litematica"),
            QStringLiteral("Litematica"),
            QStringLiteral("Schematic viewer / easy-place for building and anarchy workflows. "
                           "Required Modrinth dependencies (e.g. MaLiLib) are resolved automatically."),
            QStringLiteral("litematica"),
            { QStringLiteral("litematica-fabric") },
            {},
        },
        AnarchyUtilitySpec{
            QStringLiteral("litematica-printer"),
            QStringLiteral("Litematica Printer"),
            QStringLiteral("Automatic schematic printing for Litematica (CAPS_LOCK by default). "
                           "Pulls Litematica / MaLiLib via Modrinth required dependencies when needed."),
            QStringLiteral("litematica-printer"),
            { QStringLiteral("litematica-printer") },
            {},
        },
        AnarchyUtilitySpec{
            QStringLiteral("arsenal-nameprotect"),
            QStringLiteral("Arsenal NameProtect"),
            QStringLiteral("Client-side name aliases synced from Arsenal settings "
                           "(chat / tab / nametags on your screen only). "
                           "Does not change your online Mojang identity. Fabric 1.21.x."),
            {},  // Modrinth unused
            { QStringLiteral("arsenal-nameprotect") },
            QStringLiteral("Thravok/Arsenal-Name-Protect"),
        },
        AnarchyUtilitySpec{
            QStringLiteral("viafabricplus"),
            QStringLiteral("ViaFabricPlus"),
            QStringLiteral("Join older and newer Minecraft server versions from a modern Fabric client "
                           "(Classic through current releases, including Bedrock via ViaBedrock)."),
            QStringLiteral("viafabricplus"),
            { QStringLiteral("viafabricplus") },
            {},
        },
        AnarchyUtilitySpec{
            QStringLiteral("bobby"),
            QStringLiteral("Bobby"),
            QStringLiteral("Client-side chunk cache so you can set render distance higher than the "
                           "server view-distance. No server-side install required."),
            QStringLiteral("bobby"),
            { QStringLiteral("bobby") },
            {},
        },
        AnarchyUtilitySpec{
            QStringLiteral("seedcrackerx"),
            QStringLiteral("SeedcrackerX"),
            QStringLiteral("Client-side structure finder that helps recover the world seed from "
                           "observed terrain. Fabric builds from GitHub Releases."),
            {},  // Modrinth unused
            { QStringLiteral("seedcrackerx"), QStringLiteral("seedcrackerX") },
            QStringLiteral("19MisterX98/SeedcrackerX"),
        },
    };
}

AnarchyUtilitySpec AnarchyUtilityCatalog::byId(const QString& id)
{
    for (const auto& entry : all()) {
        if (entry.id == id)
            return entry;
    }
    return {};
}

}  // namespace HackClients
