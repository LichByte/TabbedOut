#!/usr/bin/env python3
"""Generate verified modlist entries in bulk from the Nexus API.

Hand-curating a list is fine at 40 mods and impossible at 1000. The bottleneck
is not typing — it is *verification*: a mod id you did not check is a broken
collection entry, and build_collection.py rejects those by design.

This flips the direction. You supply mod ids; every name, author, category and
adult flag in the emitted YAML comes back from Nexus. Entries are correct by
construction, so bulk additions cannot fabricate anything.

    export NEXUS_API_KEY=...

    # from an explicit id list (e.g. exported from an existing load order)
    python3 bulk_add.py skyrim-se-overhauled --ids-file ids.txt --out modlist-bulk.yaml

    # from mods you track on Nexus — track a few hundred, then generate
    python3 bulk_add.py skyrim-se-overhauled --from-tracked --out modlist-bulk.yaml

    # enumerate what has been updated recently, then prune by hand
    python3 bulk_add.py skyrim-se-overhauled --from-updated --period 1m --limit 200

Ids already present in the collection's existing modlists are skipped, so you
can re-run this against a growing list without producing duplicates (which the
builder would reject anyway).

What it does NOT do is curate. It cannot tell you that two mods conflict, which
of three body replacers to pick, or what belongs in which phase. Everything it
emits lands at phase 2 with a TODO note, for you to sort. That is the honest
division of labour: the API can verify, only a person can curate.
"""

from __future__ import annotations

import argparse
import os
import sys
from pathlib import Path

import yaml

from build_collection import BuildError, NexusClient

# Nexus category names → the category vocabulary these modlists use. Anything
# unmapped keeps its Nexus name, which is a fine starting point for sorting.
CATEGORY_HINTS = {
    "Bug Fixes": "Fixes",
    "Patches": "Fixes",
    "Utilities": "Framework",
    "User Interface": "Interface",
    "Models and Textures": "Visuals",
    "Visuals and Graphics": "Visuals",
    "Environmental": "Visuals",
    "Lighting": "Visuals",
    "Weather and Lighting": "Visuals",
    "Animation": "Animation",
    "Body, Face, and Hair": "Characters",
    "Combat": "Combat",
    "Gameplay Effects and Changes": "Gameplay",
    "Skills and Leveling": "Gameplay",
    "Magic - Gameplay": "Gameplay",
    "Quests and Adventures": "Quests",
    "New Lands": "Quests",
    "NPC": "NPC",
    "Cities, Towns, Villages and Hamlets": "Visuals",
    "Audio": "Audio",
    "Items and Objects - Player": "Items",
    "Weapons": "Items",
    "Armour": "Items",
}


def existing_ids(collection: Path) -> set[int]:
    """Every mod id already in this collection's base list and overlays."""
    found: set[int] = set()
    for path in sorted(collection.glob("modlist*.yaml")):
        spec = yaml.safe_load(path.read_text()) or {}
        for entry in spec.get("mods") or []:
            found.add(entry["mod_id"])
    return found


def read_ids(args: argparse.Namespace, client: NexusClient) -> list[int]:
    ids: list[int] = []
    if args.ids:
        ids += [int(x) for x in args.ids.replace(",", " ").split()]
    if args.ids_file:
        for line in args.ids_file.read_text().splitlines():
            line = line.split("#", 1)[0].strip()
            if line:
                ids.append(int(line))
    if args.from_tracked:
        ids += client.tracked_mods()
    if args.from_updated:
        ids += client.updated_mods(args.period)
    # dedupe, preserve order
    seen: set[int] = set()
    return [i for i in ids if not (i in seen or seen.add(i))]


def make_entry(info: dict, categories: dict[int, str]) -> dict:
    nexus_category = categories.get(info.get("category_id", -1), "")
    entry: dict = {
        "name": info["name"],
        "mod_id": info["mod_id"],
        "phase": 2,
        "category": CATEGORY_HINTS.get(nexus_category, nexus_category or "Unsorted"),
    }
    if info.get("contains_adult_content"):
        entry["adult"] = True
    author = info.get("author") or ""
    entry["notes"] = (
        f"TODO curate. Auto-generated from Nexus"
        + (f" (by {author})" if author else "")
        + ". Set the phase, check for conflicts, and write a real note "
        "explaining why this mod is in the list."
    )
    return entry


