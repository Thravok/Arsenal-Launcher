#!/usr/bin/env python3
"""Download Lucide SVGs into launcher icon theme names (ISC license — Lucide)."""
from __future__ import annotations

import re
import urllib.error
import urllib.request
from pathlib import Path

BASE = "https://raw.githubusercontent.com/lucide-icons/lucide/main/icons"
ROOT = Path(__file__).resolve().parent
OUT = ROOT / "scalable"

# Arsenal Launcher icon theme name -> lucide icon file name (kebab-case)
MAP: dict[str, str] = {
    "new": "plus",
    "settings": "settings",
    "viewfolder": "folder-open",
    "centralmods": "folder-kanban",
    "refresh": "refresh-cw",
    "accounts": "users",
    "copy": "copy",
    "bug": "bug",
    "about": "info",
    "checkupdate": "download",
    "loadermods": "package",
    "log": "scroll-text",
    "news": "image",
    "noaccount": "circle-user",
    "java": "coffee",
    "worlds": "globe",
    "resourcepacks": "images",
    "shaderpacks": "sparkles",
    "minecraft": "box",
    "launcher": "rocket",
    "notes": "sticky-note",
    "storage": "hard-drive",
    "instance-settings": "sliders-horizontal",
    "help": "circle-question-mark",
    "language": "languages",
    "proxy": "shield",
    "externaltools": "wrench",
    "custom-commands": "terminal",
    "coremods": "cpu",
    "jarmods": "file-archive",
    "screenshots": "camera",
    "screenshot-placeholder": "image-off",
    "star": "star",
    "packages": "package",
    "status-good": "circle-check",
    "status-bad": "circle-x",
    "status-yellow": "circle-alert",
}

STATUS_STROKE = {
    "status-good": "#3db8a0",
    "status-bad": "#e85d5d",
    "status-yellow": "#e8b84a",
}


def normalize_svg(body: str, stroke_override: str | None) -> str:
    body = body.replace('stroke="currentColor"', 'stroke="currentColor"')
    if stroke_override:
        body = re.sub(r'stroke="[^"]*"', f'stroke="{stroke_override}"', body, count=1)
    if 'stroke-width' not in body and "<svg" in body:
        body = body.replace("<svg ", '<svg stroke-width="2" ', 1)
    return body


def main() -> None:
    OUT.mkdir(parents=True, exist_ok=True)
    for dest, src in sorted(MAP.items()):
        url = f"{BASE}/{src}.svg"
        try:
            with urllib.request.urlopen(url, timeout=30) as resp:
                raw = resp.read().decode("utf-8")
        except urllib.error.URLError as e:
            raise SystemExit(f"failed to fetch {url}: {e}") from e
        stroke = STATUS_STROKE.get(dest)
        text = normalize_svg(raw, stroke)
        (OUT / f"{dest}.svg").write_text(text, encoding="utf-8")
        print(f"wrote {dest}.svg <- {src}")

    index = ROOT / "index.theme"
    if not index.exists():
        index.write_text(
            "[Icon Theme]\n"
            "Name=Lucide\n"
            "Comment=Lucide stroke icons for Arsenal Launcher\n"
            "Inherits=multimc\n"
            "Directories=scalable\n\n"
            "[scalable]\n"
            "Size=48\n"
            "Type=Scalable\n"
            "MinSize=16\n"
            "MaxSize=256\n",
            encoding="utf-8",
        )


if __name__ == "__main__":
    main()
