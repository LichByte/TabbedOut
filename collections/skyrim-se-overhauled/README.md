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

Three opt-in overlays, stackable:
[`visuals`](#visuals-overlay) (+12), [`cinematics`](#cinematics-overlay) (+12),
[`adult`](#adult-overlay-ostim) (+20). Everything stacked is 85 mods.

```bash
python3 ../build_collection.py . --overlay visuals --overlay adult
```

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

## Mod Organizer 2 or Vortex?

**Recommendation: MO2, if you are running the overlays. Vortex if you are
running the base list alone and want it installed for you.**

The tension is real and worth stating plainly: collections are a **Vortex
feature**. The zip this repo builds only imports into Vortex. Choosing MO2 means
`modlist.yaml` becomes an install checklist rather than an installer — you get
the curation, the ordering rules and the notes, but you install by hand.

For the base list alone, that trade is not worth it. Vortex handles 41 mods
fine, has LOOT built in, and one-click install is a real saving.

Three things flip it once the overlays are in play:

1. **Overwrite chains stop being incidental.** SMIM → Skyland → Majestic
   Mountains, Lux → Lux Orbis → Lux Via → Embers XD, JK's before all the Lux
   mods, CBBE → 3BA → BodySlide. MO2's left pane makes that order *visible and
   draggable*; Vortex expresses the same thing as pairwise rules you resolve
   through a dialog, which is correct but much harder to audit at a glance.
2. **Three tools must run against the finished load order** — Nemesis,
   BodySlide and DynDOLOD. All three read the *virtual* file layout, and all
   three you will run repeatedly as you tweak. MO2's VFS presents tools exactly
   the view the game gets, and capturing their output back as a mod is a
   first-class workflow rather than something you arrange.
3. **You are iterating.** Four overlays in, this is a list being edited, not
   installed once. MO2 profiles let you keep a clean base and an
   everything-enabled profile side by side and switch between them.

If you go Vortex anyway, the collection installs and works — just budget time in
the conflict-rules dialog, and read the ordering rules in the modlists as your
guide to what the answers should be.

Either way the curation is the deliverable; the manager is how you consume it.

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

## Visuals overlay

The base list's visual tier is deliberately modest — five mods, no ENB — so it
stays cheap and conflict-free. This is the heavy pass.

```bash
python3 ../build_collection.py . --overlay visuals
```

**[Community Shaders](https://www.nexusmods.com/skyrimspecialedition/mods/86492)
is the headline.** It is a free, open-source, modular rendering framework built
on SKSE — the modern alternative to ENB, much cheaper for comparable results,
and unlike ENB it is Nexus-hosted so it can actually live in a collection. That
is why this overlay ships no ENB preset.

| Mod | Role |
|---|---|
| [Community Shaders](https://www.nexusmods.com/skyrimspecialedition/mods/86492) | Rendering framework; the ENB alternative |
| [Complex Parallax Materials](https://www.nexusmods.com/skyrimspecialedition/mods/95134) | Parallax with self-shadowing — stone and snow gain real depth |
| [Majestic Mountains](https://www.nexusmods.com/skyrimspecialedition/mods/11052) | Mountain rework; most of Skyrim's skyline |
| [Blended Roads](https://www.nexusmods.com/skyrimspecialedition/mods/8834) | Roads blend into terrain instead of ending at a seam |
| [JK's Skyrim](https://www.nexusmods.com/skyrimspecialedition/mods/6289) | All cities, towns and villages detailed; script-free |
| [JK's Patch Collection](https://www.nexusmods.com/skyrimspecialedition/mods/154077) *(optional)* | Patches for JK's against other mods |
| [Lux Orbis](https://www.nexusmods.com/skyrimspecialedition/mods/56095) | Exterior artificial lighting — the outdoor counterpart to Lux |
| [Lux Via](https://www.nexusmods.com/skyrimspecialedition/mods/63588) | Roads, paths and bridges |
| [Embers XD](https://www.nexusmods.com/skyrimspecialedition/mods/37085) | Fire and embers; has explicit Lux Orbis and CS support |
| [High Poly NPC Overhaul](https://www.nexusmods.com/skyrimspecialedition/mods/44155) + [Resources](https://www.nexusmods.com/skyrimspecialedition/mods/42768) | 1000+ vanilla NPCs on high-poly heads |
| [DynDOLOD 3](https://www.nexusmods.com/skyrimspecialedition/mods/68518) | Distant object and tree LOD — run last |

Four things worth knowing:

- **JK's must load before the Lux family.** Lux and Lux Orbis both ship JK's
  patches, and those patches have to win — tick the JK's options in both
  FOMODs. Being script-free, JK's is safe to add or remove mid-save, which is
  unusual for a city overhaul. It is mutually exclusive with Dawn of Skyrim,
  Expanded Towns and Cities and The Great Cities unless you go hunting for
  cross-patches.
- **Lux, Lux Orbis and Lux Via are complementary, not alternatives.** Same
  author. The base list's Lux does interiors only; Orbis does exterior
  artificial lighting and Via does roads and bridges.
- **High Poly NPC Overhaul ships FaceGen data only** — no meshes or textures. It
  inherits whatever body and skin you have, which is exactly why it composes
  with the adult overlay instead of fighting it.
- **DynDOLOD is phase 3 because you *run* it**, like Nemesis. Re-run it after
  anything that changes the worldspace. It is still labelled Alpha but has been
  the community standard for years; DynDOLOD 2 is the legacy branch.

This overlay costs real framerate. If you are near budget, install it in two
passes — rendering and lighting first, then textures — and check Whiterun and
the Rift between them. Those break first.

## Adult overlay (OStim)

```bash
python3 ../build_collection.py . --overlay adult
```

**Built on OStim Standalone rather than SexLab, and that choice is the whole
reason this overlay can exist.** SexLab and its animation packs are
LoversLab-only, so a SexLab list would be almost entirely manual installs — the
same wall the Fallout 4 adult overlay hits. Much of the OStim ecosystem is
Nexus-hosted, so most of it fits in a collection.

Twenty mods in four layers:

**Dependencies** OStim needs that the base list doesn't already have —
[XPMSSE](https://www.nexusmods.com/skyrimspecialedition/mods/1988) (extended
skeleton, the Skyrim equivalent of ZeX),
[JContainers SE](https://www.nexusmods.com/skyrimspecialedition/mods/16495),
[Mfg Fix](https://www.nexusmods.com/skyrimspecialedition/mods/11669) (facial
expressions — without it faces stay blank through every scene). SKSE, Nemesis,
SkyUI, Address Library, ConsoleUtilSSE, PapyrusUtil and RaceMenu are already
there.

**Framework** —
[OStim Standalone](https://www.nexusmods.com/skyrimspecialedition/mods/98163).

**Body** —
[CBBE](https://www.nexusmods.com/skyrimspecialedition/mods/198) →
[CBBE 3BA](https://www.nexusmods.com/skyrimspecialedition/mods/30174) →
[Settings Loader](https://www.nexusmods.com/skyrimspecialedition/mods/56875),
plus [BodySlide](https://www.nexusmods.com/skyrimspecialedition/mods/201) at
phase 3. Note 3BA sits **on top of** CBBE rather than replacing it — install
CBBE first and let 3BA overwrite. The
[SFW edition](https://www.nexusmods.com/skyrimspecialedition/mods/74257) is
listed optional-and-off purely so the mutual exclusivity is explicit.

**Physics** —
[CBPC](https://www.nexusmods.com/skyrimspecialedition/mods/21224) for body,
[FSMP](https://www.nexusmods.com/skyrimspecialedition/mods/57339) for cloth and
hair. They coexist; the usual split is SMP for cloth, CBPC for body. **FSMP
loaded before 3BA causes permanently jittery physics** — the ordering rules
encode this.

**Content and scene behaviour** —
[OSTEM animations](https://www.nexusmods.com/skyrimspecialedition/mods/105849),
[Stage Flow](https://www.nexusmods.com/skyrimspecialedition/mods/183299),
[Sequential Stage Playback](https://www.nexusmods.com/skyrimspecialedition/mods/173926),
[Post-Scene Aftercare](https://www.nexusmods.com/skyrimspecialedition/mods/166148),
[Social Consequences](https://www.nexusmods.com/skyrimspecialedition/mods/167220)
(NPCs react to what they witness),
[Dynamic Dialogue Framework](https://www.nexusmods.com/skyrimspecialedition/mods/185024),
plus three optional extras.

### Two steps that are not optional

1. **Re-run Nemesis** with the OStim patches ticked. Scenes that start and
   instantly end, or actors that T-pose, are almost always a missing Nemesis run.
2. **Build everything in BodySlide** against the 3BA preset with *Build Morphs*
   ticked. Skipping the morphs checkbox is why bodies ignore their sliders.

### Nexus account setup

Adult content is hidden by default. Enable it in
[preferences](https://www.nexusmods.com/users/myaccount?tab=preferences) and
complete age verification if you are in the UK or EU. Until then the API 404s
every mod in this overlay and the build fails on the first lookup — the
builder's 404 message says so.

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

- **An ENB preset.** The [visuals overlay](#visuals-overlay) uses Community
  Shaders instead — cheaper, and Nexus-hosted so it can be a collection entry.
  Add an ENB yourself if you prefer it, once the base list is stable.
- **SexLab and its ecosystem.** The [adult overlay](#adult-overlay-ostim) uses
  OStim precisely because SexLab is LoversLab-only and could not be collected.
- **Immersive Citizens.** Long-standing conflicts with city overhauls and other
  AI packages; not worth the patch burden on a list meant to just work.

## Adding a mod

Same schema as the Fallout 4 list — see
[that README](../commonwealth-overhauled/README.md#adding-a-mod). `mod_id` is
the number in the Nexus URL, and `expected_name_contains` is the escape hatch
when a page title carries version tags the name check would trip on.
