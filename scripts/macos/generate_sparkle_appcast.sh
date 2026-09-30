#!/usr/bin/env bash
# Generate a Sparkle appcast.xml for an Arsenal macOS DMG.
#
# Usage:
#   ./scripts/macos/generate_sparkle_appcast.sh \
#       --dmg Arsenal.dmg \
#       --tag v1.0.0 \
#       --key arsenal-sparkle-ed25519.pem \
#       --out appcast.xml
#
# Requires Sparkle's generate_appcast (from a local build tree or SPARKLE_BIN).
set -euo pipefail

dmg=""
tag=""
key=""
out="appcast.xml"
sparkle_bin="${SPARKLE_BIN:-}"
repo="Thravok/Arsenal-Launcher"
asset_name=""

usage() {
  sed -n '2,12p' "$0" | sed 's/^# \{0,1\}//'
  exit "${1:-0}"
}

while [[ $# -gt 0 ]]; do
  case "$1" in
    --dmg) dmg="$2"; shift 2 ;;
    --tag) tag="$2"; shift 2 ;;
    --key) key="$2"; shift 2 ;;
    --out) out="$2"; shift 2 ;;
    --sparkle-bin) sparkle_bin="$2"; shift 2 ;;
    --repo) repo="$2"; shift 2 ;;
    --asset-name) asset_name="$2"; shift 2 ;;
    -h|--help) usage 0 ;;
    *) echo "Unknown option: $1" >&2; usage 1 ;;
  esac
done

[[ -n "$dmg" && -f "$dmg" ]] || { echo "Missing --dmg file" >&2; exit 1; }
[[ -n "$tag" ]] || { echo "Missing --tag (e.g. v1.0.0)" >&2; exit 1; }
[[ -n "$key" && -f "$key" ]] || { echo "Missing --key file" >&2; exit 1; }

if [[ -z "$sparkle_bin" ]]; then
  for candidate in \
    "${SPARKLE_BIN:-}" \
    "$(dirname "$0")/../../build/frameworks/Sparkle/bin" \
    "$(dirname "$0")/../../build/frameworks/Sparkle/Sparkle.framework/Versions/B/bin"
  do
    if [[ -n "$candidate" && -x "$candidate/generate_appcast" ]]; then
      sparkle_bin="$candidate"
      break
    fi
  done
fi

[[ -n "$sparkle_bin" && -x "$sparkle_bin/generate_appcast" ]] || {
  echo "generate_appcast not found. Build Arsenal on macOS first, or set SPARKLE_BIN." >&2
  exit 1
}

if [[ -z "$asset_name" ]]; then
  asset_name="Arsenal-macOS-${tag}.dmg"
fi

work="$(mktemp -d "${TMPDIR:-/tmp}/arsenal-appcast.XXXXXX")"
cleanup() { rm -rf "$work"; }
trap cleanup EXIT

cp "$dmg" "$work/$asset_name"

# Skip deltas — full DMG updates only (simpler CI / hosting on GitHub Releases).
"$sparkle_bin/generate_appcast" \
  --ed-key-file "$key" \
  --download-url-prefix "https://github.com/${repo}/releases/download/${tag}/" \
  --link "https://github.com/${repo}/releases/tag/${tag}" \
  --maximum-deltas 0 \
  --maximum-versions 5 \
  -o "$(basename "$out")" \
  "$work"

generated="$work/$(basename "$out")"
[[ -f "$generated" ]] || { echo "generate_appcast did not write $generated" >&2; exit 1; }

mkdir -p "$(dirname "$out")"
cp "$generated" "$out"
echo "Wrote $out"
echo "Feed URL once published: https://github.com/${repo}/releases/latest/download/$(basename "$out")"
