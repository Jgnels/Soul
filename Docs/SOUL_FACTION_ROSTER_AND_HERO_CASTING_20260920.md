# Soul Faction Roster + Hero Casting Audit — 2026-09-20

## Scope and evidence rules

- NON-UE audit only. No editor, cook, UBT/UAT or shader work was used.
- Ownership comes from the local asset catalog/current Fab database; public Fab pages are used only to resolve current listing UUIDs and pack contents.
- `EVIDENCE_BACKED` means the local payload/manifest proves the named mesh or animal path.
- `UE_VISUAL_CONFIRM` means ownership/content evidence is good but the final silhouette, scale, retarget, equipment or faction cohesion still needs UE.
- `PAYLOAD_PENDING` means ownership is established but exact local package paths cannot be audited until the payload is downloaded.
- Paragon characters remain heroes/Paragons, never regular unit-family substitutes.

## Preferred seven-unit rosters

### Humans

| Slot | Unit | Owned asset | Listing ID | Known package path | Recruit building | Combat role | Footprint | Flying | Status | Risk |
|---|---|---|---|---|---|---|---|---|---|---|
| fighter_1 | Castle Militia | Knights Pack / Knight_02 | 6907d3d8-d0db-43c0-bf9c-8ad43c70e7b3 | Content/Knights_Pack/Meshes/Knight_02/Mesh_UE5/Full/SKM_Knight_02_Full_01.uasset | Muster Yard | basic melee fighter | 1 hex | no | EVIDENCE_BACKED | Needs low-status material/loadout pass so it does not read as elite |
| ranged | Castle Archer (provisional) | Knight_03 body + Dark Fantasy Weapons bow | 6907d3d8-d0db-43c0-bf9c-8ad43c70e7b3 + 244607ad-e819-4320-bbc7-687b844b5bed | Content/Knights_Pack/Meshes/Knight_03/Mesh_UE5/Full/SKM_Knight_03_Full_01.uasset | Archery Range | ranged physical | 1 hex | no | UE_VISUAL_CONFIRM | No owned regular archer body was verified; local Pro Longbow Pack supplies bow actions, but retarget and bow/body cohesion require UE |
| fighter_2 | Spear Guard | Knights Pack / Knight_04 | 6907d3d8-d0db-43c0-bf9c-8ad43c70e7b3 | Content/Knights_Pack/Meshes/Knight_04/Mesh_UE5/Full_Mesh/SKM_Knight_04_Full_01.uasset | Spear Guardhouse | reach melee / anti-large | 1 hex | no | EVIDENCE_BACKED | Verify spear animation coverage and formation clipping |
| fighter_3 | Man-at-Arms | Knights Pack / Knight_05 | 6907d3d8-d0db-43c0-bf9c-8ad43c70e7b3 | Content/Knights_Pack/Meshes/Knight_05/Mesh_UE5/Full_Mesh/SKM_Knight_05_Full_01.uasset | Man-at-Arms Barracks | armored melee fighter | 1 hex | no | EVIDENCE_BACKED | Avoid the spikiest optional armor pieces |
| support_magic | Witch Adventurer (fallback: Fantasy Witch) | Fantasy Warriors Pack / Fantasy Witch | df8194f9-ec63-49f6-a885-6feb7a94fcb1 | payload/path pending | Witch Collegium | support caster / control | 1 hex | no | UE_VISUAL_CONFIRM | Exact Witch Adventurer ownership/path was not found; Fantasy Witch is the evidence-backed owned substitute |
| fighter_4 | Royal Guard | Knights Pack / Knight_01 | 6907d3d8-d0db-43c0-bf9c-8ad43c70e7b3 | Content/Knights_Pack/Meshes/Knight_01/Mesh_UE5/Knight_01_Full/SKM_Knight_01_Full_01.uasset | Royal Chapterhouse | elite melee / guard | 1 hex | no | EVIDENCE_BACKED | Prefer sword/shield or halberd; suppress banner in normal combat |
| beast | Griffon | Quadruped Fantasy Creatures / Griffon | 52d686b6-1180-4f26-901f-ce3c69a14767 | payload/path pending | Griffon Roost | flying shock beast | 2 hex | yes | UE_VISUAL_CONFIRM | Owned pack verified but current local payload path unresolved; confirm scale and tactical landing footprint |

**Alternates for weak slots**

- **ranged: Knight_02 or NPC Male body + Dark Fantasy Weapons bow** — More grounded body options; local longbow animations exist, but retarget/body cohesion still need UE `UE_VISUAL_CONFIRM`
- **support_magic: Fantasy Witch** — Best verified owned substitute for the intended Witch Adventurer `UE_VISUAL_CONFIRM`
- **fighter_1: Knight_03 low-ornament configuration** — Can separate militia/man-at-arms silhouettes if Knight_02 reads too elite `UE_VISUAL_CONFIRM`

### Dwarves

