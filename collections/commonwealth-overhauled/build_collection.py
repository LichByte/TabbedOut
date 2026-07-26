#!/usr/bin/env python3
"""Build a Vortex-importable collection archive from modlist.yaml.

Reads the curated list, resolves every mod against the Nexus API to pick up the
current main file id and version, verifies that the live mod name still matches
what the list expects, and writes collection.json plus a zip Vortex can import.

    export NEXUS_API_KEY=...
    python3 build_collection.py

Get an API key from https://www.nexusmods.com/users/myaccount?tab=api (Personal
API Key, bottom of the page).

Without a key, --offline emits a manifest with placeholder file ids so you can
inspect the structure. That output is NOT installable — Vortex needs real file
ids to download anything.
"""

from __future__ import annotations

import argparse
import json
import os
import re
import sys
import zipfile
from pathlib import Path
from typing import Any

import requests
import yaml

API_ROOT = "https://api.nexusmods.com/v1"
HERE = Path(__file__).resolve().parent

# Vortex refuses to resolve a nexus source without a numeric fileId. This
# sentinel makes an unresolved entry obvious rather than subtly wrong.
PLACEHOLDER_FILE_ID = 0


class BuildError(Exception):
    pass


def normalise(name: str) -> str:
    """Loose comparison key for mod titles.

    Authors routinely append version tags, language markers and decoration to
    the page title, so exact equality produces false alarms. Compare on
    lowercase alphanumerics only.
    """
    return re.sub(r"[^a-z0-9]+", "", name.lower())


class NexusClient:
    def __init__(self, api_key: str, game: str) -> None:
        self.game = game
        self.session = requests.Session()
        self.session.headers.update(
            {
                "apikey": api_key,
                "Application-Name": "commonwealth-overhauled-builder",
                "Application-Version": "1.0.0",
                "User-Agent": "commonwealth-overhauled-builder/1.0.0",
                "Accept": "application/json",
            }
        )
        self.remaining: str | None = None

    def _get(self, path: str, **params: Any) -> Any:
        resp = self.session.get(f"{API_ROOT}{path}", params=params, timeout=30)
        self.remaining = resp.headers.get("X-RL-Daily-Remaining", self.remaining)
        if resp.status_code == 401:
            raise BuildError("Nexus rejected the API key (401). Check NEXUS_API_KEY.")
        if resp.status_code == 429:
            raise BuildError(
                "Nexus rate limit hit (429). Daily remaining: "
                f"{resp.headers.get('X-RL-Daily-Remaining', 'unknown')}. Retry later."
            )
        if resp.status_code == 404:
            raise BuildError(f"Not found: {path} — the mod may have been hidden or deleted.")
        resp.raise_for_status()
        return resp.json()

    def mod_info(self, mod_id: int) -> dict:
        return self._get(f"/games/{self.game}/mods/{mod_id}.json")

    def main_files(self, mod_id: int) -> list[dict]:
        payload = self._get(f"/games/{self.game}/mods/{mod_id}/files.json", category="main")
        return payload.get("files", [])


def pick_file(files: list[dict], mod_name: str) -> dict:
    """Choose the file a fresh installer should get.

    Nexus marks one main file as primary when the author sets one; otherwise the
    most recently uploaded main file is the right answer.
    """
    if not files:
        raise BuildError(
            f"{mod_name}: no main files on Nexus. It may be archived, or the "
            "author may only publish optional/update files — resolve by hand."
        )
    primary = [f for f in files if f.get("is_primary")]
    if primary:
        return primary[0]
    return max(files, key=lambda f: f.get("uploaded_timestamp", 0))


def check_name(expected: str, entry: dict, live: dict, problems: list[str]) -> None:
    live_name = live.get("name") or ""
    needle = entry.get("expected_name_contains")
    if needle:
        ok = normalise(needle) in normalise(live_name)
    else:
        ok = normalise(expected) == normalise(live_name)
    if not ok:
        problems.append(
            f"mod {entry['mod_id']}: list says {expected!r}, Nexus says {live_name!r}"
        )
    if live.get("status") not in (None, "published"):
        problems.append(f"mod {entry['mod_id']} ({live_name}): status is {live['status']!r}")
    if live.get("available") is False:
        problems.append(f"mod {entry['mod_id']} ({live_name}): no longer available")


def build_mod_entry(entry: dict, domain: str, resolved: dict | None) -> dict:
    source: dict[str, Any] = {
        "type": "nexus",
        "modId": entry["mod_id"],
        "fileId": PLACEHOLDER_FILE_ID,
        # "prefer" lets Vortex move installers to a newer file when the pinned
        # one is archived, instead of hard-failing the whole collection.
        "updatePolicy": "prefer",
    }
    version = "0.0.0"
    author = ""
    if resolved:
        source["fileId"] = resolved["file"]["file_id"]
        source["logicalFilename"] = resolved["file"].get("name", "")
        version = resolved["file"].get("version") or resolved["info"].get("version") or "0.0.0"
        author = resolved["info"].get("author") or ""

    mod: dict[str, Any] = {
        "name": entry["name"],
        "version": version,
        "optional": bool(entry.get("optional", False)),
        "domainName": domain,
        "source": source,
        "hashes": [],
        "author": author,
        "phase": int(entry.get("phase", 0)),
    }
    if entry.get("notes"):
        mod["instructions"] = " ".join(entry["notes"].split())
    return mod


