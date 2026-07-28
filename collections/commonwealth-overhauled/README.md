# Commonwealth Overhauled

A full-overhaul Fallout 4 collection: 56 mods across stability, fixes, UI,
progression, combat, settlements, quests and visuals — plus the tooling to turn
the curated list into a Vortex-importable collection archive.

- [`modlist.yaml`](modlist.yaml) — the curated list. Source of truth.
- [`modlist-adult.yaml`](modlist-adult.yaml) — opt-in adult/body overlay, off by
  default. See [Adult and body mods](#adult-and-body-mods).
- [`build_collection.py`](build_collection.py) — resolves it against the Nexus
  API and emits `collection.json` + a zip.

## Why it is built this way

A Nexus collection is normally produced by installing everything in Vortex and
hitting export, which means the list only exists inside one person's Vortex
profile. Here the list is a reviewable YAML file, and the manifest is generated
from it. You can diff it, PR it, and rebuild it when mods update.

The catch: a collection entry needs a **file id**, not just a mod id, and file
ids change every time an author uploads. So `build_collection.py` looks up the
current main file for each mod at build time. Every `mod_id` in `modlist.yaml`
was verified against a live Nexus URL, and the builder re-checks that the mod's
name on Nexus still matches the list — if a mod is renamed, hidden or deleted,
the build fails instead of silently shipping a broken entry.

## Build it

```bash
pip install -r requirements.txt
export NEXUS_API_KEY=...      # nexusmods.com/users/myaccount?tab=api
python3 build_collection.py
```

Output lands in `build/`. Without a key, `--offline` emits the same structure
with placeholder file ids — useful for reviewing the manifest, not installable.

Add `--adult` to merge the adult/body overlay. It writes a separately named
collection and zip, so the two builds sit side by side rather than overwriting
each other.

Roughly 110 API calls per build (two per mod), or 130 with `--adult`. The free
daily quota is 2,500, so this is not a concern unless you are looping it.

Three things the builder refuses to emit rather than warn about: a mod whose
Nexus name no longer matches the list, two mutually exclusive mods both marked
required, and the same mod id appearing twice.

## Install it

1. **Get the game to 1.10.163.** See [Game version](#game-version) below.
2. **Install F4SE and the other manual prerequisites** — listed in
   `modlist.yaml` under `prerequisites` and carried into the manifest's install
   instructions. F4SE cannot be mod-manager-managed; it goes in the game root.
3. In Vortex: **Collections → Add Collection → From File**, point it at
   `build/commonwealth-overhauled.zip`.
4. Let it install. Answer the FOMOD prompts using the notes in the list.
5. Run **LOOT** (Vortex has it built in), then read
   [Load order](#load-order-what-actually-matters) — LOOT gets PRP wrong often
   enough that you should check it by hand.
6. Run **BodySlide** and build your outfits with *Build Morphs* ticked.
7. **Start a new game.** Sim Settlements 2 and Start Me Up both require it.

## Publish it to Nexus

The generated zip is importable but not publishable as-is — Nexus wants file
hashes and a screenshot set that only Vortex can produce. The path is:

1. Import the generated zip into Vortex and let it install everything.
2. Adjust anything you want changed (FOMOD choices, load order, ini tweaks).
3. **Collections → your collection → Publish.** Vortex re-exports with real
   hashes, your FOMOD choices baked in, and uploads it.

Treat this repo as the thing you edit and the Vortex profile as the build
artifact, not the other way around.

## Game version

Fallout 4's April 2024 "next-gen" update (1.10.980+) moved every code offset and
broke F4SE and every F4SE plugin. Most of the large, long-maintained F4SE mods
never fully moved across, so **this collection targets 1.10.163**.

Use [Simple Fallout 4 Downgrader](https://www.nexusmods.com/fallout4/mods/81933)
— one click, no Steam credentials — and set Steam to *Only update this game when
I launch it* so it does not silently re-upgrade.

If you insist on next-gen: Buffout 4 NG, Address Library and PRP all have
next-gen builds, but Place Everywhere, Baka ScrapHeap and several others are
version-locked. Expect to drop mods and hand-resolve.

## The list

Phases are install order barriers — Vortex finishes each phase before starting
the next.

### Phase 0 — frameworks (10)

The plumbing. Nothing else works without it.

| Mod | Why |
|---|---|
| [Address Library for F4SE Plugins](https://www.nexusmods.com/fallout4/mods/47327) | Version-independent offsets every F4SE plugin depends on |
| [xSE PluginPreloader F4](https://www.nexusmods.com/fallout4/mods/33946) | Loads plugins before game init; required by Buffout |
| [Buffout 4 NG](https://www.nexusmods.com/fallout4/mods/64880) | Engine fixes + the crash logger everyone troubleshoots with |
| [PrivateProfileRedirector F4](https://www.nexusmods.com/fallout4/mods/33947) | Caches INI reads; big startup win |
| [Baka ScrapHeap](https://www.nexusmods.com/fallout4/mods/46340) | Expands the papyrus allocation SS2 blows through |
| [Mod Configuration Menu](https://www.nexusmods.com/fallout4/mods/21497) | In-game settings for most of the list |
| [MCM Booster](https://www.nexusmods.com/fallout4/mods/56997) | MCM load time, near-instant |
| [HUDFramework](https://www.nexusmods.com/fallout4/mods/20309) | Conflict-free HUD widgets |
| [Workshop Framework](https://www.nexusmods.com/fallout4/mods/35004) | Rewrites workshop scripts; SS2 dependency |
| [Canary Save File Monitor](https://www.nexusmods.com/fallout4/mods/44949) | Warns when a mod loses data in your save |

### Phase 1 — fixes and performance (8)

| Mod | Why |
|---|---|
| [Unofficial Fallout 4 Patch](https://www.nexusmods.com/fallout4/mods/4598) | The baseline bug fix pack |
| [Community Fixes Merged](https://www.nexusmods.com/fallout4/mods/74945) | Dozens of fixes UFO4P does not cover |
| [Previsibines Repair Pack](https://www.nexusmods.com/fallout4/mods/46403) | Rebuilt precombines — the biggest framerate win available |
| [Weapon Debris Crash Fix](https://www.nexusmods.com/fallout4/mods/48078) | The NVIDIA weapon-debris hard crash |
| [High FPS Physics Fix](https://www.nexusmods.com/fallout4/mods/44798) | Decouples physics from framerate |
| [Long Loading Times Fix](https://www.nexusmods.com/fallout4/mods/73469) | Removes a busy-wait in the loader |
| [Insignificant Object Remover](https://www.nexusmods.com/fallout4/mods/9835) | Strips tiny clutter meshes |
| [Unlimited Survival Mode](https://www.nexusmods.com/fallout4/mods/26163) *(optional)* | Restores console/saving in Survival |

### Phase 2 — interface (7)

The [FallUI suite](https://www.nexusmods.com/fallout4/mods/51813) —
[Icon Library](https://www.nexusmods.com/fallout4/mods/60579),
[HUD](https://www.nexusmods.com/fallout4/mods/51813),
[Inventory](https://www.nexusmods.com/fallout4/mods/48758),
[Workbench](https://www.nexusmods.com/fallout4/mods/49300),
[Map](https://www.nexusmods.com/fallout4/mods/49920),
[Sleep and Wait](https://www.nexusmods.com/fallout4/mods/49070) — plus
[Extended Dialogue Interface](https://www.nexusmods.com/fallout4/mods/27216),
which removes the four-option dialogue cap and shows full response text.

XDI over [Full Dialogue Interface](https://www.nexusmods.com/fallout4/mods/1235):
they do the same job and cannot coexist. XDI is the one quest mods patch against.

### Phase 2 — characters (3 + 1 in phase 3)

[LooksMenu](https://www.nexusmods.com/fallout4/mods/12631),
[Looks Menu Customization Compendium](https://www.nexusmods.com/fallout4/mods/24830),
[CBBE](https://www.nexusmods.com/fallout4/mods/15), and
[BodySlide](https://www.nexusmods.com/fallout4/mods/25) in phase 3 because you
run it as a tool against the finished load order.

CBBE's FOMOD has a *Vanilla Outfits* option if you want it SFW.

### Phase 2 — progression and combat (10)

| Mod | Why |
|---|---|
| [Be Exceptional](https://www.nexusmods.com/fallout4/mods/28222) | Skill system + reworked perk chart. The progression backbone |
| [CURSE - Combat Overhaul](https://www.nexusmods.com/fallout4/mods/74413) | Script-free lethality rebalance |
| [Munitions](https://www.nexusmods.com/fallout4/mods/66051) | Real ammunition types and economy |
| [Munitions Patch Repository](https://www.nexusmods.com/fallout4/mods/66052) | Patches for whichever weapon mods you add |
| [See Through Scopes](https://www.nexusmods.com/fallout4/mods/9476) | Real optics instead of the overlay |
| [Tactical Reload](https://www.nexusmods.com/fallout4/mods/49444) | Magazine-retention reloads |
| [Everyone's Best Friend](https://www.nexusmods.com/fallout4/mods/13459) | Dogmeat + a human companion without Lone Wanderer |
| [Survival Options](https://www.nexusmods.com/fallout4/mods/14650) *(optional)* | Per-setting control over Survival's rules |
| [Survival Options + EBF Merged Patch](https://www.nexusmods.com/fallout4/mods/45109) *(optional)* | Mandatory if you run both of the above |
| [Start Me Up Redux](https://www.nexusmods.com/fallout4/mods/56984) | Alternate start — skip the vault intro |

CURSE was picked over [Better Locational Damage](https://www.nexusmods.com/fallout4/mods/3815)
and [Lunar Fallout Overhaul](https://www.nexusmods.com/fallout4/mods/34769)
because both of those are near-total conversions that occupy the same perk and
levelled-list space as Be Exceptional and Munitions. If you would rather run
Lunar or BLD as your single overhaul, drop Be Exceptional and CURSE — do not
stack them.

### Phase 2 — settlements (9)

[Sim Settlements 2](https://www.nexusmods.com/fallout4/mods/47976) +
[Chapter 2](https://www.nexusmods.com/fallout4/mods/55817) +
[Chapter 3](https://www.nexusmods.com/fallout4/mods/73394) is the backbone —
a settlement overhaul and a long voiced questline in one. The
[Previsibines Expansion Pack](https://www.nexusmods.com/fallout4/mods/57947)
restores previs in the cells it edits and is not optional if you care about
framerate.

Plus [Homemaker](https://www.nexusmods.com/fallout4/mods/1478),
[Settlement Objects Expansion Pack](https://www.nexusmods.com/fallout4/mods/10075),
[Place Everywhere](https://www.nexusmods.com/fallout4/mods/9424), and two
optional SS2 addon packs
([Wasteland Venturers](https://www.nexusmods.com/fallout4/mods/48060),
[Junk Town 2](https://www.nexusmods.com/fallout4/mods/48271)).

### Phase 2 — quests (3)

[Atomic Radio and Tales from the Commonwealth](https://www.nexusmods.com/fallout4/mods/8704),
[Outcasts and Remnants](https://www.nexusmods.com/fallout4/mods/21469),
[America Rising 2](https://www.nexusmods.com/fallout4/mods/75767). All fully
voiced, all large, all long-maintained.

### Phase 2 — visuals (5)

[NAC X](https://www.nexusmods.com/fallout4/mods/46722) as the weather/lighting
baseline, [True Storms](https://www.nexusmods.com/fallout4/mods/4472) layered on
with the [mandatory compatibility patch](https://www.nexusmods.com/fallout4/mods/60884),
[Enhanced Lights and FX](https://www.nexusmods.com/fallout4/mods/13596) for
interiors, and [Vivid Fallout AIO](https://www.nexusmods.com/fallout4/mods/25714)
for textures — packed in a BA2 rather than loose, which costs far less streaming
performance than most texture packs.

No ENB. On a list this heavy an ENB is where the remaining framerate goes; add
one yourself once you have confirmed the base list is stable.

## Adult and body mods

Opt-in overlay in [`modlist-adult.yaml`](modlist-adult.yaml), built with
`--adult`. Eight Nexus-hosted mods: a skeleton, an animation framework, body
replacers, skin textures and physics. The base collection stays SFW — nothing
here is merged unless you ask for it.

```bash
python3 build_collection.py --adult
```

### The site split, which decides everything

Fallout 4's adult ecosystem lives on two sites, and **a Vortex collection can
only reference Nexus-hosted files.** So the overlay splits along that line:

| On Nexus — can be in the collection | On LoversLab — manual install only |
|---|---|
| [AAF](https://www.nexusmods.com/fallout4/mods/31304) (the framework) | Every AAF animation pack — Atomic Lust, Leito, SavageCabbage, Farelle |
| [ZeX - ZaZ Extended Skeleton](https://www.nexusmods.com/fallout4/mods/36702) | [Fusion Girl](https://www.loverslab.com/) (female body) |
| [Enhanced Vanilla Bodies](https://www.nexusmods.com/fallout4/mods/22110) | BodyTalk3 (male body) |
| [Atomic Beauty](https://www.nexusmods.com/fallout4/mods/12406) *(optional)* | |
| [Valkyr Face and Body Textures](https://www.nexusmods.com/fallout4/mods/3841) | |
| [Lovely Skin Complex](https://www.nexusmods.com/fallout4/mods/29722) *(optional)* | |
| [CBP Physics](https://www.nexusmods.com/fallout4/mods/39088) | |
| [MTM Physics Preset](https://www.nexusmods.com/fallout4/mods/39195) | |

The right-hand column is listed in the overlay's `prerequisites` and carried
into the manifest's install instructions, the same way F4SE is. **AAF ships with
no animations** — it is a player, not content. Installed alone it loads and does
nothing, which is the single most common "AAF is broken" report.

### Nexus account setup

Adult content is hidden by default on new accounts. Enable it in
[preferences](https://www.nexusmods.com/users/myaccount?tab=preferences), and
complete age verification if you are in the UK or EU — Nexus gates adult
downloads behind it to comply with the Online Safety Act and Digital Services
Act. Until that is done, the API returns 404 for every mod in the overlay and
`--adult` fails on the first lookup. The builder's 404 message says so.

### One body, not several

Body replacers overwrite each other rather than merging. Pick **one female body
and one male body**, and rebuild every outfit in BodySlide against it — mixing
bodies gives you neck seams and armour clipping, and the cause is not obvious
when you hit it forty hours later.

- **Female:** CBBE (already in the base list) · Atomic Beauty · Fusion Girl
- **Male:** Enhanced Vanilla Bodies · BodyTalk3

Same for skin textures: Valkyr *or* Lovely Skin Complex, not both. The overlay
encodes these as `conflicts_with`, and the builder refuses to emit a manifest
where two mutually exclusive mods are both required — so the defaults are
CBBE + EVB + Valkyr, with the alternatives shipped as optional and off.

If you switch female body, disable CBBE in the base list too. The builder cannot
catch that one for you: it validates the manifest, not your Vortex profile.

## Load order: what actually matters

LOOT handles most of it. Three things it will not reliably get right:

1. **UFO4P immediately after the DLC ESMs.** Top of the order, before everything.
2. **PRP as close to the bottom as possible.** Any plugin that edits a cell
   *after* PRP invalidates previs for that cell — you get the framerate you were
   trying to fix, only worse, and it is invisible until you walk into downtown.
   If you add a mod later, check whether it needs a PRP patch.
3. **See Through Scopes after every weapon mod it patches.** Otherwise ADS
   alignment is off and animations break.

## Known conflicts

- **Tactical Reload + See Through Scopes** — if first-person reloads stop
  working but third-person is fine, enable the Tactical Reload patch in the STS
  MCM/holotape settings. This is the single most common support question for
  either mod.
- **Be Exceptional + CURSE** — both touch damage records. They are broadly
  compatible but not patched against each other; if combat feels wrong, load
  both in [FO4Edit](https://www.nexusmods.com/fallout4/mods/2737), find the
  overlapping records, and forward whichever you prefer into a small patch ESP.
- **ELFX + PRP** — ELFX edits interior cells. Check its files for a PRP patch
  and install it.
- **NAC X + True Storms** — broken without patch 60884. Not optional.
- **Survival Options + Everyone's Best Friend** — both edit the `hc_manager`
  quest. Without patch 45109 the "heal your companion" objective points at the
  wrong NPC or places no marker at all. Only bites if you enabled the optional
  Survival mods.

## Deliberately not included

- **Scrap Everything.** It destroys precombines wholesale, which undoes PRP and
  is the leading cause of "my modded game runs at 20 FPS in Boston."
- **AWKCR / Armorsmith Extended.** Long deprecated; the author asked people to
  stop using them and most modern armour mods no longer need them.
- **WET (Water Enhancement Textures).** Delisted from Nexus, so it cannot be
  resolved by the builder.
- **An ENB preset.** See above.
- **Settlers of the Commonwealth.** Genuinely good, and it cannot go in a Nexus
  collection. Only its satellites are on the Nexus — the SS2 leader packs, the
  [FaceGen data](https://www.nexusmods.com/fallout4/mods/90816) — with no base
  mod page; the author distributes it from 3dnpc.com. A collection can only
  reference Nexus-hosted files, so this one has to be a manual install
  regardless of how the list is built.

## Adding a mod

```yaml
  - name: Exact Name From The Nexus Page Title
    mod_id: 12345          # the number in the URL
    phase: 2
    category: Gameplay
    optional: false        # omit for required
    rules:
      after:
        - Some Other Mod In This List
    notes: >
      Shown to the installer as an instruction. Say what FOMOD options to pick
      and what it conflicts with.
```

If the Nexus page title has version tags or decoration in it, use
`expected_name_contains: Shorter Distinctive Substring` instead of fighting the
name check. Then rebuild.