| Slot | Unit | Owned asset | Listing ID | Known package path | Recruit building | Combat role | Footprint | Flying | Status | Risk |
|---|---|---|---|---|---|---|---|---|---|---|
| fighter_1 | Stoneguard | Dwarfs Pack / Bedvar | 80643e93-5d54-438c-ae9d-7717242e5a16 | payload/path pending | Stoneguard Hall | armored melee fighter | 1 hex | no | UE_VISUAL_CONFIRM | Pack ownership and named character verified; current payload path unresolved |
| ranged | Crossbow Guard (provisional) | Dwarfs Pack / Orme | 80643e93-5d54-438c-ae9d-7717242e5a16 | payload/path pending | Crossbow Workshop | ranged physical | 1 hex | no | UE_VISUAL_CONFIRM | Source evidence does not prove a crossbow loadout; no crossbow animation set was verified locally |
| fighter_2 | Hammer Guard | Dwarfs Pack / Broddi | 80643e93-5d54-438c-ae9d-7717242e5a16 | payload/path pending | Hammer Hall | heavy melee / armor break | 1 hex | no | UE_VISUAL_CONFIRM | Named character and Epic-skeleton support verified; weapon silhouette still needs visual check |
| support_magic | Runesmith | Dwarfs Pack / Agvid | 80643e93-5d54-438c-ae9d-7717242e5a16 | payload/path pending | Rune Forge | support / rune magic | 1 hex | no | UE_VISUAL_CONFIRM | Forge/runesmith read is strong; spell presentation is game-side and needs qualification |
| fighter_3 | Runic Golem | Fantasy Characters Pack / Golem | f5816915-86d9-4bef-b8f3-921408ae240b | payload/path pending | Construct Foundry | large blocker / construct melee | 2 hex | no | UE_VISUAL_CONFIRM | Confirm it reads stone/forge rather than generic fantasy and does not overlap dragon scale |
| fighter_4 | King's Guard (provisional) | Fantasy Characters Pack / Dwarf | f5816915-86d9-4bef-b8f3-921408ae240b | payload/path pending | King's Guard Hall | elite melee / morale anchor | 1 hex | no | UE_VISUAL_CONFIRM | Use a regular dwarf body, not the unique Dwarf King; exact elite guard silhouette requires UE pick |
| beast | Mountain Dragon | Quadruped Fantasy Creatures / Mountain Dragon | 52d686b6-1180-4f26-901f-ce3c69a14767 | payload/path pending | Mountain Dragon Eyrie | apex flying large creature | 3 hex | yes | UE_VISUAL_CONFIRM | Owned pack verified but local payload path unresolved; confirm scale and aerial footprint |

**Alternates for weak slots**

- **ranged: Generic Dwarf from Fantasy Characters + future crossbow** — Body is owned but no owned crossbow was verified `UE_VISUAL_CONFIRM`
- **ranged: Orme as rune-thrower rather than crossbowman** — Preserves owned cast but changes building/role fiction; only use if design approves `UE_VISUAL_CONFIRM`
- **fighter_3: Generic Dwarf from Fantasy Characters** — Fallback if Golem art clashes with dwarf city `UE_VISUAL_CONFIRM`

### Vikings

| Slot | Unit | Owned asset | Listing ID | Known package path | Recruit building | Combat role | Footprint | Flying | Status | Risk |
|---|---|---|---|---|---|---|---|---|---|---|
| fighter_1 | Raider | Viking by Art.Hiraeth | ca4ba583-8d90-4069-b51f-50e694530b2f | Content/Viking/Mesh/SK_Viking.uasset | Raider Longhouse | fast melee fighter | 1 hex | no | EVIDENCE_BACKED | Custom skeleton/additional bones may require retarget work |
| ranged | Hunter (provisional) | Viking Customized modular family + Dark Fantasy Weapons bow | e814c3a3-11ba-4316-a07a-b0980fd016ef + 244607ad-e819-4320-bbc7-687b844b5bed | Content/Vikings/Mesh_UE5/Full/ | Hunter Range | ranged skirmisher | 1 hex | no | UE_VISUAL_CONFIRM | Viking pack has no bow; local Pro Longbow Pack supplies bow actions, but retarget and bow style/cohesion require UE |
| fighter_2 | Shieldmaiden | Norse Shield maiden | 5ee25a21-08c6-4511-8cd4-eb84a9e6344d | Content/Shieldmaiden/Mesh/SK_ShieldMaiden.uasset | Shield Hall | defensive melee / shield wall | 1 hex | no | EVIDENCE_BACKED | Different art source; verify scale/material cohesion beside Bugrimov and Art.Hiraeth characters |
| fighter_3 | Berserker | Fantasy Characters Pack / Viking Ulf | f5816915-86d9-4bef-b8f3-921408ae240b | payload/path pending | Berserker Mead Hall | aggressive melee / damage | 1 hex | no | UE_VISUAL_CONFIRM | Strong semantic fit; current payload package path not resolved |
| support_magic | Shaman (provisional) | Viking Customized modular family | e814c3a3-11ba-4316-a07a-b0980fd016ef | Content/Vikings/Mesh_UE5/Full/ | Shaman Lodge | support caster / debuff | 1 hex | no | UE_VISUAL_CONFIRM | No exact owned shaman character was verified; requires grounded staff/talisman/VFX treatment |
| fighter_4 | Huscarl | Viking Customized modular family | e814c3a3-11ba-4316-a07a-b0980fd016ef | Content/Vikings/Mesh_UE5/Full/ | Huscarl Hall | heavy melee / line holder | 1 hex | no | UE_VISUAL_CONFIRM | Choose restrained armor configuration; pack has many modular variants and weapons |
| beast | Wolf | ANIMAL VARIETY PACK / Wolf | 2dd7964c-a601-4264-a53d-465dcae1644c | Content/AnimalVarietyPack/Wolf/Meshes/SK_Wolf.uasset | Wolf Kennels | fast beast / flank | 1 hex | no | EVIDENCE_BACKED | 26-animation wolf set is cached; verify stack readability at hex scale |

**Alternates for weak slots**

- **ranged: Customized Viking + Dark Fantasy Weapons bow** — Strongest visual-cohesion route; local longbow actions exist, but exact body/retarget still needs UE `UE_VISUAL_CONFIRM`
- **ranged: Art.Hiraeth Viking with spear/javelin ranged treatment** — Avoids bow mismatch but changes hunter fantasy `UE_VISUAL_CONFIRM`
- **support_magic: Primitive elder / grandfather configuration** — Older silhouette may sell shaman better than armored Viking but cohesion is weaker `UE_VISUAL_CONFIRM`
- **support_magic: Fantasy Witch heavily re-equipped** — Mechanically ready caster proxy but female gothic silhouette may not read Norse `UE_VISUAL_CONFIRM`

### Orcs