def main() -> int:
    parser = argparse.ArgumentParser(
        description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter
    )
    parser.add_argument("collection", type=Path, help="Collection directory.")
    parser.add_argument("--ids", help="Comma/space separated mod ids.")
    parser.add_argument("--ids-file", type=Path, help="File of mod ids, one per line.")
    parser.add_argument(
        "--from-tracked",
        action="store_true",
        help="Use mods tracked by the API key's Nexus account, filtered to this game.",
    )
    parser.add_argument(
        "--from-updated",
        action="store_true",
        help="Use mods updated recently. Broad and unfiltered — pair with --limit.",
    )
    parser.add_argument("--period", default="1w", choices=["1d", "1w", "1m"])
    parser.add_argument("--limit", type=int, help="Stop after N new mods.")
    parser.add_argument(
        "--out",
        type=Path,
        default=Path("modlist-bulk.yaml"),
        help="Output file, relative to the collection dir. Appended to if it exists.",
    )
    parser.add_argument(
        "--include-adult",
        action="store_true",
        help="Keep adult-flagged mods. Off by default so they do not land in a "
        "base list by accident — put them in an adult overlay deliberately.",
    )
    parser.add_argument("--api-key", default=os.environ.get("NEXUS_API_KEY"))
    args = parser.parse_args()

    if not args.collection.is_dir():
        print(f"error: {args.collection} is not a directory", file=sys.stderr)
        return 2
    if not args.api_key:
        print("error: no API key. Set NEXUS_API_KEY or pass --api-key.", file=sys.stderr)
        return 2
    if not any([args.ids, args.ids_file, args.from_tracked, args.from_updated]):
        print("error: give one of --ids, --ids-file, --from-tracked, --from-updated",
              file=sys.stderr)
        return 2

    base = yaml.safe_load((args.collection / "modlist.yaml").read_text())
    domain = base["collection"]["domain"]
    client = NexusClient(args.api_key, domain)

    skip = existing_ids(args.collection)
    wanted = [i for i in read_ids(args, client) if i not in skip]
    print(f"{len(wanted)} candidate ids ({len(skip)} already in this collection)")

    categories = client.game_categories()
    entries: list[dict] = []
    dropped_adult = 0
    for mod_id in wanted:
        if args.limit and len(entries) >= args.limit:
            break
        try:
            info = client.mod_info(mod_id)
        except BuildError as exc:
            print(f"  skip {mod_id}: {exc}", file=sys.stderr)
            continue
        if info.get("status") not in (None, "published") or info.get("available") is False:
            print(f"  skip {mod_id}: not available")
            continue
        if info.get("contains_adult_content") and not args.include_adult:
            dropped_adult += 1
            continue
        entry = make_entry(info, categories)
        entries.append(entry)
        print(f"  + {entry['mod_id']:>7}  [{entry['category']}] {entry['name']}")

    if dropped_adult:
        print(f"{dropped_adult} adult-flagged mods skipped (--include-adult to keep)")
    if not entries:
        print("Nothing new to add.")
        return 0

    out = args.collection / args.out
    if out.exists():
        doc = yaml.safe_load(out.read_text()) or {}
        doc.setdefault("mods", []).extend(entries)
    else:
        doc = {
            "collection": {"name_suffix": f" ({args.out.stem.replace('modlist-', '')})"},
            "mods": entries,
        }
    out.write_text(yaml.safe_dump(doc, sort_keys=False, width=88, allow_unicode=True))

    print(f"\nWrote {len(entries)} entries to {out} ({len(doc['mods'])} total)")
    print("Every entry is marked TODO curate — set phases, add rules, note conflicts.")
    if client.remaining is not None:
        print(f"Nexus API requests remaining today: {client.remaining}")
    return 0


if __name__ == "__main__":
    try:
        sys.exit(main())
    except BuildError as exc:
        print(f"error: {exc}", file=sys.stderr)
        sys.exit(1)
