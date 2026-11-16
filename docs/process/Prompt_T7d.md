# T7d prompt (from Simon): her look, camera cuts, variants, the impostor

Send this after T7c is done. Order of work: 1, 2, then 9, 7, 8 (fairness fixes and changes to existing modes first), then 3, 4, 5, 6. No time stop: Simon wants every chunk finished (T8 shipping comes after; the real deadline is 02:59 CET), so work through all of them, committing after each. Same routine: build, quick test, dv commit per chunk (include this file), CLAUDE.md Status, DevNotes lines, screenshots in Saved/Screenshots/KP. At the end, update DESIGN.md and package a Windows build and run the exe once.

## 1. No more punch; the shears close on the snip
After the dash she plays the mannequin's attack clip, which reads as a punch, and it lands after the snip sound. Remove it: she ends the dash in idle. Instead, the shears do the cutting: blades open during the dash and close procedurally (rotate together in code, no new animation) exactly when sfx_snip plays. Give the code shears a nicer shape while you're there: tapered blades instead of thin boxes, ring handles, still the V shape.

## 2. Her look: the seamstress
The original plan from Friday, never built: Quinn as a seamstress. A dark plum dress, a white apron (front of the torso), pink hair: the head tinted pink with two pigtails and a bow on top, made from simple shapes attached to sockets. Chunky enough to read from the high game camera. If she still looks small, try scale 1.4 (the hitbox stays the same). Screenshot from the game camera and a close-up.

## 3. Cinematic camera cut on 5+ (Killing part)
When a line marks 5 or more plushies (and on the boss's final split), the view cuts to a low, close camera following her during the dash and the beat, then cuts back to the game camera before time returns to normal. She can't be hurt during the dash and the beat, and drawing is blocked then anyway, so the player never loses control or a heart to it. A setting "Camera cuts: On / Off" in the pause menu and on the title screen, saved, default On.

## 4. Wardrobe
A "Wardrobe" button on the title screen. She stands in the lamp light; rows with left/right arrows (like the modes screen) for: dress, apron, hair and bow, shears, dash trail. About 6 colour swatches each (pastel and bold, the current scheme as the default). Saved, applies everywhere. No unlocking.

## 5. Variants: combined plushies (Simon's idea)
Rare variants that combine exactly two types' traits, only where both stay readable at a glance. Four of them:
- Patchwork bunny: hops and ignores slow motion (its hops keep going while you draw). Patches, X-stitched eye, long ears.
- Bunny big bear: a big bear that hops; its cubs hop too.
- Patchwork big bear: a big bear that ignores slow motion; its cubs are patchwork.
- Golden big bear: gold, x5 score, normal big-bear speed; its two cubs are golden too (the jackpot).
No hen variants (she already has two behaviours) and no triple combos.
- From Thursday, about 4% of spawns become a variant, only combining types already introduced that week (Extended cut: from round 30). The first time each variant appears, a small toast at the top ("New: Patchwork bunny"), no full introduction.
- All in the JSON (which combos exist, chance, start).

## 6. The impostor (Simon's idea)
A plushie camouflaged as a pincushion, trying to escape the mat. It looks like a pincushion but has little felt feet. Rules, chosen so it's always fair:
- Spawns on the mat like a pincushion, but at least about 450 uu from every edge, and never in a dark wave or the boss wave.
- First it sits still for about 1.5 s with a small wiggle (the tell), then walks slowly (about 60 uu/s, so roughly 8 s to escape) in a straight line to the edge that was nearest when it spawned. It slows down in slow motion like other plushies, so drawing gives you time to react.
- It never attacks: it steers around her, and bumping into her does nothing.
- It's a plushie: cutting it is safe (no heart lost), scores x3 and counts in the combo. The reveal is the wiggle, the feet and the walking; the preview ring on it is a normal white plushie ring, not the red warning, which confirms it's safe to cut.
- If it reaches the edge it escapes: you lose a heart (a small "Escaped!" text at that edge).
- It counts as a plushie for clearing the wave, so a wave can't end with it still walking; it's always resolved within a few seconds (cut or escaped).
- At most one at a time. From Friday, about 40% of non-boss, non-dark waves get one (Extended cut: from round 35, 30% of rounds).
- A full introduction the first time ("Impostor: It's not a pincushion. Cut it before it escapes"). Numbers in the JSON.
- Golden impostor: about 1 in 10 impostors is gold (gold fabric and pins, the sparkle, its feet show). Same rules, x5 score instead of x3.

## 7. Why you lost
The "Hugged to bits" screen (and the endless run-over) gets one short line saying what took the last heart, so players learn:
- a plushie reached you: "A plushie got its hug."
- a pincushion: "You cut through a pincushion."
- an impostor escaped: "An impostor escaped. Cut walking pincushions before they reach the edge."
- the boss: "The Firstborn got its hug."
Words in the JSON.

## 8. Curse modes per wave, renamed (changes what T7c built)
T7c built two curse modes with one curse per night. Simon wants them per wave (harder, and it mirrors the card modes):
- Demon (was Impossible, unlock: finish Final cut): Final cut rules (no boons, 1 heart, lose it: start over from Monday), and after each wave pick 1 of 3 curses.
- Impossible (was Godlike, unlock: finish Demon): the same, but 1 random curse after each wave, no choice. The top of the ladder: Final cut, then Demon, then Impossible.
- Curses level up like cards: a curse you already have can come again as II, then III (max), adding its loose thread again. Curses reset every night.
- Bullet rules on the modes screen like the others, ending with "Lose your heart: start over from Monday". Keep their saves and records (rename the keys if needed).

## 9. The blinking pincushion in the dark
In dark waves it's invisible like the other pincushions, except: its purple pin heads glint faintly (like the plushies' button eyes), and its 1 s warning ring glows purple and lights up that spot a little with each pulse and tick (a small pulsing light), so players see where it will land even in the dark.

When done, tell me what to look at.
