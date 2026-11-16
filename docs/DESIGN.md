# Killing Part: design brief

The source of truth for the game. Simon makes the design calls: propose changes, don't make them.
Feel reference: Prototype/KillingPart_Toy.html (open it in a browser and play it). Its numbers are the starting tuning; the table below converts them to Unreal units.

## Pitch

A seamstress's plushies come alive at night, and they want hugs. Deadly ones. Hold the mouse and time almost stops while you draw her cut in tailor's chalk. Let go: she flashes along the line with giant pink shears, a beat, snip, and everything she passed through falls in half. Cut five at once and the game calls it out: Killing part.

Tagline (proposal, Simon decides): She made them. She cuts them. ("She buries them" went with the graveyard in T5b.)

Her name: Simon's call (not decided yet).

## Pillars

1. One verb: draw the cut. Every feature should make drawing a line more interesting.
2. Cute outside, gore inside. Pastel plush world, red insides. The Theatrical cut swaps blood for white stuffing.
3. Every night counts. Each night keeps its best score, the week score is the sum of them, and the week calendar after every night shows where you stand: finish the week, then beat your week score.
4. Readable in ten seconds. A judge gets it without a tutorial.

## Hard rules

- Original characters and art only. Nothing from Hunter x Hunter, anywhere (game, video, itch page).
- Stay away from the FNAF / Poppy Playtime toy-horror look: no animatronics, no "five nights", no 6 AM clock, no grinning mascot monsters. Our plushies are soft, button-eyed and cute until they're cut.
- Jam rule 7(h): all gameplay code is written during the jam.
- One single-screen arena with a fixed camera. No hand-built levels, no custom animation.

## The core loop (about 10 seconds)

1. Plushies walk straight at her from the edges of the mat, pushing apart so they don't stack.
2. Hold the left mouse button: time slows almost to a stop. A dashed chalk line starts at her feet and follows the cursor across the mat, up to a maximum length (the chalk). While drawing:
   - plushies the line will cut get a dashed ring,
   - an "xN" counter sits at the end of the line,
   - pincushions the line crosses get a red warning ring.
3. Release: she dashes along the line at real speed (not slowed). Every plushie whose centre comes within its radius + the blade width of her path is marked: it freezes with a thin white cut line across it.
4. The beat: the shears close (snip sound) while time is still slow.
5. Snip: every marked plushie splits along the cut with red insides, a blood burst and splats on the mat. Time ramps back to normal. Combo text and score pop up.

- Combo names: 1 Snip, 2 Double, 3 Tailored, 4 Bespoke, 5+ Killing part (big gold text, flash, bigger shake).
- What counts in the combo (T7c): every plushie the line cuts, the boss (one hit), Seam ripper's targets and a golden pincushion. Chicks don't count: they give no score, and counting them let a line through a crowd of boss-fight chicks score a huge combo (10 chicks + 1 plushie was 100 x 11 x 11). A line that cuts only chicks shows no combo text.
- She doesn't walk. Dashing is her only movement. It worked in the toy; keep it unless playtests say otherwise.
- A plushie that touches her (outside the dash and the beat) costs a heart and dies without score. She blinks, invulnerable, for a moment. 3 hearts.
- Pincushions are stationary hazards. Cutting through one costs a heart.

### Starting numbers

| What | In the toy | In Unreal (about x3.5) |
| --- | --- | --- |
| Arena | 680 x 408 px | mat about 2400 x 1440 uu |
| Her radius | 11 px | about 40 uu (template capsule is 42) |
| Plushie radius | 12-16 px | 42-56 uu |
| Plushie speed | 18 + 3.2 per wave + 0-9 px/s | 63 + 11 per wave + 0-32 uu/s |
| Wave size | 3 + 2 x wave, one every 0.45 s (game time), from just outside the edges | same |
| Chalk (max line per cut) | 1.05 x arena height | about 1500 uu |
| Point spacing while drawing | 5 px | 15-20 uu |
| Shortest line that counts | 18 px (shorter cancels) | 60 uu |
| Time scale while drawing | 0.07 | 0.07 |
| Time scale during dash and beat | 0.12 | 0.12 |
| Return to normal speed | approach 1.0 at rate 16/s | same |
| Dash speed (real time) | 1700 px/s | about 6000 uu/s (a full line takes about 0.25 s) |
| Blade width (hit pad) | radius + 6 px | radius + 20 uu |
| The beat (dash end to snip) | 0.30 s real time | same |
| Cooldown after a snip | 0.12 s | same |
| Hearts / invulnerability after a hit | 3 / 1.1 s | same |
| Score per snip | 100 x n x n | same |
| Pincushion | radius 13 px, lives 12 s, from wave 3: min(3, wave - 2) per wave | radius 46 uu, same count; since T4b it lasts until the wave ends (the 12 s lifetime looked like a bug) |
| Halves | fly apart along the cut normal at 70-160 px/s, spin, fade after 6 s | 250-560 uu/s |

## One night = one run