| Slot | Unit | Owned asset | Listing ID | Known package path | Recruit building | Combat role | Footprint | Flying | Status | Risk |
|---|---|---|---|---|---|---|---|---|---|---|
| fighter_1 | Orc Grunt | 14 Orcs Pack / Orc 01 | 3c224616-e998-433c-910b-32901d426103 | payload/path pending | Grunt Barracks | basic melee fighter | 1 hex | no | UE_VISUAL_CONFIRM | Pack and shared-skeleton family are verified; exact visual role assignment among 14 variants needs review |
| ranged | Orc Hunter (provisional) | 14 Orcs Pack / Orc 02 + Dark Fantasy Weapons bow | 3c224616-e998-433c-910b-32901d426103 + 244607ad-e819-4320-bbc7-687b844b5bed | payload/path pending | Hunter Range | ranged skirmisher | 1 hex | no | UE_VISUAL_CONFIRM | No bow/crossbow is evidenced in the Orc pack; local Pro Longbow Pack supplies bow actions, but retarget and faction styling require UE |
| fighter_2 | Shield Orc | 14 Orcs Pack / Orc 04 | 3c224616-e998-433c-910b-32901d426103 | payload/path pending | Shield Pit | defensive melee / blocker | 1 hex | no | UE_VISUAL_CONFIRM | Variant selected only as casting candidate; confirm shield/armor silhouette before lock |
| fighter_3 | Berserker Orc | 14 Orcs Pack / Gnur | 3c224616-e998-433c-910b-32901d426103 | payload/path pending | Berserker Pit | aggressive melee / damage | 1 hex | no | UE_VISUAL_CONFIRM | Named variant exists; role mapping is visual hypothesis, not package metadata |
| fighter_4 | Orc Brute | Fantasy Characters Pack / Orc Hammer | f5816915-86d9-4bef-b8f3-921408ae240b | payload/path pending | Brute Hall | heavy melee / armor break | 2 hex | no | UE_VISUAL_CONFIRM | Confirm size supports 2-hex treatment and weapon is not visually excessive |
| support_magic | Orc Shaman (provisional) | 14 Orcs Pack / Magur | 3c224616-e998-433c-910b-32901d426103 | payload/path pending | Shaman Totem Court | support caster / morale | 1 hex | no | UE_VISUAL_CONFIRM | No shaman role is documented in the pack; Magur is a named casting hypothesis only |
| beast | War Elephant (provisional) | Fantasy Animal Pack / Fantasy Elephant | 4878a5f0-f82b-4530-b50d-a3f880ef4665 | payload/path pending | War Elephant Yard | large shock beast | 3 hex | no | UE_VISUAL_CONFIRM | Elephant is owned and rigged; confirm armor/rider treatment reads Orc rather than generic fantasy |

**Alternates for weak slots**

- **ranged: Any visually suitable 14 Orcs Pack variant + Dark Fantasy Weapons bow** — Keeps faction body language; exact variant must be chosen visually `UE_VISUAL_CONFIRM`
- **support_magic: Primitive elder re-skinned/re-equipped** — Grounded body fallback only; high faction-cohesion cost `UE_VISUAL_CONFIRM`
- **support_magic: Fantasy Witch with complete Orc-facing replacement kit** — Caster mechanics proxy, but too much visual replacement for preferred route `UE_VISUAL_CONFIRM`

### Dark

| Slot | Unit | Owned asset | Listing ID | Known package path | Recruit building | Combat role | Footprint | Flying | Status | Risk |
|---|---|---|---|---|---|---|---|---|---|---|
| fighter_1 | Black Guard | Fantasy Warriors Pack / Dark Knight | df8194f9-ec63-49f6-a885-6feb7a94fcb1 | payload/path pending | Black Guard Bastion | elite melee / guard | 1 hex | no | UE_VISUAL_CONFIRM | Strip or replace excessive spikes/emissive materials if present |
| ranged | Devil | Fantasy Enemies Pack / Devil | 79f79e3e-570b-4d96-bc3f-f62107178169 | payload/path pending | Dread Gallery | ranged spell attacker | 1 hex | no | UE_VISUAL_CONFIRM | Verify projectile/VFX remain readable without neon presentation |
| fighter_2 | Executioner | Fantasy Enemies Pack / Executioner | 79f79e3e-570b-4d96-bc3f-f62107178169 | payload/path pending | Execution Court | high-damage melee | 1 hex | no | UE_VISUAL_CONFIRM | Check weapon scale against grounded art rule |
| fighter_3 | Demon | Fantasy Enemies Pack / Demon | 79f79e3e-570b-4d96-bc3f-f62107178169 | payload/path pending | Demon Gate | durable melee / terror | 2 hex | no | UE_VISUAL_CONFIRM | Confirm silhouette is fantasy rather than stylized or overly emissive |
| fighter_4 | Troll | Fantasy Enemies Pack / Troll | 79f79e3e-570b-4d96-bc3f-f62107178169 | payload/path pending | Fallen Hall | large melee / regeneration | 2 hex | no | UE_VISUAL_CONFIRM | Confirm 2-hex footprint and avoid comic proportions |
| support_magic | Befouler | Fantasy Enemies Pack / Befouler | 79f79e3e-570b-4d96-bc3f-f62107178169 | payload/path pending | Befouler Sanctum | debuff / occult support | 1 hex | no | UE_VISUAL_CONFIRM | Confirm readable support silhouette and tame emissive effects |
| beast | Fantasy Dragon | Fantasy Animal Pack / Fantasy Dragon | 4878a5f0-f82b-4530-b50d-a3f880ef4665 | payload/path pending | Apex Dragon Roost | apex flying creature | 3 hex | yes | UE_VISUAL_CONFIRM | Owned pack component verified; current payload package path unresolved and scale must be qualified |

**Alternates for weak slots**

