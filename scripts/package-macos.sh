#!/usr/bin/env bash

# WAIVE-FRONT
# Copyright (C) 2024  Bram Bogaerts, Superposition
#
# This program is free software: you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation, either version 3 of the License, or
# (at your option) any later version.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with this program.  If not, see <https://www.gnu.org/licenses/>.

# Packages the macOS builds: wraps the standalone app in an .app bundle, and,
# when the environment allows it, signs and notarises everything so it opens
# without warnings.
#
# Usage: package-macos.sh <build/bin> <output dir>
#
# Signing, optional:
#   MACOS_SIGN_IDENTITY   "Developer ID Application: ..." or its hash, in your keychain
# Notarisation, optional, needs signing:
#   APPLE_ID              Apple ID email address
#   APPLE_APP_PASSWORD    app-specific password for that Apple ID
#   APPLE_TEAM_ID         team ID of the developer account

set -euo pipefail

BIN="$1"
OUT="$2"
ROOT="$(cd "$(dirname "$0")/.." && pwd)"
NAME=WAIVE-FRONT-V2
BUNDLES=("$NAME.app" "$NAME.vst3" "$NAME.component")

rm -rf "$OUT"
mkdir -p "$OUT"

# The standalone app as a bundle, with its icon
APP="$OUT/$NAME.app"
mkdir -p "$APP/Contents/MacOS" "$APP/Contents/Resources"
cp "$BIN/$NAME" "$APP/Contents/MacOS/$NAME"
cp "$ROOT/assets/Info.plist" "$APP/Contents/Info.plist"
cp "$ROOT/assets/Icon.icns" "$APP/Contents/Resources/Icon.icns"
plutil -replace LSMinimumSystemVersion -string "${MACOSX_DEPLOYMENT_TARGET:-11.0}" "$APP/Contents/Info.plist"

for bundle in "$NAME.vst3" "$NAME.component"; do
    ditto "$BIN/$bundle" "$OUT/$bundle"
done

if [[ -z "${MACOS_SIGN_IDENTITY:-}" ]]; then
    echo "package-macos: MACOS_SIGN_IDENTITY not set, leaving the builds unsigned"
    exit 0
fi

for bundle in "${BUNDLES[@]}"; do
    codesign --force --timestamp --options runtime \
        --entitlements "$ROOT/assets/entitlements.plist" \
        --sign "$MACOS_SIGN_IDENTITY" "$OUT/$bundle"
    codesign --verify --strict --verbose=2 "$OUT/$bundle"
done

if [[ -z "${APPLE_ID:-}" || -z "${APPLE_APP_PASSWORD:-}" || -z "${APPLE_TEAM_ID:-}" ]]; then
    echo "package-macos: APPLE_ID, APPLE_APP_PASSWORD or APPLE_TEAM_ID not set, signed but not notarised"
    exit 0
fi

# One submission for all bundles, then staple the ticket to each, so they
# also open without a network connection.
SUBMISSION="$OUT/notarise.zip"
STAGE="$(mktemp -d)"
for bundle in "${BUNDLES[@]}"; do
    ditto "$OUT/$bundle" "$STAGE/$bundle"
done
ditto -c -k --sequesterRsrc "$STAGE" "$SUBMISSION"
rm -rf "$STAGE"

xcrun notarytool submit "$SUBMISSION" \
    --apple-id "$APPLE_ID" --password "$APPLE_APP_PASSWORD" --team-id "$APPLE_TEAM_ID" \
    --wait --timeout 30m
rm "$SUBMISSION"

for bundle in "${BUNDLES[@]}"; do
    xcrun stapler staple "$OUT/$bundle"
done

spctl --assess --type execute --verbose=2 "$OUT/$NAME.app"
echo "package-macos: signed and notarised"
