# Nexus mod collections

Curated Nexus Mods collections defined as reviewable YAML rather than opaque
Vortex profile exports, plus a shared builder that turns them into
Vortex-importable archives.

| Collection | Game | Base | Overlays |
|---|---|---|---|
| [commonwealth-overhauled](commonwealth-overhauled) | Fallout 4 | 56 | `adult` (+8), `cinematics` (+5) |
| [skyrim-se-overhauled](skyrim-se-overhauled) | Skyrim Special Edition | 41 | `visuals` (+12), `cinematics` (+12), `adult` (+20) |

```bash
pip install -r requirements.txt
export NEXUS_API_KEY=...      # nexusmods.com/users/myaccount?tab=api

python3 build_collection.py commonwealth-overhauled
python3 build_collection.py skyrim-se-overhauled --overlay cinematics
```

Add `--offline` for a structure-only build with no API key — useful for
reviewing a manifest, not installable.

## Overlays

An overlay is `modlist-<name>.yaml` in a collection directory, merged on top of
the base list with `--overlay <name>`. They stack, and each contributes a name
suffix so every combination lands in its own manifest and zip instead of
overwriting the base build:

```bash
python3 build_collection.py commonwealth-overhauled --overlay cinematics --overlay adult
# → commonwealth-overhauled-cinematics-adult.zip
```

Overlays carry their own `prerequisites`, so off-Nexus dependencies travel with
the mods that need them. `--adult` is a legacy alias for `--overlay adult`.

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

## Scaling to a large list

Hand-curation is fine at 40 mods and impossible at 1000. The bottleneck is not
typing — it is **verification**. An unverified mod id is a broken collection
entry, and `build_collection.py` rejects those by design.

[`bulk_add.py`](bulk_add.py) flips the direction: you supply mod ids, and every
name, author, category and adult flag comes back from the Nexus API. Entries are
correct by construction, so bulk additions cannot fabricate anything.

```bash
export NEXUS_API_KEY=...

# from an id list exported from an existing load order
python3 bulk_add.py skyrim-se-overhauled --ids-file ids.txt --out modlist-bulk.yaml

# from mods you track on Nexus — track a few hundred, then generate
python3 bulk_add.py skyrim-se-overhauled --from-tracked --out modlist-bulk.yaml

# enumerate recent activity, then prune by hand
python3 bulk_add.py skyrim-se-overhauled --from-updated --period 1m --limit 200
```

Ids already in the collection are skipped, so it is safe to re-run as the list
grows. Adult-flagged mods are dropped unless you pass `--include-adult`, so they
cannot land in a base list by accident.

**It does not curate.** Everything lands at phase 2 with a `TODO curate` note.
It cannot tell you that two mods conflict, which of three body replacers to
pick, or what belongs in which phase. That is the honest division: the API can
verify, only a person can curate.

### What actually bites at 1000 mods

- **Not the plugin limit.** Skyrim's 254-plugin cap counts only full ESPs.
  ESL-flagged plugins get 4096 separate slots, and texture, mesh and animation
  replacers carry no plugin at all — most of a large list is those. Check the
  ESL flag as you go rather than discovering the ceiling at mod 300.
- **Patches, not mods.** Past a few hundred, most additions are compatibility
  patches between things you already have. Budget for a Bashed/Smashed patch and
  real xEdit conflict resolution; neither is optional at that size.
- **Tool runtime.** DynDOLOD and Nemesis go from minutes to a long coffee break,
  and you re-run them on every change.
- **Distribution.** Nexus Collections handle large lists, but most 1000+ lists
  ship as [Wabbajack](https://www.wabbajack.org/) installers instead, because
  they can carry the patches, INI edits and generated LOD output that a
  collection manifest cannot.

Realistically a 1000-mod list is months of work and is why the big ones
(Nolvus, Lorerim, Living Skyrim) are long-running projects with teams. The
tooling here removes the verification tax; it does not remove the curation.

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