- **ranged: Befouler** — Could carry ranged/debuff role if Devil projectile presentation fails `UE_VISUAL_CONFIRM`
- **fighter_3: Demonic Warrior** — Fallback if Demon silhouette is too creature-like or emissive `UE_VISUAL_CONFIRM`
- **support_magic: Fantasy Witch** — Cleaner humanoid caster fallback if Befouler is visually noisy `UE_VISUAL_CONFIRM`

## Hero casting board

### Humans heroes

| Candidate | Owned source | Role | Grounded fit | Status | Casting note |
|---|---|---|---|---|---|
| Greystone | Paragon: Greystone | martial | high | UE_VISUAL_CONFIRM | Realistic knight/hero; strip supernatural FX and prefer least-luminous skin |
| Sparrow | Paragon: Sparrow | ranged | high | UE_VISUAL_CONFIRM | Realistic archer; suppress glowing arrows/ability FX |
| Fantasy Hero | Fantasy Warriors Pack | martial/hybrid | high | UE_VISUAL_CONFIRM | Owned pack candidate; inspect weapon scale and ornament |
| Fantasy Hero 02 | Fantasy Warriors Pack | martial/hybrid | high | UE_VISUAL_CONFIRM | Second owned hero silhouette; useful redundancy only if visually distinct |
| Knight_01 Commander | Knights Pack | martial | high | EVIDENCE_BACKED | Cached modular knight with sword/shield/halberd options |
| NPC King | NPC King And Queen | martial/support | high | UE_VISUAL_CONFIRM | Strong court commander candidate; binary manifest blocks exact package path audit |
| Fantasy Witch | Fantasy Warriors Pack | caster | medium-high | UE_VISUAL_CONFIRM | Grounded caster candidate if clothing/VFX pass avoids JRPG read |
| Terra | Paragon: Terra | martial | high | UE_VISUAL_CONFIRM | Shield/armor tank silhouette fits Castle with minimal conversion |
| Aurora | Paragon: Aurora | caster/hybrid | high | UE_VISUAL_CONFIRM | Armored frost hero; keep ice FX ability-bound rather than constantly emissive |
| Kwang | Paragon: Kwang | martial/hybrid | medium-high | UE_VISUAL_CONFIRM | Grounded warrior body; cultural silhouette is foreign to Castle but not technological |

### Dwarves heroes

| Candidate | Owned source | Role | Grounded fit | Status | Casting note |
|---|---|---|---|---|---|
| Dwarf King | Dwarfs Pack | martial/support | high | UE_VISUAL_CONFIRM | Most natural commander silhouette in owned dwarf family |
| Agvid | Dwarfs Pack | caster/support | high | UE_VISUAL_CONFIRM | Runesmith/forge identity differentiates non-royal hero |
| Bedvar | Dwarfs Pack | martial | high | UE_VISUAL_CONFIRM | Armored veteran option |
| Broddi | Dwarfs Pack | martial | high | UE_VISUAL_CONFIRM | Hammer-warrior option |
| Orme | Dwarfs Pack | martial | medium-high | UE_VISUAL_CONFIRM | Named owned dwarf; visible sword-bearing read conflicts with ranged slot but works as hero |
| Generic Dwarf | Fantasy Characters Pack | martial | medium-high | UE_VISUAL_CONFIRM | Useful alternate body if main pack heroes look too similar |

### Vikings heroes

| Candidate | Owned source | Role | Grounded fit | Status | Casting note |
|---|---|---|---|---|---|
| Khaimera | Paragon: Khaimera | martial/hybrid | medium-high | UE_VISUAL_CONFIRM | Tribal hunter/beast visual fits raider culture better than Castle; strip ability FX |
| Art.Hiraeth Viking | Viking | martial | high | EVIDENCE_BACKED | Cached realistic Nordic fighter with axe |
| Customized Viking Commander | Viking Customized | martial | high | UE_VISUAL_CONFIRM | 16 full-body combinations plus modular equipment; best controllable hero family |
| Shieldmaiden | Norse Shield maiden | martial | high | EVIDENCE_BACKED | Sword/shield heroine; art-source cohesion must be checked |
| Viking Ulf | Fantasy Characters Pack | martial | high | UE_VISUAL_CONFIRM | Strong berserker/hero silhouette candidate |
| Barbarian | Fantasy Characters Pack | martial | medium-high | UE_VISUAL_CONFIRM | Useful alternate if Ulf overlaps regular unit too closely |
| Barbarian Fantasy | Fantasy Characters Pack | hybrid | medium | UE_VISUAL_CONFIRM | Likely more fantastical; inspect for oversized weapon/ornament |
| Feng Mao | Paragon: Feng Mao | martial | medium | UE_VISUAL_CONFIRM | Realistic spear master, but imperial/East-Asian styling may fight Norse cohesion |

### Orcs heroes

| Candidate | Owned source | Role | Grounded fit | Status | Casting note |
|---|---|---|---|---|---|
| Azrak | 14 Orcs Pack | martial | high | UE_VISUAL_CONFIRM | Named owned-pack orc; shared pack skeleton/92-animation family |
| Dardan | 14 Orcs Pack | martial | high | UE_VISUAL_CONFIRM | Named owned-pack orc; candidate shield/brute commander after visual pass |
| Grux | Paragon: Grux | martial | high | UE_VISUAL_CONFIRM | Heavy creature-warrior commander with strong faction read |
| Gnur | 14 Orcs Pack | martial | high | UE_VISUAL_CONFIRM | Named owned-pack orc; candidate berserker commander |
| Iggy & Scorch | Paragon: Iggy & Scorch | hybrid | medium-high | UE_VISUAL_CONFIRM | Goblin beast-rider duo; strip scrapyard/industrial props |
| Magur | 14 Orcs Pack | martial/hybrid | medium-high | UE_VISUAL_CONFIRM | Named owned-pack orc; shaman role is a hypothesis, not source metadata |
| Ursag | 14 Orcs Pack | martial | high | UE_VISUAL_CONFIRM | Named owned-pack orc; alternate heavy commander |
| Orc Hammer | Fantasy Characters Pack | martial | high | UE_VISUAL_CONFIRM | Distinct heavy-hammer commander; useful if 14-Orcs heroes feel too samey |
| Narbash | Paragon: Narbash | support/martial | high | UE_VISUAL_CONFIRM | Excellent tribal drummer/morale commander; stronger support identity than inventing a shaman hero |

