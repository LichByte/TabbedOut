# Nexus mod collections

Curated Nexus Mods collections defined as reviewable YAML rather than opaque
Vortex profile exports, plus a shared builder that turns them into
Vortex-importable archives.

| Collection | Game | Mods |
|---|---|---|
| [commonwealth-overhauled](commonwealth-overhauled) | Fallout 4 | 56 (+8 in an opt-in adult overlay) |
| [skyrim-se-overhauled](skyrim-se-overhauled) | Skyrim Special Edition | 41 |

```bash
pip install -r requirements.txt
export NEXUS_API_KEY=...      # nexusmods.com/users/myaccount?tab=api

python3 build_collection.py commonwealth-overhauled
python3 build_collection.py skyrim-se-overhauled
```

Add `--offline` for a structure-only build with no API key — useful for
reviewing a manifest, not installable. Add `--adult` where an overlay exists.

## Why

A Nexus collection is normally produced by installing everything in Vortex and
hitting export, so the list only ever exists inside one person's Vortex profile.
Here it is a YAML file you can diff, review and rebuild when mods update.

The tradeoff: a collection entry needs a **file id**, not just a mod id, and
those change on every upload. So the builder resolves the current main file for
each mod at build time against the Nexus API.

## What the builder refuses to emit

Not warnings — hard failures, because each of these ships something broken:

- **A mod whose Nexus name no longer matches the list.** Catches renames,
  hides and deletions.
- **Two mutually exclusive mods both marked required.** Body replacers, perk
  overhauls and weather mods overwrite each other rather than merging.
- **The same mod id twice** across a base list and its overlay.
- **A mod flagged adult on Nexus but not in the list** — it would otherwise
  leak into the SFW build. The reverse only warns.
- **A dangling ordering rule** naming a mod that isn't in the list.

## Adding a collection

Create a directory with a `modlist.yaml`. The `collection.domain` field sets the
Nexus game domain, so the builder is game-agnostic — no code changes needed for
a new title. Copy the schema from either existing list; both READMEs document
the per-mod fields.

## Limits worth knowing

The generated zip imports into Vortex but is not directly publishable to Nexus —
that needs file hashes and screenshots only Vortex produces. The publish path is
import → install → adjust → **Publish** from Vortex, treating this repo as the
thing you edit and the Vortex profile as the build artifact.

Collections can only reference **Nexus-hosted files**. Anything distributed
elsewhere — SKSE, F4SE, most LoversLab content — is listed under
`prerequisites` and carried into the manifest's install instructions as a manual
step instead.
