# Level Pronouns (GD 2.2081 / Geode 5)

Source rewrite of the Level Pronouns mod for Geometry Dash 2.2081.

## Build locally
1. Install the Geode CLI and SDK: `geode sdk install` (or `geode sdk update`), then `geode sdk install-binaries`
2. Run `geode sdk version` and put that version in the `"geode"` field of `mod.json` if it differs
3. In this folder: `geode build`
4. The `.geode` file appears in `build/`; drop it into your Geode `mods` folder

## Build without installing anything
Push this folder to a GitHub repo. The included Actions workflow builds Windows,
macOS and Android binaries and uploads a combined `.geode` as an artifact.