### Dark heroes

| Candidate | Owned source | Role | Grounded fit | Status | Casting note |
|---|---|---|---|---|---|
| Countess | Paragon: Countess | martial/hybrid | high | UE_VISUAL_CONFIRM | Gothic vampire/assassin presentation is one of the cleanest Dark hero fits; payload downloaded |
| Serath | Paragon: Serath | martial/hybrid | high | UE_VISUAL_CONFIRM | Gothic knight/angel-devil duality fits Dark or fallen-knight commander; remove bright holy FX if needed |
| Dark Knight | Fantasy Warriors Pack | martial | high | UE_VISUAL_CONFIRM | Direct grounded dark-fantasy commander candidate |
| Demonic Warrior | Fantasy Warriors Pack | martial/hybrid | medium-high | UE_VISUAL_CONFIRM | Strong if weapon/armor silhouette can be kept restrained |
| Befouler | Fantasy Enemies Pack | caster | medium-high | UE_VISUAL_CONFIRM | Purpose-built occult support silhouette; hero use avoids regular-unit duplication if roster changes |
| Executioner | Fantasy Enemies Pack | martial | medium-high | UE_VISUAL_CONFIRM | Grounded brutal commander if weapon scale passes |
| Morigesh | Paragon: Morigesh | caster | high | UE_VISUAL_CONFIRM | Swamp-witch/curse commander is a direct Dark fit |
| Sevarog | Paragon: Sevarog | martial/hybrid | high | UE_VISUAL_CONFIRM | Spectral reaper gives Dark a distinct non-human commander silhouette |
| Fantasy Witch | Fantasy Warriors Pack | caster | high | UE_VISUAL_CONFIRM | Cleaner humanoid occult commander than many monster silhouettes |
| Aurora | Paragon: Aurora | caster/hybrid | high | UE_VISUAL_CONFIRM | Armored frost queen/knight; ability-bound ice FX only |

## Future faction candidate — Nature

**Identity:** five animal-warrior families + Centaur Archer + one elephant/dragon-tier Apex Beast.
**Hero direction:** The Fey and Wukong are Nature heroes; human/animal hybrids expand the hero pool.
**Asymmetry:** no dedicated support/magic regular family. Preserve this unless balance testing proves it untenable; give one warrior secondary utility rather than adding an eighth family.

| Slot | Unit | Owned/source evidence | Recruitment site | Combat role | Footprint | Flying | Status | Risk |
|---|---|---|---|---|---|---|---|---|
| fighter_1 | Bear Warrior | Animals Warrior Pack | Bear Lodge | durable bruiser / line holder | 1 hex | no | UE_VISUAL_CONFIRM | Exact mesh/package path awaits payload; confirm weapon scale and armor cohesion. |
| fighter_2 | Bull Warrior | Animals Warrior Pack | Bull Ring | charge / linebreaker | 1 hex | no | UE_VISUAL_CONFIRM | Exact mesh/package path awaits payload; charge readability must not imply mounted cavalry. |
| fighter_3 | Crocodile Warrior | Animals Warrior Pack | Marsh Lodge | control bruiser / ambush fighter | 1 hex | no | UE_VISUAL_CONFIRM | Exact mesh/package path awaits payload; tail/cloth physics need visual qualification. |
| fighter_4 | Elephant Warrior (anthropomorphic) | Animals Warrior Pack | Ivory Guard Hall | heavy guardian / anchor | 1 hex | no | UE_VISUAL_CONFIRM | This is the humanoid Elephant Warrior, not the apex-beast slot; verify it remains normal-unit scale. |
| fighter_5 | Warrior2 animal warrior | Animals Warrior Pack | Prowler Platform | fast melee skirmisher / flanker | 1 hex | no | UE_VISUAL_CONFIRM | Older pack metadata calls this character Warrior2; species/name remains visual-confirm rather than guessed. |
| ranged | Centaur Archer | Quadruped Fantasy Creatures / Centaur | Centaur Range | mobile ranged pressure / repositioning | 1 hex | no | UE_VISUAL_CONFIRM | Owned model has bow/arrow and 85 animations; exact package path and Soul-scale group read still need qualification. |
| beast | Apex Beast (species TBD) | OWNED_LIBRARY_SELECTION_PENDING | Apex Beast Grove | elephant/dragon-tier capstone creature | 3 hex | TBD | ROSTER_PENDING | Do not lock Elephant or Dragon yet. Reusing Orc Fantasy Elephant or Dark/Dwarf dragons would reduce faction distinctiveness. |

### Nature hero candidates

| Candidate | Source | Role | Fit | Status | Note |
|---|---|---|---|---|---|
| The Fey | Paragon: The Fey | caster/support | HIGH | PAYLOAD_PENDING | Direct Nature-faction hero. Preserve spirit/nature presentation and keep magic restrained. |
| Wukong | Paragon: Wukong | martial/trickster | HIGH | PAYLOAD_PENDING | Nature hero by founder direction; staff fighter/trickster gives the faction a non-European mythic hybrid identity. |
| Leopard Hybrid_A1 | Animated3D | martial/hybrid | HIGH | LOCAL_NON_UE_EVIDENCE | Owned and locally downloaded FBX hybrid candidate; 8 combat/locomotion motions in source listing. |
| Leopard Hybrid_A2 | Animated3D | martial/hybrid | HIGH | PAYLOAD_PENDING | Owned current-library hybrid alternate; useful only if visually distinct from A1. |
| Werewolf Gerl | Bugrimov Maksim | martial/hybrid | MEDIUM_HIGH | UE_VISUAL_CONFIRM | Owned hybrid candidate; keep only if it reads as a nature champion rather than horror monster. |
| Khaimera | Paragon: Khaimera | martial/hybrid | MEDIUM_HIGH | PAYLOAD_PENDING | Optional tribal beast-champion alternate; do not cast him simultaneously as a Viking/Orc/Nature hero. |

