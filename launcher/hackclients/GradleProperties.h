// SPDX-License-Identifier: GPL-3.0-only
#pragma once

#include <QString>

namespace HackClients {

/** First non-comment `key=value` line in a gradle.properties document. */
inline QString gradleProperty(const QString& text, const QString& key)
{
    const QString prefix = key + QLatin1Char('=');
    const auto lines = text.split(QLatin1Char('\n'));
    for (QString line : lines) {
        line = line.trimmed();
        if (line.isEmpty() || line.startsWith(QLatin1Char('#'))) {
            continue;
        }
        if (line.startsWith(prefix)) {
            return line.mid(prefix.size()).trimmed();
        }
    }
    return {};
}

}  // namespace HackClients