- The game is one week: seven nights, Monday to Sunday. Each night is a list of waves (data in Content/Data/KillingPart.json). Clearing the last wave is dawn.
- Each night has one rule that changes how you draw:
  - Mon: no rule (learn the cut)
  - Tue: Pins (pincushions appear)
  - Wed: Hop (bunnies move in hops, so timing matters). Patchwork plushies (ignore slow motion) are regulars from here on; one or two appear as a teaser in Tuesday's last wave, so judges meet them early.
  - Thu: Lights out. Really dark: outside her lamp circle you only see faint glinting button eyes, and pincushions are invisible. The chalk line lights up the mat while you draw (fading after the cut), so drawing is how you see.
  - Fri: Big bears (cut one and both halves get up as two small bears)
  - Sat: Short chalk (-30% chalk, more plushies). Mother hen (T7c): she walks in until she's about 650 uu from her, then keeps that distance, circling and backing away when you come closer, laying a chick every 1.5 s (she still arrives with 3 rushing chicks). Killing her first takes a committed dash across the mat.
- Variants (T7d, Simon's idea): rare plushies that combine exactly two types, only where both stay readable: Patchwork bunny (hops and ignores slow motion), Bunny big bear (hops; its cubs hop), Patchwork big bear (ignores slow motion; patchwork cubs), Golden big bear (gold, x5, big-bear speed; two golden cubs: the jackpot). No hen variants, no triples. From Thursday (Extended cut: round 30) about 4% of spawns, only combining types already seen that week. The first time each appears: a small toast at the top ("New: Patchwork bunny"), no introduction.
- The impostor (T7d, Simon's idea): a plushie disguised as a pincushion (the same red cushion, band and pins) with four little felt feet. From Friday about 40% of non-boss, non-dark waves get one (Extended cut: from round 35, 30%), at most one at a time, at least 450 uu from every edge. It sits still wiggling for 1.5 s (the tell), then walks at 60 uu/s (slowed by slow motion like any plushie) straight for the edge that was nearest when it appeared, stepping round her; it never attacks. Cutting it is safe: x3 (the golden impostor, 1 in 10, x5), counts in the combo, a normal white preview ring. If it gets off the mat it escapes: "Escaped!" at the edge and a heart. It holds its wave until it's cut or gone. Introduced once: "Impostor: It's not a pincushion. Cut it before it escapes".
- Why you lost (T7d): the Hugged to bits screen and the endless run-over say what took the last heart: "A plushie got its hug." / "You cut through a pincushion." / "An impostor escaped. Cut walking pincushions before they reach the edge." / "The Firstborn got its hug."
- The blinking pincushion (T7c; T7d: in a dark wave only its purple pin heads glint faintly, its warning ring glows purple and a small purple light pulses on the spot with each pulse): from Thursday (Extended cut: from round 20), about 1 in 4 pincushions is purple and blinks: every 4 s it vanishes and pops up elsewhere on the mat. 1 s before, a faint ring marks the new spot and pulses twice, each pulse with a quiet, low tick. Otherwise a pincushion like any (Magnet works). Introduced once like the other hazards.
  - Sun: The Firstborn (renamed from "The first one" in T6d, Simon's pick; boss: her very first plushie, huge, and since T6c the hardest fight of the week: 50 hits (every line through it is one), a chunk off every 10 that gets up as a small teddy, every hit brings another plushie or pincushion from the edges, 7 chicks every 3 s for the whole fight, enraged at 20 hits left, the last hit splits it in half)
- Darkness after Thursday (T6c, all week modes): exactly 3 waves across Friday, Saturday and Sunday are dark (never the boss wave, at most 2 in one night), rolled when a week starts and saved with it, so a retry gets the same ones. A dark wave says "Lights out!" and uses Thursday's darkness exactly.
- Between waves: pick 1 of 3 upgrade cards (in Normal and Hardcore; the other modes change this, see Modes). Every card has a boon and a loose thread (a catch), so every pick is a real choice. Cards reset every night.
- Lose all hearts: "Hugged to bits", the night ends. In Normal, "Try again" replays it (a pure retry); in the other week modes it's back to Monday.
- The 3-2-1 countdown over the mat comes only after Continue (loading a save); T6c had it before every night, T6d removed that.
- Score (T6c): a week's score is the sum of its nights' scores (in Normal the best of each night's attempts, so grinding a night only helps if you beat yourself). A mode's high score (T6d: called "High score", not "Record") is its best finished week. Night select was removed in T6c.
- Best time (T6d): week modes track active play time, in real seconds while a night is being played (drawing included, counted in real time, not slowed time; not the countdown, card picks, pause, introductions or results; lost attempts and restarted nights count, it's time you played). A mode's "Best time" is its fastest finished week. The results screen shows the week time so far, the ending shows the time and the best time.

### Modes (T6c, Simon's decisions Sun 12:53)

No score multipliers; each mode has its own save (the week in progress) and its own high score. All names and texts are in the JSON ("modes"). The title menu is Play, the cut, Quit; Play opens the modes screen.

Modes screen (T6d, Simon found seven cards at once too cluttered): two tall cards side by side, the week modes on the left, the endless modes on the right. Each card steps through its modes with < and > arrows (locked modes can be browsed too, greyed, with "Finish ... to unlock" under the card) and shows a dot per mode, the high score and best time (week) or high score and best round (endless), and the mode's complete rules as bullets. Under each card, for the mode it shows: "Continue: Tuesday, 10,050" (endless: "Continue: Round 23, 41,200") when there's a save, and "New game". A back arrow in the top-left corner; the card height shrinks on short screens so the jam footer never overlaps a button.

Rules (T6d: each mode's complete rules as bullets, no more "Like Hardcore, but..."; every last bullet reads "Lose all hearts: <what happens>"):

| Mode | Rules (the card's bullets) | Unlock |
| --- | --- | --- |
| Normal | 7 nights, Monday to Sunday / After each wave, pick 1 of 3 cards / Cards and hearts reset every night / Lose all hearts: replay that night | from the start |
| Hardcore | 7 nights / After each wave, pick 1 of 3 cards / Cards reset every night, hearts don't / Lose all hearts: start over from Monday | Finish Normal |
| Brutal | 7 nights / After each wave you get 1 random card, no choice / Cards reset every night, hearts don't / Lose all hearts: start over from Monday | Finish Hardcore |
| Extreme | 7 nights / 1 random card at the start of each night, none after waves / Hearts don't reset between nights / Lose all hearts: start over from Monday | Finish Brutal |
| Insane | 7 nights / No cards / Hearts don't reset between nights / Lose all hearts: start over from Monday | Finish Extreme |
| Final cut | 7 nights / No cards / Only 1 heart / Lose your heart: start over from Monday | Finish Insane |
| Demon (T7d; was Impossible in T7c) | 7 nights / No cards, only 1 heart / After each wave, pick 1 of 3 curses / A curse can come again, stronger (II, III) / Curses reset every night / Lose your heart: start over from Monday | Finish Final cut |
| Impossible (T7d; was Godlike in T7c) | 7 nights / No cards, only 1 heart / After each wave, 1 random curse, no choice / A curse can come again, stronger (II, III) / Curses reset every night / Lose your heart: start over from Monday | Finish Demon |
| Extended cut | Endless rounds, 1 more plushie every round / New plushies along the way, bosses from round 30 / Pick 1 of 3 cards every 10 rounds, they last the whole run / Hearts don't reset / Lose all hearts: the run ends | Finish Normal |
| Extended nightmare | Everything from Extended cut / Every round is dark | Finish round 50 in Extended cut (round 51 reached) |

Saving (T6d): a week is saved at the start of every night; an endless run at the start of every round (round, hearts, cards and their levels, score, time), and Continue starts that round again. The pause menu says so: Resume, the cut, Restart night (endless: Restart round, from the round's save), "Save & quit to home", "Save & quit game".

Extended cut (T6c): rounds instead of nights, no night rules, hearts carry over (Thimble can add, max 5), losing ends the run; "Round 12" at the top. Truly endless: every round comes from formulas in the JSON ("endless"): one more plushie per round (spawns wait at the alive cap of 45), speed rising to a cap, spawn interval shrinking to a minimum, a new type every 5th round (pincushions 5, bunny 10, patchwork 15, big bear 20, mother hen 25; after that their shares keep growing), pincushions growing slowly to a cap, and from round 30 every 10th round a boss round (1 boss, a second from round 50, and so on; 15 hits each plus 5 per boss round; Sunday's spawning rules). A card pick (1 of 3, kept all run) every 10th round until nothing is left to offer. No darkness (that's what Extended nightmare is for). The run-over screen: round reached, score, high score and best round with "New!", Play again or Back to home. Records: high score and best round, on its card. Runs are resumable since T6d (saved at every round).

### Upgrade cards (build 6-8 first)

Card words (T4c): numbers, never vague words; the "line" is what you draw, "chalk" is how long it can be, "cut" is what happens to plushies; never "snip" or "she" on a card; short like a stat line, with the heart icon where a heart is meant.

Card levels (T4c): a card you hold can be offered again as level II, then III (max). Each level adds the boon and the loose thread again.

Since T4d every card number lives in KillingPart.json (each card's "values"; its texts show them through {key}), so cards are balanced by editing the JSON and restarting Play.

| Card | Boon (as shown) | Loose thread (as shown) | Levels |
| --- | --- | --- | --- |
| Long chalk | +25% chalk | Plushies +10% speed | +25% chalk per level |
| Pinking shears | Line cuts 2x wider | -10% chalk | 2x, 3x, 4x the reach; a pale band shows the reach while drawing |
| Thimble | +1 (heart icon) (max 5); not offered at the cap | +30% plushies per wave | +30% per level |
| Extra inch (replaced Quick hands) | Line end cuts a cone ahead | Plushies +10% speed | cone pointing along the line's last direction: 30 degrees x 70 uu, 50 x 100, 70 x 130 |
| Tape measure | +50% score | Plushies +5% speed | +50% per level |
| Seam ripper | Cut plushies throw pins at neighbours | +1 pincushion per wave | reach 150, 225, 300 uu; the pins fly with a trail and stay stuck in what they hit |
| Magnet (new in T4c) | First pincushion you cut each wave is harmless | -15% chalk | 1, 2, 3 pincushions per wave; gold preview ring instead of the red warning |
| Lucky button | +50% buttons | Next card pick: 2 cards | (built in T5, removed in T5b with the buttons) |
| Bias cut | a line that crosses itself cuts everything inside the loop | -15% chalk | (not built) |

Curses (T7c, Simon's idea; per wave since T7d): a card's loose thread on its own, without its boon. Restless (Plushies +10% speed), Short chalk (-15% chalk), Full house (+30% plushies per wave), Pin drop (+1 pincushion per wave). They show as cards with only the red part. T7d: they come after each wave like cards do in Normal (Demon: pick 1 of 3; Impossible: 1 at random), level up to II and III like cards (each level adds the thread again) and reset every night. In the JSON they're cards with "curse": true. Saves from T7c are renamed once (Impossible -> Demon, Godlike -> Impossible).

## Between nights (one screen)

Decided Sat 20:32 (T5b), after Simon played T5: with this little content a workbench and a graveyard have no purpose (Monday paid 40 buttons, the first card cost 20, every purchase was a formality) and the screens were detours from cutting. So there are no buttons, no shop, no graveyard, no reroll and no looks; every card is in the pool from the start.

- After every night, won or lost, one screen over the mat:
  - the week calendar on top: Monday to Sunday, each night's best score or a dash, won nights in mint, the next night ringed in pink;
  - the heading: "Dawn / Monday survived", or "Hugged to bits / Tuesday" after a loss; after Sunday "Week complete!" with the week score;
  - the night's results: score (gold with "New best!" when it beats that night's best), that night's best, best combo, week score;
  - one button: "Next night: Tuesday", "Try again: Tuesday" (Normal), "Back to Monday" (the other week modes) or, after Sunday, "Back to home". It takes clicks after 0.6 s, so the click that ended the night can't skip it.
  - T6c: the numbers are score, the week's score so far, the mode's record and the best combo. T6d: score, week score, week time, "<mode> high score", best combo.
- The save (USaveGame): each night's best score and best combo, won nights, the current night, the furthest night, week complete and the chosen cut; since T6 also whether the cut was chosen, the tutorial's two steps, the plushies already met and the Hardcore best. Saved at the end of every night. Launch opens the title screen (T6).
- After Sunday (T6): the ending replaces the results screen (see T6).
- The screen's words are in KillingPart.json ("texts").

## Theme: Director's Cut

- You direct every cut: you draw it, she performs it.
- At first launch the game asks which version to watch: the Theatrical cut (white stuffing, no blood) or the Director's cut (blood). Switchable from pause. It doubles as the content setting.
- Camera cuts (T7d): a line through 5 or more plushies (and the Firstborn's final split) cuts to a low, close camera behind and beside her for the dash and the beat, with letterbox bars, and cuts back to the game camera at the snip (after its hitstop), before time returns to normal. She can't be hurt in the dash and the beat, so it never costs control or a heart. "Camera cuts: On / Off" on the title and in pause, saved, default On.
- Stretch (not built): The end of each night shows "the director's cut", your three best cuts as polaroids (SceneCapture at the snip).

## Look

- The arena is a giant cutting mat on her worktable at night: mint green with a white grid and ruler numbers (a recoloured grid material from /Game/LevelPrototyping), warm lamp light, oversized spools, pins and buttons around the edges as props.
- One screen with a fixed camera, looking steeply down (about 60-65 degrees) so the plushies' faces read.
- Her (T7d, the seamstress): the template's Quinn mannequin at 1.4x (the hitbox is unchanged): a two-tone plum dress with long sleeves, a white apron (bib, straps over the shoulders, belt, skirt, a pink pocket), pink hair with two pigtails, ties and a big bow, all basic shapes on her bones. Giant shears (tapered blades, round ring handles, a screw at the pivot) only during the dash and the beat: they open as she dashes, stay open over the beat and snap shut exactly as the snip sounds; no attack clip (it read as a punch). A pink afterimage trail on the dash.
- Wardrobe (T7d): a title button. She stands in the lamp light, close up, shears open, her dash trail circling her feet; rows with arrows and 6 swatches each for the dress, apron, hair and bow, shears and dash trail (pastel and bold; the first is the seamstress). Saved, applies everywhere, no unlocking. Swatches in the JSON ("wardrobe").
- Plushies from basic shapes: round body, ears, button eyes, a stitched seam down the middle. Pastel colours: cream, lilac, peach, sky, rose, mint.
- Blood: red bursts, and decal splats that stay on the mat for the whole night. Theatrical cut: white fluff bursts, no splats.
- UI in Slate (as in Card Island): hearts, chalk bar, score, wave, combo text, card picks, the results screen after each night. A cute rounded font if time allows (an OFL font such as Fredoka or Baloo 2).

## Audio (Saturday)

- SFX: chalk scratch while drawing, whoosh, the snip (the most important sound in the game), squish, plush squeak, hurt, card pick, UI clicks. CC0 sources (freesound.org) or an AI SFX tool, credited.
- Music: one cozy lo-fi or city-pop track for the nights (low-pass it while drawing), and a music-box tune for the results screen and menus. Royalty-free or AI-made, credited.

## Unreal notes

- Slow motion is UGameplayStatics::SetGlobalTimeDilation. The dash, the beat and UI timers must run on undilated time (UWorld::DeltaRealTimeSeconds or FApp::GetDeltaTime()), not the dilated DeltaSeconds.
- Cursor to mat: DeprojectMousePositionToWorld, then intersect the ray with the mat's plane (no collision needed).
- Chalk line: an instanced static mesh of short, flat, white dashes just above the mat. Debug lines don't render in shipping builds.
- Hit test: on every frame of the dash, take the point-to-segment distance in XY from each plushie to the segment she just moved along. Store the cut direction on the plushie.
- Plushies: one manager ticks them all (steering plus separation). No CharacterMovement, no AI controllers, no nav mesh. Aim for 40 alive at once without hitching.
- Slicing (Saturday): UProceduralMeshComponent (enable the ProceduralMeshComponent plugin and add the module) and UKismetProceduralMeshLibrary::SliceProceduralMesh with CreateNewSectionForCap and a red cap material (white for the Theatrical cut). Plane normal = cross(cut direction, up). Give both halves simple convex collision, simulate physics, push them apart along +/- the normal, remove them after about 6 s, and keep at most about 60 pieces.
  - Gotcha: CopyProceduralMeshFromStaticMeshComponent only works in packaged builds when the static mesh has Allow CPU Access turned on, and the engine's basic shapes don't. Generate the plushie body in C++ instead (a UV sphere) with proper UVs (u around, v top to bottom, seam at the back), normals and tangents, so a tiling fabric texture can go on it in T4/T7. Test slicing in a packaged build on Saturday, not on Sunday.
  - Fallback if slicing fights back: two premade half-sphere meshes with a red flat face.
- Data: Content/Data/KillingPart.json (nights, waves, plushie types, cards, screen texts), loaded with FJsonObjectConverter. Packaging stages Content/Data (see CLAUDE.md).
- Save: USaveGame (UKPSaveGame, slot KillingPart) for night bests and best combos, won nights, the current and furthest night, week complete and the chosen cut.
- Console commands for testing, prefixed kp.: kp.Night N, kp.Wave N, kp.God, kp.Cut theatrical|director, kp.ResetSave, kp.Pause, kp.Ui <action> (next, continue, newweek, nightselect, night:N, hardcore, choose:director, playagain, title, ...).

## Build order

Claude Code works through these in order. Each task ends with a build, a quick test and a dv commit.

### T1. Clean up and rename (Unreal closed)

Card Island is archived, not deleted.

1. Run dv status. If anything is uncommitted, inspect dv diff and commit it as "Card Island: final state before the pivot".
2. Search for references first, then move:
   - Source/CardIsland/Cards to _Archive/CardIsland/Source/Cards
   - Content/Data/Cards.json to _Archive/CardIsland/Data/Cards.json
   - Content/Maps/Lvl_Island.umap to _Archive/CardIsland/Content/Maps/Lvl_Island.umap
   - Content/Island (the blockout materials) to _Archive/CardIsland/Content/Island
   (IslandLayout.md and a copy of the old CLAUDE.md are already in _Archive/CardIsland.)
3. CardIsland.Build.cs: remove the "CardIsland/Cards" include path.
4. Config:
   - DefaultGame.ini: ProjectName=Killing Part; delete the stale [/Script/CardIsland.CardIslandCharacter] section (those properties no longer exist); add +DirectoriesToAlwaysStageAsUFS=(Path="Data") under [/Script/UnrealEd.ProjectPackagingSettings]; keep the BasicShapes always-cook line.
   - DefaultEngine.ini: point GameDefaultMap and EditorStartupMap at /Game/TopDown/Lvl_TopDown for now (T2 replaces them). Keep the ActiveGameNameRedirects and ActiveClassRedirects.
5. Rename the project so the game builds as KillingPart.exe:
   - CardIsland.uproject to KillingPart.uproject
   - Source/CardIsland.Target.cs to Source/KillingPart.Target.cs (class KillingPartTarget)
   - Source/CardIslandEditor.Target.cs to Source/KillingPartEditor.Target.cs (class KillingPartEditorTarget)
   - Keep the C++ module named CardIsland (renaming the module breaks Blueprint references; not worth it). Don't rename the project folder.
   - BuildEditor.bat: use KillingPart.uproject and the KillingPartEditor target, say "Killing Part" in its messages, and after a successful build launch the editor with the MCP server on: start "" "%UE_DIR%\Engine\Binaries\Win64\UnrealEditor.exe" "%PROJ%" -ModelContextProtocolStartServer
   - Delete Binaries/, Intermediate/ and the stale CardIsland .sln/.slnx files (all generated).
6. Build with BuildEditor.bat and fix anything that breaks. Timebox the rename: if it isn't building cleanly within 20 minutes, undo step 5, keep the CardIsland names, fix the "Names" line in CLAUDE.md, and move on. The exe name is cosmetic.
7. dv commit "Pivot to Killing Part: archive Card Island, rename project".

Leave the template's Variant_Strategy and Variant_TwinStick folders alone for now (TwinStick may have reusable FX). Packaging will list only our maps.

### T2. The core verb (Friday night; goal: playable by 23:00)

- New code goes in Source/CardIsland/KillingPart, classes prefixed KP, with its own C++ GameMode.
- Arena map /Game/KillingPart/Maps/Lvl_Mat, built over the Unreal MCP: the mat, a fixed camera, a lamp light, dim night ambience. Make it the editor startup map and the game default map.
- Plushies made from basic shapes spawn from the edges in waves and walk at her.
- Hold the left mouse button to draw in slow motion, with preview rings and the xN counter. Release to dash, then the beat, then the snip.
- No mesh slicing yet: a snipped plushie pops with a red burst.
- Combo text, score, hearts, chalk bar, simple endless waves, game over, click to restart.
- Match the toy's feel and the numbers above. Then Simon plays it and reports.

### T3. Slicing and gore (Saturday morning)

Procedural plushie bodies, real slicing with a red cap, physics halves, blood bursts, splat decals, the Theatrical cut toggle. Make a packaged test build before moving on.

### T4. Content (Saturday afternoon)

Plushie types (teddy, bunny, big bear that splits, pincushion, golden), nights and their rules from JSON, upgrade cards with loose threads, the card pick between waves.

If Simon has made them by then: put the PBRgen plush fabric material on the plushie bodies (tinted by the pastel colours) and the wood material on the table.

### T4b. Fixes from the T4 playtest (Saturday afternoon)

Card texts in player language, Extra inch replaces Quick hands, stronger Pinking shears, Double stitch's real catch, visible Seam ripper pins. Lights out made really dark with a chalk light. Pincushions last the whole wave, and from Thursday some spawn in her path. New plushie: Patchwork (ignores slow motion). Mother hen and chicks if there's time. Prompt: Prompt_T4b.md.

### T5. Days (Saturday evening)

Built in T5 (Prompt_T5.md): buttons, a workbench (cards for the pool, looks, reroll), a graveyard with epitaphs and reveals, the save, the day screen with the week calendar. T5b (Prompt_T5b.md, Simon's call after playing it) stripped it to one screen: no buttons, workbench, yard, reroll, Lucky button or looks; all cards in the pool from the start; one results screen after every night with the week calendar, the night's numbers and one button; the save keeps the night bests and where you are in the week. See "Between nights".

### T6. Sunday morning (done Sun 01:45 to 02:40, Prompt_T6.md)

Built, in Simon's priority order:
1. Title screen: "Killing Part" over the mat at night, which plays itself behind the menu (attract mode: plushies of every type wander in and she cuts them by herself; this also warms up every material, the cap and the blood effects before the first night). First launch only: "Which version do you want to watch?", Director's cut (blood) or Theatrical cut (no blood, white stuffing), saved. Menu: Continue: <night> (with a save), New week, Night select and Hardcore (after the week is complete), Cut: Director's / Theatrical, Quit; the week score and the Hardcore best. A small "Preparing..." while PSO precaching or a pipeline cache is still compiling (the first launch of a build).
2. Pause (Esc, or P in the editor): the world stops (slow motion kept), the mat blurs; Resume, Cut (live), Restart night, Title screen, Quit.
3. Tutorial: Monday's first wave, first time only: "Hold the left mouse button and drag to draw the cut", "Let go to cut", "Cut several in one line for a combo", and on the first card pick "Pick a card. Green helps, red is the catch".
3b. Meet a new plushie (Simon's idea): the first time ever a wave can bring a type with an intro (pincushion, bunny, patchwork, big bear, mother hen with a chick, the boss), the game holds before it: the mat blurs and a live portrait (its own scene capture) shows the plushie turning under a spotlight with its name and line. Click to continue. Saved per type. Never for the teddy, cub, chick or golden plushie.
4. Sunday's boss, the first one (Sunday's last wave): a huge faded teddy with patches and a mismatched button eye; 40 uu/s, a touch costs a heart and knocks it back 300 uu; every line through it slices a chunk off the side the line passed (plane at 60% of the radius, a cap like any cut) and it shrinks; 500 per chunk (counts in the line's combo), the 6th cut splits it for 3000 with held slow motion, everything on the mat falling apart and "Killing part", then dawn. Seam ripper pins don't hurt it; an escort from Sunday's mix keeps coming. All numbers in its JSON type.
5. The ending instead of Sunday's results: "Sunday, dawn." / "The first one is in pieces. The workroom is quiet.", the week, "Thanks for playing", credits and tools; Play the week again, Night select, Title screen.
6. Night select (title and ending, after the week is complete): the seven nights with their bests; a night played from there doesn't move the saved night and its results offer "Night select".
7. Hardcore (after the week is complete): hearts carry over between nights, losing them all goes back to Monday, score x1.5, its own best (the run total), the week's bests untouched. Not resumable: leaving a run abandons it.

Not built (still on the cut list): the wardrobe. (Night select and the Hardcore button were replaced in T6c.)

### T6b. Fixes from the T6 playtest (Sun 11:27 on, Prompt_T6b.md)

1. Text: the tutorial pill has a fixed size, position and font size (prompts only fade in and out); the cut button says "Cut: Director's (blood)" / "Cut: Theatrical (no blood)" on the title and in pause; the first launch asks "Which version do you want to play?"; a night whose first wave brings a new plushie shows the introduction first, then the banner with only the night's name and rule name.
2. The boss is now the hardest fight of the week: 8 cuts; every chunk cut off gets up after 0.5 s as a small teddy (a cub in its colour) that rushes at you at 200 uu/s; it ignores slow motion and walks at 70 uu/s; an escort from Sunday's mix every 1.2 s; at 4 cuts left it enrages (+50% speed, a second of shaking with a flashing button eye, "It's angry!"). Numbers in its JSON type (chunkInto, chunkDelay, chunkSpeed, enrageAt, enrageSpeed, enrageTell, ignoreSlowMo) and the wave's escortEvery.
3. Game modes (replaces the title's Hardcore button): a screen with one card per mode (name, two-line rule, score multiplier, week best, or why it's locked). Normal: the week, x1. Hardcore: hearts carry over, losing them all goes back to Monday, x1.5, unlocked by finishing the week. Ultra: no cards all week (a loss retries the night), x1.5, unlocked by finishing the week. Insane: Hardcore rules and every pick offers one card you must take, x2, unlocked by beating Hardcore. Each mode keeps its own week best (saved); the title shows the best of the mode picked last. Modes are data ("modes" in the JSON). Mode runs aren't resumable: leaving one abandons it.
4. Plushies slide along a pincushion's edge instead of stalling against it.
- (T6b's modes and boss were redone in T6c.)

### T6c. Modes, the Play flow, the real boss (Sun 13:25 to 14:10, Prompt_T6c.md, the last feature task)

All five chunks built: 1 texts (card-pick tutorial, "Cards reset every night", no heading on a given card); 2 the mode ladder with saves and records, no multipliers, and the darkness after Thursday; 3 the Play flow (Play, the modes screen with Continue/New game, the 3-2-1 countdown, night select removed, "Back to home" after the week); 4 the boss at 50 hits with the spawning rules, the alive cap and the knockback kept on the mat; 5 Extended cut and Extended nightmare. Details in "Modes" and "One night = one run" above.
- Stretch (added in T5b): a wardrobe / customise button in the main menu (dress, shears, chalk and dash trail colours, all available, no unlocking).

### T6d. Modes screen, clear rules, saving, stats (Sun 14:55 on, Prompt_T6d.md, the last menu change)

Two tall mode cards with arrows (week / endless), every mode's rules as bullets, "High score" and "Best time" (active play time of the fastest finished week), Continue showing night or round and score, endless runs saved at every round, the countdown only after Continue, "Save & quit to home" / "Save & quit game" in pause, the boss renamed The Firstborn. Details in "One night = one run" and "Modes" above. No gameplay numbers changed (the boss stays as Simon approved it).

### T7. Juice and audio

SFX, music, shake, hitstop, her look, camera cuts on big combos, polaroids if there's time.

- Audio from WeLoveIndies (free for non-commercial use, fine for the jam): the snip, squeak, squish, whoosh, chalk scratch, hurt, UI clicks, a cozy night track and a music-box tune for the results screen and menus. Note each file's source for the credits.
- Props from Tripo (AI 3D, Simon generates them, Claude Code imports them): proper shears (blades in a V, ring handles), thread spools, desk lamp, pins, buttons. Static props only; the plushies stay procedural for slicing.
- Parked for T7: plushies look grey under the night light, she's small from the camera, the dash trail is a solid band instead of fading.
- Stretch, only if T5 finished on time and strictly timeboxed: a Tripo auto-rigged heroine retargeted to the mannequin animations, replacing the Quinn recolour.
- From the T4 playtest: a nicer card design (the cards are plain cream boxes now), and the table's repeating wood pattern (bigger tiles, random rotation or macro variation).
- Parked ideas, only with time to spare: Double stitch (the line is cut twice, ending where it started; out of the card pool since T4c because it fights the Patchwork and nothing survives the first pass anyway; the code still works, kp.Card doublestitch; maybe for the Sunday boss), walking pincushions (with legs), plushie variants (golden big bear, patchwork big bear whose babies are patchwork too), a card that makes the mat bigger (needs a camera change, so it's more for Extended cut).

### T7d. Her look, camera cuts, variants, the impostor (Sun 19:42 on, Prompt_T7d.md)

Done, all nine chunks: shears close on the snip (no punch), the seamstress look, camera cuts on 5+ (setting), the wardrobe, variants, the impostor, why you lost, curse modes per wave (Demon, Impossible), the blinking pincushion in the dark.

### T8. Ship (from Sunday 16:00)

T7c (Sun 18:46 on, Prompt_T7c.md), the last game task: night music is one looping track (music_night_b removed), chicks out of the combo, hitstop and snip shake 1.5x stronger, the hen keeps her distance, Impossible and Godlike (curses), the blinking pincushion.

Package for Windows with only our maps listed, test the zip in a clean folder, record the video, build the itch page, finish the dev log and tools list. The jam's real deadline is Sunday 15 Nov 5:59 PM PST = Monday 16 Nov 02:59 CET; Simon's own target is submitted by about midnight CET, leaving a buffer.

Itch cover image and title art can be made with fal (image generation), original designs only.

First launch: the packaged exe compiles shaders for about 40 s before anything moves (found in T3). A judge may think it froze. Hide it behind a title or loading screen that shows it's working, try a bundled PSO cache if that's quick, and mention it on the itch page.

Done (Sun 23:00 to Mon 02:00, Prompt_T8.md). The shipped state, v1.0 jam build:
- Practice on the modes screen: any night, Monday to Sunday, with Normal rules. Nothing is saved.
- Release: a Windows Shipping build (no console, kp commands, F8 sound test or test switches) zipped as Saved/Release/KillingPart_Windows.zip (364.9 MB, with README.txt). A Development package for the video is in Saved/Release/Staged_Development.
- First-launch defaults: the cut question, Camera cuts Off, volumes 80%, only Normal unlocked. "Preparing..." shows while shaders compile. The window title is "Killing Part", with "v1.0 jam build" on the title screen.
- Clean-folder test passed (see DevNotes). Itch art in Saved/Release/Screens: 7 screenshots, cover.png, killing_part.gif, cast.png.
- Content: 7 nights, 18 plushie types (incl. 4 variants and the impostor), 3 pincushions (plain, golden, blinking), The Firstborn, 8 week modes, 2 endless modes, 7 cards and 4 curses, both cuts, the wardrobe (shears and dash trail).

## Cut list (cut from the top if we're behind)

1. The wardrobe (the modes and Extended cut were built in T6c)
2. Polaroid director's cut
3. Cinematic camera cuts
4. Mother hen and chicks (built)
5. Golden plushie (built)
6. ~~The boss becomes a big final wave~~ (not needed: the boss was built in T6)
7. Seven nights become three (Monday, Tuesday, Sunday)

## After the jam

To do:
- Split the heroine into parts again, so the wardrobe can recolour her dress, apron and hair (those rows' code and saved picks are kept).
- More combined variants, and walking pincushions beyond the impostor.
- A name for the heroine.
- Bundle a PSO cache so the first launch doesn't need the 40 s "Preparing...".
- Audio stays muted while the window is unfocused (Unreal's default): consider an option.
- Remove the template's leftovers (Variant_* folders, the template classes) and rename the C++ module to KillingPart (it needs the Blueprint references fixed).

Decided against for now (T7c):
- Right-click to cancel a line: time is nearly frozen while drawing, so a free cancel removes the commitment of each line.
- The mother hen with 5 hearts, or bursting into chicks when cut: too close to the boss and the big bear.

## Submission checklist

- Windows build zip (KillingPart.exe)
- itch page: title, tagline, 3-5 screenshots, a GIF, controls, and a content note ("cartoon gore; the Theatrical cut turns it off")
- A 30-60 s video
- Dev log (Glass Box): https://claude.ai/artifact/GKNj1atX46adSjmavHvWye
- Tools list: Unreal Engine 5.9, Claude (chat and Claude Code), Diversion, plus the music, SFX and font sources, and every partner tool actually used (PBRgen, WeLoveIndies, Tripo, fal)

## Partner perks (from the jam's perks list)

| Tool | Use it for | Deadline |
| --- | --- | --- |
| PBRgen | plush fabric material for the plushies, wood for the table (T4/T7) | access ends Nov 15 |
| WeLoveIndies | SFX and music (T7); free licence is non-commercial only | none |
| Tripo | AI 3D props: shears, spools, lamp, pins, buttons (T7) | redeem by Nov 16, Pro ends Nov 17 |
| fal | itch cover and title art (T8) | claim by Nov 30 |
| Anthropic API credits | emergency backup if Claude usage runs out: Claude Code can sign in with a platform.claude.com account | claim by Nov 27 |
| Diversion | version control (already in use) | redeem by Nov 22 |

Not used: Meshy and Hyper3D (same job as Tripo), iClone and Rokoko (no custom animation), Cursor/Grok, NetApp.