**Locked design decisions:** Centaur is the ranged family; Apex Beast is elephant/dragon power tier but species remains open.
**Owned Centaur evidence:** Quadruped Fantasy Creatures already contains the PROTOFACTOR Centaur with 85 animations; the matching Centaur model supports both archery and close combat.
**Capstone caution:** reusing the same Fantasy Elephant or Dragon already earmarked for Orc/Dark/Dwarf would weaken faction silhouette separation. Prefer a distinct owned apex creature if the library supports one.

## Paragon-specific casting audit

| Paragon | Ownership/local state | Fab listing | Faction fit | Class | Grounded fit | Hide/replace | Skeleton/animation evidence | Redundancy | Best hero role | Hero eligible | Status |
|---|---|---|---|---|---|---|---|---|---|---|---|
| Countess | ACQUIRED_AND_LOCAL | 0bf014eb-f2ed-4029-adda-81a855eb5220 | Dark | martial/hybrid | HIGH | Use a restrained skin and suppress bright ability FX; keep blades only if proportions read as normal weapons. | Local payload present; binary manifest prevented exact package-path/skeleton inventory. Official pack includes animations and AnimBP. | Overlaps Serath/Morigesh occult space but has a distinct gothic-court silhouette. | vampire noble / assassin commander | yes | UE_VISUAL_CONFIRM |
| Greystone | SCREENSHOT_CONFIRMED_OWNED_NOT_LOCAL | 122fd7bf-6f12-4304-a930-cccbbacdaebc | Humans | martial | HIGH | Prefer least-luminous knight skin; remove resurrection/ability glow. | Official Fab pack includes character model plus animations/AnimBP or equivalent animation content. Exact package and skeleton paths remain PAYLOAD_PENDING until downloaded. | Overlaps Knight_01/Fantasy Hero; strongest as a named castle commander. | knight-lord / frontline commander | yes | PAYLOAD_PENDING |
| Sparrow | SCREENSHOT_CONFIRMED_OWNED_NOT_LOCAL | 7d76ddf0-d9ce-4d00-939e-d72793534d01 | Humans | ranged | HIGH | Suppress magical-arrow FX and keep the most conventional bow/armor skin. | Official Fab pack includes character model plus animations/AnimBP or equivalent animation content. Exact package and skeleton paths remain PAYLOAD_PENDING until downloaded. | Unique clean ranged-commander niche among the owned Paragons. | ranger / archer commander | yes | PAYLOAD_PENDING |
| Terra | SCREENSHOT_CONFIRMED_OWNED_NOT_LOCAL | 5ea6bcb6-e43e-4bbe-813f-c19d8c907565 | Humans, Vikings | martial | HIGH | Keep shield/armor; suppress ability glow and any non-medieval VFX. | Official Fab pack includes character model plus animations/AnimBP or equivalent animation content. Exact package and skeleton paths remain PAYLOAD_PENDING until downloaded. | Competes with Greystone/Shieldmaiden but offers a strong tank-command silhouette. | shield captain / defensive commander | yes | PAYLOAD_PENDING |
| Feng Mao | SCREENSHOT_CONFIRMED_OWNED_NOT_LOCAL | af344d79-eca6-4b6c-aae9-87c617a27ba1 | Humans | martial | MEDIUM_HIGH | Keep spear and practical armor; suppress supernatural FX. Cultural silhouette is the main mismatch. | Official Fab pack includes character model plus animations/AnimBP or equivalent animation content. Exact package and skeleton paths remain PAYLOAD_PENDING until downloaded. | Competes with Greystone/Kwang/Yin as a foreign human martial hero. | veteran spear-master / guardian | yes | PAYLOAD_PENDING |
| Kwang | SCREENSHOT_CONFIRMED_OWNED_NOT_LOCAL | f4c67e92-b976-4b5b-ab9f-4c25b010f6f3 | Humans | martial/hybrid | MEDIUM_HIGH | Keep the grounded warrior body; tame electrical FX and avoid any overly ornate sword treatment. | Official Fab pack includes character model plus animations/AnimBP or equivalent animation content. Exact package and skeleton paths remain PAYLOAD_PENDING until downloaded. | Overlaps Feng Mao/Yin in an East-Asian martial niche. | sword-sage / storm champion | yes | PAYLOAD_PENDING |
| Yin | SCREENSHOT_CONFIRMED_OWNED_NOT_LOCAL | dc21702b-7f1e-4aa5-a747-78d519f5fb51 | Humans | martial | MEDIUM_HIGH | Whip can stay if scale reads practical; suppress energy trails except for discrete abilities. | Official Fab pack includes character model plus animations/AnimBP or equivalent animation content. Exact package and skeleton paths remain PAYLOAD_PENDING until downloaded. | Distinct weapon silhouette, but culturally overlaps Kwang/Feng Mao. | whip fighter / mobile duelist | yes | PAYLOAD_PENDING |
| Aurora | SCREENSHOT_CONFIRMED_OWNED_NOT_LOCAL | 918456eb-4c36-4346-9be2-8986e25c9a0b | Humans, Dark | caster/hybrid | HIGH_WITH_FX_TRIM | Keep medieval armor and ice-blade silhouette; reduce constant ice glow to ability moments. | Official Fab pack includes character model plus animations/AnimBP or equivalent animation content. Exact package and skeleton paths remain PAYLOAD_PENDING until downloaded. | Overlaps Fantasy Witch/Morigesh as caster but offers a clean armored frost identity. | frost knight / control caster | yes | PAYLOAD_PENDING |
| Serath | SCREENSHOT_CONFIRMED_OWNED_NOT_LOCAL | 522b6160-15ab-492b-a2b0-c09f9bb5f6e6 | Dark, Humans | martial/hybrid | HIGH_WITH_FX_TRIM | Keep grounded knight body; suppress wings/bright transformation FX except for rare abilities. | Official Fab pack includes character model plus animations/AnimBP or equivalent animation content. Exact package and skeleton paths remain PAYLOAD_PENDING until downloaded. | Overlaps Greystone as ritual knight and Countess as Dark female hero. | fallen knight / cursed champion | yes | PAYLOAD_PENDING |
| Morigesh | SCREENSHOT_CONFIRMED_OWNED_NOT_LOCAL | 29e67175-fa08-448f-822b-37f411530749 | Dark | caster | HIGH | Keep swamp-witch body language; trim permanent glow and any oversized magical props. | Official Fab pack includes character model plus animations/AnimBP or equivalent animation content. Exact package and skeleton paths remain PAYLOAD_PENDING until downloaded. | Overlaps Fantasy Witch/Befouler but is a much stronger named Dark hero silhouette. | swamp witch / curse commander | yes | PAYLOAD_PENDING |
| Sevarog | SCREENSHOT_CONFIRMED_OWNED_NOT_LOCAL | a4882b5e-cfad-4830-a3dd-46a6c31a79b2 | Dark | martial/hybrid | HIGH | Keep reaper/spectre silhouette; remove any sci-fi surface treatment and restrain emissives. | Official Fab pack includes character model plus animations/AnimBP or equivalent animation content. Exact package and skeleton paths remain PAYLOAD_PENDING until downloaded. | Distinct from humanoid Dark heroes; strongest spectral/undead commander option. | reaper lord / terror commander | yes | PAYLOAD_PENDING |
| The Fey | SCREENSHOT_CONFIRMED_OWNED_NOT_LOCAL | 9afbcde6-4a14-4018-95c3-2f3a2e1da858 | Nature | caster | HIGH | No major technology removal needed; keep nature/spirit styling and restrained magic. | Official Fab pack includes character model plus animations/AnimBP or equivalent animation content. Exact package and skeleton paths remain PAYLOAD_PENDING until downloaded. | Direct Nature-faction commander by founder direction. | nature spirit / druidic commander | yes | PAYLOAD_PENDING |
| Narbash | SCREENSHOT_CONFIRMED_OWNED_NOT_LOCAL | d8904a0e-9169-4763-b82b-5fcf864235a4 | Orcs | support/martial | HIGH | Keep tribal armor/drum; suppress MOBA-scale ability FX. | Official Fab pack includes character model plus animations/AnimBP or equivalent animation content. Exact package and skeleton paths remain PAYLOAD_PENDING until downloaded. | Very strong Orc support identity and materially better than inventing an Orc shaman hero. | war drummer / morale-support commander | yes | PAYLOAD_PENDING |
| Grux | SCREENSHOT_CONFIRMED_OWNED_NOT_LOCAL | 8c4bac2c-f7f7-4632-a644-47f4e104f5d8 | Orcs | martial | HIGH | Keep creature-warrior silhouette; replace or hide any implausibly oversized weapon pieces if needed. | Official Fab pack includes character model plus animations/AnimBP or equivalent animation content. Exact package and skeleton paths remain PAYLOAD_PENDING until downloaded. | Distinct heavy beastman commander; overlaps brute role but not shaman/support. | beast warlord / shock commander | yes | PAYLOAD_PENDING |
| Khaimera | SCREENSHOT_CONFIRMED_OWNED_NOT_LOCAL | e7c665c1-8c13-42f0-9152-0753008853d7 | Vikings, Orcs | martial/hybrid | MEDIUM_HIGH | Keep tribal armor/skull motif; remove glowing ability FX. | Official Fab pack includes character model plus animations/AnimBP or equivalent animation content. Exact package and skeleton paths remain PAYLOAD_PENDING until downloaded. | Strong tribal identity overlaps Viking Ulf/Orc berserker space; use in one faction only. | cursed hunter / berserker commander | yes | PAYLOAD_PENDING |
| Iggy & Scorch | SCREENSHOT_CONFIRMED_OWNED_NOT_LOCAL | 67570f6d-3290-4482-819d-b18853bd8307 | Orcs | hybrid | MEDIUM_HIGH | Keep goblin+rider concept; remove scrapyard/industrial props and any comedic-modern equipment. | Official Fab pack includes character model plus animations/AnimBP or equivalent animation content. Exact package and skeleton paths remain PAYLOAD_PENDING until downloaded. | Unique mounted/duo silhouette; less grounded than Narbash/Grux but highly faction-readable. | goblin beast-rider / disruptive commander | yes | PAYLOAD_PENDING |
| Rampage | SCREENSHOT_CONFIRMED_OWNED_NOT_LOCAL | 0807cf74-08fd-4a33-8c8d-f33c9439fb1f | Orcs, Dark | martial/tank | MEDIUM | Strip sci-fi laboratory cues, tech accessories and bright FX; retain the monster body. | Official Fab pack includes character model plus animations/AnimBP or equivalent animation content. Exact package and skeleton paths remain PAYLOAD_PENDING until downloaded. | Overlaps Grux as a huge creature commander with substantially more conversion burden. | escaped brute / monster commander | yes | UE_VISUAL_CONFIRM |
| Wukong | SCREENSHOT_CONFIRMED_OWNED_NOT_LOCAL | 27054d0c-c26e-4fe3-b6f9-fa778dfcb8b6 | Nature | martial | HIGH | Keep staff/warrior silhouette; suppress magical clone/ability FX. Nature-faction fit is intentional; retain staff-warrior/trickster identity. | Official Fab pack includes character model plus animations/AnimBP or equivalent animation content. Exact package and skeleton paths remain PAYLOAD_PENDING until downloaded. | Direct Nature hero by founder direction; complements animal/hybrid regular army. | nature trickster / mobile martial commander | yes | PAYLOAD_PENDING |
| Phase | SCREENSHOT_CONFIRMED_OWNED_NOT_LOCAL | b2c95d5c-a805-460b-a01b-db6da3a778f0 | Dark | caster/support | LOW_TO_MEDIUM | Remove cyberpunk clothing/details, modern accessories and sci-fi reads; replace with period-fantasy garments. | Official Fab pack includes character model plus animations/AnimBP or equivalent animation content. Exact package and skeleton paths remain PAYLOAD_PENDING until downloaded. | Caster/support niche overlaps Morigesh/Fantasy Witch with much higher conversion cost. | psychic/occult support after full fantasy redress | yes | UE_VISUAL_CONFIRM |
| Minions | SCREENSHOT_CONFIRMED_OWNED_NOT_LOCAL | 039ea035-9360-4e76-ad06-5d3a92da6f65 |  | troop_pack | LOW_TO_MEDIUM | Not a hero candidate; do not use as a Paragon commander. Technology/style varies by minion type. | Official Fab pack includes character model plus animations/AnimBP or equivalent animation content. Exact package and skeleton paths remain PAYLOAD_PENDING until downloaded. | Separate troop/donor pack rather than a hero archetype. | not hero-eligible | no | UE_VISUAL_CONFIRM |

