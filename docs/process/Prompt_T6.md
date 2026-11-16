# T6 prompt (from Simon): title, pause, tutorial, the boss, the ending

The game is complete from Monday to Sunday (T5b). T6 turns it into a finished game a judge can start cold. Chunks in priority order. Same routine per chunk: build, quick test, dv commit (include this file in the first), CLAUDE.md Status, DevNotes lines and screenshots in Saved/Screenshots/KP. All words in KillingPart.json "texts". Player text rules: no "she"/"her" for the heroine, numbers not vague words, the card vocabulary (line, chalk, cut). Run `date` at each commit: if it's past 12:30 on Sunday, finish the current chunk and stop (T7 and T8 need the afternoon). When done, update DESIGN.md (T6, cut list) and package a Windows build and run the exe once.

## 1. Title screen and the cut choice (most important: it's the first thing a judge sees)
- Launch opens a title screen over the mat at night (plushies idle or slowly wandering in the background is nice if cheap): "Killing Part" big, and a menu: Continue (shows the saved night, only if there's a save), New week, Night select (only after the week is complete), Cut: Director's / Theatrical, Quit.
- First launch only: before the menu, ask which version to watch, two big choices: "Director's cut (blood)" and "Theatrical cut (no blood, white stuffing)". Saved. This is also the content setting.
- The packaged exe compiles shaders for about 40 s on first launch (found in T3). Show the title screen as early as possible with a small "Preparing..." indicator while that happens, so it never looks frozen. If UE's PSO precaching or a quick warm-up (spawning one of each plushie, a slice and the blood effects off-screen behind the title) helps, do it here.

## 2. Pause (Esc)
Esc pauses (time stops, slow-motion state kept): Resume, Cut: Director's / Theatrical (switches live, as kp.Cut does), Restart night, Title screen, Quit. Esc again resumes.

## 3. Tutorial (Monday's first wave, first time only, saved flag)
Short prompts at the bottom of the screen, one at a time, each disappears when done:
1. "Hold the left mouse button and drag to draw the cut" (until the first line is drawn)
2. "Let go to cut" (until the first cut)
3. "Cut several in one line for a combo" (after the first cut, for a few seconds)
4. On the first card pick: "Pick a card. Green helps, red is the catch"
Nothing else in the tutorial: each new plushie gets its own introduction instead (3b), so there's still something to discover.

## 3b. Meet a new plushie (Simon's idea)
The first time ever a type appears (saved per type), just before the wave that brings it starts: the game holds, the mat behind darkens and blurs, and a big introduction shows the plushie itself (a live 3D one in a spotlight, or a SceneCapture portrait, your pick, as long as it looks good), its name and the one-line intro text that already exists (e.g. "Patchwork: It doesn't slow down when you draw"). Click to continue. For: pincushion, bunny, patchwork, big bear, mother hen (with a chick), and the boss before its fight. Not for the teddy (the tutorial covers it) and not for the golden plushie (a rare surprise stays a surprise). This replaces the current one-time intro lines. Fallback if the big version fights back for more than 30 minutes: a toast at the top of the screen with a colour swatch, the name and the line.

## 4. Sunday's boss: The first one
Sunday's last wave becomes the boss (keep the waves before it).
- The first plushie ever made: a huge teddy (radius about 200 uu), faded, well-loved fabric with patches, one button eye replaced with a mismatched button. Cute and sad, not a horror mascot (DESIGN.md hard rules). Intro line: "The first one: every cut takes a piece off".
- Walks slowly at her (about 40 uu/s). Touching her costs a heart and knocks the boss back about 300 uu.
- Each line through the boss slices a chunk off on the side the line passed (reuse the T3c offset slicing: plane at about 60% of the radius on that side). The chunk flies off with a cap as usual and the boss shrinks a little. 6 cuts: the 6th splits it in half for good.
- Score: 500 per chunk (counts in the combo with whatever else the line cut), 3000 for the final split. Seam ripper pins don't hurt the boss.
- A light escort of normal plushies keeps coming from the edges during the fight (Sunday's mix, low rate).
- The final split gets a big moment: slow motion held a little longer, a big burst, "Killing part" gold text, then dawn.
- Tunable in the JSON (radius, speed, cuts, chunk offset, scores, escort rate). Fallback if the chunk slicing fights back for more than 60 minutes: stop and tell me (the cut list says the boss becomes a big final wave, which is my call).

## 5. The ending
After the boss: an ending screen instead of the normal results. "Sunday, dawn." then "The first one is in pieces. The workroom is quiet." Then the week score, each night's best (the calendar), and "Thanks for playing" with short credits: "Made by omgxdlmao & Claude for the 2026 Epic AI MegaJam" plus the tools (Unreal Engine 5.9, Claude, Diversion, PBRgen, and audio sources once T7 adds them). Buttons: "Play the week again", "Night select", "Title screen".

## 6. Night select
On the title screen and the ending, after the week is complete: the seven nights with their best scores; pick one to replay it (its best can improve, which raises the week score). After a night started from night select, the results screen offers "Night select" instead of "Next night".

## 7. Only if it's before 11:30: Hardcore
A mode on the title screen after the week is complete: hearts carry over between nights, losing all hearts sends you back to Monday, score x1.5, its own week best shown separately. Skip it otherwise (cut list item 1).

Don't change night gameplay or the numbers. When done, tell me what to look at, in order.