def build_rules(mods: list[dict]) -> list[dict]:
    known = {m["name"] for m in mods}
    rules: list[dict] = []
    for entry in mods:
        for other in entry.get("rules", {}).get("after", []):
            if other not in known:
                raise BuildError(
                    f"{entry['name']}: 'after' rule references {other!r}, "
                    "which is not in the modlist."
                )
            rules.append(
                {
                    "source": {"name": entry["name"]},
                    "type": "after",
                    "reference": {"name": other},
                }
            )
    return rules


def install_instructions(spec: dict) -> str:
    lines = [
        spec["collection"]["description"].strip(),
        "",
        "MANUAL PREREQUISITES — install these before running the collection:",
        "",
    ]
    for pre in spec.get("prerequisites", []):
        lines.append(f"* {pre['name']}")
        if pre.get("url"):
            lines.append(f"  {pre['url']}")
        if pre.get("notes"):
            lines.append(f"  {' '.join(pre['notes'].split())}")
        lines.append("")
    return "\n".join(lines).strip()


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--modlist", type=Path, default=HERE / "modlist.yaml")
    parser.add_argument("--out", type=Path, default=HERE / "build")
    parser.add_argument(
        "--offline",
        action="store_true",
        help="Skip the Nexus API. Emits placeholder file ids — structure only, not installable.",
    )
    parser.add_argument(
        "--api-key",
        default=os.environ.get("NEXUS_API_KEY"),
        help="Nexus personal API key. Defaults to $NEXUS_API_KEY.",
    )
    args = parser.parse_args()

    spec = yaml.safe_load(args.modlist.read_text())
    meta = spec["collection"]
    domain = meta["domain"]
    entries = spec["mods"]

    resolved_by_id: dict[int, dict] = {}
    problems: list[str] = []

    if not args.offline:
        if not args.api_key:
            print(
                "error: no API key. Set NEXUS_API_KEY, pass --api-key, or use "
                "--offline for a structure-only build.",
                file=sys.stderr,
            )
            return 2
        client = NexusClient(args.api_key, domain)
        for i, entry in enumerate(entries, 1):
            mod_id = entry["mod_id"]
            print(f"[{i}/{len(entries)}] {entry['name']} (mods/{mod_id})", flush=True)
            try:
                info = client.mod_info(mod_id)
                check_name(entry["name"], entry, info, problems)
                file = pick_file(client.main_files(mod_id), entry["name"])
            except BuildError as exc:
                problems.append(str(exc))
                continue
            resolved_by_id[mod_id] = {"info": info, "file": file}
        if client.remaining is not None:
            print(f"Nexus API requests remaining today: {client.remaining}")

    if problems:
        print("\nProblems found:", file=sys.stderr)
        for p in problems:
            print(f"  - {p}", file=sys.stderr)
        print(
            "\nRefusing to write a manifest with unverified entries. Fix modlist.yaml "
            "(or drop the offending mods) and re-run.",
            file=sys.stderr,
        )
        return 1

    collection = {
        "info": {
            "author": meta.get("author") or "",
            "authorUrl": meta.get("author_url", ""),
            "name": meta["name"],
            "description": meta["description"].strip(),
            "domainName": domain,
            "gameVersions": [str(v) for v in meta.get("game_versions", [])],
            "installInstructions": install_instructions(spec),
        },
        "mods": [build_mod_entry(e, domain, resolved_by_id.get(e["mod_id"])) for e in entries],
        "modRules": build_rules(entries),
        "loadOrder": [],
        "collectionConfig": {
            "recommendNewProfile": bool(meta.get("recommend_new_profile", True))
        },
    }

    args.out.mkdir(parents=True, exist_ok=True)
    manifest = args.out / "collection.json"
    manifest.write_text(json.dumps(collection, indent=2) + "\n")

    slug = re.sub(r"[^a-z0-9]+", "-", meta["name"].lower()).strip("-")
    archive = args.out / f"{slug}.zip"
    with zipfile.ZipFile(archive, "w", zipfile.ZIP_DEFLATED) as zf:
        zf.write(manifest, "collection.json")

    required = sum(1 for m in collection["mods"] if not m["optional"])
    print(f"\nWrote {manifest}")
    print(f"Wrote {archive}")
    print(f"{len(collection['mods'])} mods ({required} required), {len(collection['modRules'])} ordering rules")
    if args.offline:
        print("\nOFFLINE BUILD — every fileId is 0. Re-run with an API key before importing.")
    return 0


if __name__ == "__main__":
    try:
        sys.exit(main())
    except BuildError as exc:
        print(f"error: {exc}", file=sys.stderr)
        sys.exit(1)