### Paragon ownership resolved from screenshots

The supplied Fab My Library screenshots resolve 21 Paragon search results: 19 named hero-character packs, the Minions troop pack, and the Agora/Monolith environment pack.
Download is not required for casting. Countess is the only payload currently machine-visible locally; every other named hero remains PAYLOAD_PENDING for exact package/skeleton paths.
Minions and Agora/Monolith are owned but excluded from the hero pool because they are not named hero characters.

## Animation/equipment evidence that changes roster feasibility

- Canonical local animation repository reports 45 Bow/archery candidates; Pro Longbow Pack includes equip/draw/aim/recoil/walk/block/death/dodge actions.
- Crossbow-specific animation coverage was not found. Dwarf Orme/crossbow remains the cleanest unresolved ranged-slot problem.
- Hivemind Dark Fantasy Weapons is owned and publicly resolves to Fab listing `244607ad-e819-4320-bbc7-687b844b5bed`; the pack is tagged for Bow/Crossbow and includes ranged weapon assets.
- `92 Animations For Warrior` is locally cached and provides a broad Epic-skeleton combat donor for compatible humanoid packs.

## Strongest unresolved roster gaps

1. **Dwarf ranged identity:** Orme is owned and named, but the source evidence does not prove a crossbow. No crossbow animation set was found locally. Keep the Crossbow Workshop concept provisional until UE visual qualification or a better owned crossbow character is surfaced.
2. **Exact Witch Adventurer identity:** no exact owned/local product named `Witch Adventurer` was found. `Fantasy Witch` from the owned Fantasy Warriors pack is the strongest evidence-backed substitute and should not be silently renamed into a proven asset.
3. **Viking Shaman identity:** no exact owned Shaman character was verified. A grounded Customized Viking or Primitive elder redress is viable, but remains casting rather than asset proof.
4. **Orc Shaman identity:** 14 Orcs provides a deep shared-skeleton family, but no source metadata labels a shaman. Magur is only a visual-casting hypothesis.

