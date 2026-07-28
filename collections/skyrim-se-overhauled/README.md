# Skyrim Reforged

A full-overhaul Skyrim Special Edition collection: 41 mods across stability,
frameworks, fixes, modernised combat, progression, content and visuals.

Same machinery as [Commonwealth Overhauled](../commonwealth-overhauled) — the
curated list lives in [`modlist.yaml`](modlist.yaml) and the shared
[`../build_collection.py`](../build_collection.py) turns it into a
Vortex-importable archive.

```bash
pip install -r ../requirements.txt
export NEXUS_API_KEY=...      # nexusmods.com/users/myaccount?tab=api
python3 ../build_collection.py .
```

Add `--overlay cinematics` for the
[screenarchery/animation overlay](#cinematics-overlay).

Every `mod_id` was verified against a live Nexus URL, and the builder re-checks
each one at build time — a renamed, hidden or deleted mod fails the build rather
than shipping broken.

## Game version: 1.6.1170

**This collection targets 1.6.1170, the current Steam build.** That is a change
from the advice most older guides give, and the reason is
[CommonLibSSE-NG](https://ng.commonlib.dev/): modern SKSE plugins now compile a
single DLL that serves SE, AE and VR, so the long-standing reason to sit on
1.5.97 has largely evaporated. A mod advertising 1170 support is safe.

What it costs: some older SKSE plugins never made the jump and are 1.5.97-only
forever. If you are following a guide built around one of those, downgrade with
the [Best of Both Worlds patcher](https://www.nexusmods.com/skyrimspecialedition/mods/169962)
first and expect several mods here to need different files. You cannot mix —
a 1.5.97 DLL will not load on 1.6.x and vice versa.

Either way, **stop Steam auto-updating.** One silent update past your target
breaks every SKSE plugin at once. Set the game to "Only update this game when I
launch it" and mark `steamapps/appmanifest_489830.acf` read-only.

## Before you install

1. **SKSE64** from [skse.silverlock.org](https://skse.silverlock.org/), matching
   1.6.1170 exactly. Loose files into the Skyrim SE root, launch via
   `skse64_loader.exe`. It cannot be mod-manager-managed.
2. **VC++ 2019/2022 x64 redistributable.**
3. **SSE Engine Fixes part 2** is a manual drop into the game root, separate
   from the mod-manager-installed part. This is the single most-missed step in
   Skyrim modding — the mod silently does half its job without it.
4. Then import the built zip in Vortex: **Collections → Add Collection → From
   File**.
5. Run **LOOT**, then check the three things under
   [Load order](#load-order-what-actually-matters).
6. Run **Nemesis** — see below. Not optional.
7. **Start a new game.**

### A note on Vortex vs MO2

Collections are a Vortex feature, so that is the supported path here. For a list
this size MO2 is still the better tool, because its explicit left-pane ordering
makes texture and lighting overwrites *visible* rather than implicit — and this
list has real overwrite chains (SMIM → Skyland, Obsidian → Lux). If you prefer
MO2, use `modlist.yaml` as the reference and install by hand.

## Run Nemesis, every time

[Nemesis](https://www.nexusmods.com/skyrimspecialedition/mods/60033) is a
behaviour patcher, not a content mod. It is phase 3 because you run it *after*
everything is installed:

1. Launch Nemesis through your mod manager
2. Tick every patch it lists
3. **Update Engine**, then **Launch Nemesis Behavior Engine**

**Re-run it every time you add or remove an animation mod.** TDM, Precision,
Valhalla, MCO and DMCO all register behaviours through it. "My dodge does
nothing" and "my attacks don't play" are almost always a Nemesis run that never
happened, or one that happened before the last mod was added.

In Nemesis, **TDM must be checked before DMCO** — the wrong order gives you a
dodge that fires the animation but doesn't move you.

## The list

Phases are install barriers: Vortex finishes each before starting the next.

### Phase 0 — frameworks (14)

Skyrim's modern ecosystem is unusually framework-heavy. SPID and KID matter more
than their descriptions suggest: they distribute spells, perks, items and
keywords through *config files* rather than edited NPC records, so mods using
them stop fighting over the same entries. That is most of why a 40-mod list is
manageable at all.

| Mod | Why |
|---|---|
| [Address Library](https://www.nexusmods.com/skyrimspecialedition/mods/32444) | Version-independent offsets for SKSE plugins |
| [SSE Engine Fixes](https://www.nexusmods.com/skyrimspecialedition/mods/17230) | The most important stability mod; read the part-2 note above |
| [Crash Logger](https://www.nexusmods.com/skyrimspecialedition/mods/59818) | Readable crash logs instead of silent desktop drops |
| [po3's Papyrus Extender](https://www.nexusmods.com/skyrimspecialedition/mods/22854) | Extra Papyrus functions; hard dependency for many mods |
| [po3's Tweaks](https://www.nexusmods.com/skyrimspecialedition/mods/51073) | Engine fixes and tuning alongside Engine Fixes |
| [PapyrusUtil SE](https://www.nexusmods.com/skyrimspecialedition/mods/13048) | Script data storage |
| [ConsoleUtilSSE](https://www.nexusmods.com/skyrimspecialedition/mods/24858) | Console commands from Papyrus |
| [SPID](https://www.nexusmods.com/skyrimspecialedition/mods/36869) | Conflict-free distribution to NPCs |
| [KID](https://www.nexusmods.com/skyrimspecialedition/mods/55728) | The same, for keywords |
| [Base Object Swapper](https://www.nexusmods.com/skyrimspecialedition/mods/60805) | Config-driven object swapping |
| [SkyUI](https://www.nexusmods.com/skyrimspecialedition/mods/12604) | The UI, and the host for every MCM |
| [MCM Helper](https://www.nexusmods.com/skyrimspecialedition/mods/53000) | Persistent MCM settings and hotkeys |
| [Open Animation Replacer](https://www.nexusmods.com/skyrimspecialedition/mods/92109) | Modern DAR successor; most animation mods need it |
| [RaceMenu](https://www.nexusmods.com/skyrimspecialedition/mods/19080) | Character creation, and a framework others hook into |

### Phase 1 — fixes and performance (6)

[USSEP](https://www.nexusmods.com/skyrimspecialedition/mods/266),
[Bug Fixes SSE](https://www.nexusmods.com/skyrimspecialedition/mods/33261),
[Scrambled Bugs](https://www.nexusmods.com/skyrimspecialedition/mods/43532),
[Papyrus Tweaks NG](https://www.nexusmods.com/skyrimspecialedition/mods/77779),
[SSE Display Tweaks](https://www.nexusmods.com/skyrimspecialedition/mods/34705),
and [Skyrim Souls RE](https://www.nexusmods.com/skyrimspecialedition/mods/27859)
*(optional)*.

Display Tweaks is not cosmetic — Skyrim's physics is coupled to framerate, and
above 60 FPS things start flying apart without it.

### Phase 2 — combat (5)

[True Directional Movement](https://www.nexusmods.com/skyrimspecialedition/mods/51614)
for 360° movement and target lock,
[Precision](https://www.nexusmods.com/skyrimspecialedition/mods/72347) for real
melee collisions, and
[Valhalla Combat](https://www.nexusmods.com/skyrimspecialedition/mods/64741) for
stamina-driven attacks, executions and timed blocking. TDM and Precision share
an author and are built to work together.

[MCO/ADXP](https://www.nexusmods.com/skyrimspecialedition/mods/117115) and
[DMCO](https://www.nexusmods.com/skyrimspecialedition/mods/175129) are optional
and **off by default** — MCO needs its own animation packs to be worth having
and changes the feel of the game substantially. Enable both or neither.

### Phase 2 — progression (6)

[Ordinator](https://www.nexusmods.com/skyrimspecialedition/mods/1137),
[Apocalypse](https://www.nexusmods.com/skyrimspecialedition/mods/1090),
[their bridge patch](https://www.nexusmods.com/skyrimspecialedition/mods/52674),
[Imperious](https://www.nexusmods.com/skyrimspecialedition/mods/1315),
[Andromeda](https://www.nexusmods.com/skyrimspecialedition/mods/14910), and
[Alternate Start](https://www.nexusmods.com/skyrimspecialedition/mods/272).

Four of those are EnaiSiaion mods, chosen deliberately — they are designed as a
suite, so perks, spells, races and standing stones reinforce each other instead
of each pulling balance in its own direction. The bridge patch is what makes
Ordinator's perk trees actually grant Apocalypse spells.

### Phase 2 — content (4)

[Legacy of the Dragonborn](https://www.nexusmods.com/skyrimspecialedition/mods/11802)
— a museum you fill across the entire game, which quietly reframes every other
mod's loot as something worth keeping.
[Interesting NPCs](https://www.nexusmods.com/skyrimspecialedition/mods/29194) —
250+ voiced characters with real dialogue trees.
[Wyrmstooth](https://www.nexusmods.com/skyrimspecialedition/mods/45565) — a
DLC-sized island. Plus
[the LotD/3DNPC patch](https://www.nexusmods.com/skyrimspecialedition/mods/39834).

### Phase 2 — visuals (5)

[SMIM](https://www.nexusmods.com/skyrimspecialedition/mods/659) →
[Skyland AIO](https://www.nexusmods.com/skyrimspecialedition/mods/34179) →
[Realistic Water Two](https://www.nexusmods.com/skyrimspecialedition/mods/2182),
then [Obsidian Weathers](https://www.nexusmods.com/skyrimspecialedition/mods/12125)
and [Lux](https://www.nexusmods.com/skyrimspecialedition/mods/43158).

That arrow is an overwrite order, not just an install order — SMIM supplies
geometry, Skyland supplies textures over it.

No ENB. It is where the remaining framerate goes; add one once the base list is
confirmed stable.

## Cinematics overlay

Opt-in, in [`modlist-cinematics.yaml`](modlist-cinematics.yaml). Twelve mods for
composing and capturing shots, plus the animation plumbing that makes custom
animations play correctly.

```bash
python3 ../build_collection.py . --overlay cinematics
```

**The recorder is not a mod.** Nothing in Skyrim's engine captures video — that
is OBS or ShadowPlay. These mods give you a controllable camera, posable actors
and a clean frame; the capture is external. Likewise ENB/ReShade, which is where
real depth of field comes from, ships from enbdev.com rather than Nexus and so
can't be a collection entry.

| Mod | Role |
|---|---|
| [Photo Mode](https://www.nexusmods.com/skyrimspecialedition/mods/91701) | The centrepiece — grids, FOV, roll, frozen time, weather, expressions, actor positioning |
| [SmoothCam](https://www.nexusmods.com/skyrimspecialedition/mods/41252) | Frame-interpolated camera; vanilla's snapping reads as cheap on video |
| [Improved Camera SE](https://www.nexusmods.com/skyrimspecialedition/mods/93962) | Real first-person body; handles its own SmoothCam conflicts |
| [Poser Hotkeys Plus](https://www.nexusmods.com/skyrimspecialedition/mods/17743) | Pose playback with search and frame stepping |
| [Additional Expressions Project](https://www.nexusmods.com/skyrimspecialedition/mods/72337) | Expression presets for the poser menu |
| [Conditional Expressions Extended](https://www.nexusmods.com/skyrimspecialedition/mods/91438) | Faces stop going blank between scripted beats |
| [Expressive Facial Animation](https://www.nexusmods.com/skyrimspecialedition/mods/19181) F / [M](https://www.nexusmods.com/skyrimspecialedition/mods/19532) | Wider, less rubbery morph range |
| [Animation Motion Revolution](https://www.nexusmods.com/skyrimspecialedition/mods/50258) | Fixes displacement mismatch — without it, travelling animations slide |
| [EVG Animated Traversal](https://www.nexusmods.com/skyrimspecialedition/mods/63232) | Vaulting, ladders, cramped-gap traversal |
| [Goetia Animations](https://www.nexusmods.com/skyrimspecialedition/mods/68625) *(optional)* | Male locomotion replacement; take the OAR file |
| [Pandora Behaviour Engine](https://www.nexusmods.com/skyrimspecialedition/mods/133232) *(optional)* | Faster Nemesis replacement — see below |

**Poser Hotkeys ships no poses.** It is a player for pose packs you add
separately, most of which live off-Nexus.

**Pandora vs Nemesis.** Pandora does the same job far faster and is backwards
compatible with Nemesis and FNIS patches — which matters here, because this
overlay means re-running the behaviour engine constantly. It is optional and off
by default, and encoded as `conflicts_with` Nemesis: run one, never both. If you
switch, disable Nemesis in the base list. The builder will reject a manifest
where both are required.

## Load order: what actually matters

LOOT gets most of it. Three it will not reliably handle:

1. **USSEP immediately after the DLC ESMs** — top of the order.
2. **Alternate Start at the very bottom.** It has to see every other mod's
   world edits to place its starts correctly.
3. **Lux after every mod that edits interiors.** Lux rebuilds interior lighting;
   anything that touches the same cells afterwards undoes it in those cells.
   Check its patch list against your load order.

## Known conflicts

- **Ordinator vs any other perk overhaul** — Vokrii, SPERG, Adamant. One only.
  They all rewrite the same eighteen trees.
- **Obsidian vs any other weather mod** — Vivid, Cathedral, Azurite, NAT. One only.
- **Lux vs ELFX / Relighting Skyrim** — one interior lighting overhaul only.
- **TDM before DMCO in Nemesis** — see above.
- **MCO + Valhalla** — they coexist, but both touch attack timing and stamina.
  If you enable MCO, expect to spend time in both MCMs balancing them.

## Deliberately not included

- **An ENB preset.** See above.
- **Majestic Mountains and JK's Skyrim.** Both are excellent and both would
  normally be here; neither mod id could be confirmed against a live Nexus URL
  during curation, and the builder rejects unverified entries by design. Add
  them by hand — grab the id from the URL and append to `modlist.yaml`.
- **Body/adult mods.** The Fallout 4 collection has an
  [opt-in overlay](../commonwealth-overhauled/README.md#adult-and-body-mods) for
  this; nothing equivalent is set up here yet. Same site-split constraint would
  apply — CBBE/3BA and BodySlide are on Nexus, most animation content is not.
- **Immersive Citizens.** Long-standing conflicts with city overhauls and other
  AI packages; not worth the patch burden on a list meant to just work.

## Adding a mod

Same schema as the Fallout 4 list — see
[that README](../commonwealth-overhauled/README.md#adding-a-mod). `mod_id` is
the number in the Nexus URL, and `expected_name_contains` is the escape hatch
when a page title carries version tags the name check would trip on.