## UE visual qualification queue

- Side-by-side Human Knight_02/03/05 variants: ensure militia, man-at-arms and elite guard read as three tiers rather than recolors.
- Verify Griffon at 2-hex and Mountain Dragon/Fantasy Dragon/War Elephant at 3-hex scale; adjust footprint only after battle-camera read.
- Retarget Pro Longbow animations onto Human/Viking/Orc candidate bodies; test shoulder/hand/bow alignment and silhouette.
- Test Dwarf Orme with an owned crossbow/bow solution; if it reads forced, change the ranged family rather than forcing the building fiction.
- Check Shieldmaiden material/scale cohesion beside Bugrimov/Art.Hiraeth Vikings.
- Cast exact Viking and Orc shaman visuals from owned modular pieces; reject anything too neon, sci-fi, anime/JRPG or over-spiked.
- Run Dark pack side-by-side: Dark Knight, Devil, Executioner, Demon, Troll, Befouler, Fantasy Dragon. Suppress permanent emissive/neon styling.
- For Paragons, inspect removable mesh components/skins and record exact skeleton paths as payloads arrive. Countess first because it is already local.

## Evidence files and source notes

- `Evidence/soul_roster_asset_inventory_20260920.json` — ownership rows, live Fab listing DB matches, downloaded manifest summaries and exact cached package paths where readable.
- `Evidence/paragon_library_screenshot_inventory_20260920.json` — the 21-product My Library screenshot inventory resolving the previously unknown Paragon identities.
- `Data/soul_faction_roster_candidates_20260920.json` — machine-readable 35-unit roster, alternates and hero board.
- `Data/soul_paragon_casting_candidates_20260920.json` — Paragon ownership/local-state and casting classifications.
- `D:/Animations/_Catalog/CAPABILITY_GAPS.md` + `SEARCH-ANIMATIONS.cmd bow` — local animation capability evidence.
- Fab content confirmation used for pack membership/current listing UUIDs: Quadruped Fantasy Creatures, Fantasy Animal, Fantasy Characters, Fantasy Enemies, Fantasy Warriors, Dwarfs Pack, 14 Orcs Pack, Dark Fantasy Weapons and verified Paragon listings.

## Non-lock rule

Nothing marked `UE_VISUAL_CONFIRM` or `PAYLOAD_PENDING` is a final art lock. The roster is the strongest evidence-backed casting board available without consuming the UE lane.
