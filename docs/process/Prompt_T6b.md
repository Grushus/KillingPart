# T6b prompt (from Simon): fixes from the T6 playtest

Simon played T6 on Sunday morning. Title, cut choice, pause and the new-plushie portraits work. Fix list below, in priority order. Hard stop: run `date` at each commit; if it's past 12:45, finish the current chunk and stop (T7 audio and T8 shipping need the afternoon). Same routine: build, quick test, dv commit (include this file), CLAUDE.md Status, DevNotes lines, screenshots in Saved/Screenshots/KP. At the end, update DESIGN.md (boss, modes, T6) and package a Windows build and run the exe once.

## 1. Small text fixes (quick)
- Tutorial prompt bar at the bottom: it keeps changing size and position between frames, looks buggy. Give it a fixed size and position and a fixed font size; prompts only fade in and out.
- Title menu cut button: say what each does: "Cut: Director's (blood)" / "Cut: Theatrical (no blood)". Same in the pause menu.
- First-launch question: "Which version do you want to play?" (not "watch").
- When a new-plushie introduction is coming, don't show its description on the night banner first (Simon read "Cut a big bear and both halves get up" at the top, then the portrait said the same). On those nights: show the introduction first, then the night banner with only the night's name and rule name, no description line.

## 2. The boss is far too easy (Simon: "the easiest level of the game", he could cut it 6 times before any other plushie reached him)
Make the finale the hardest fight of the week, reusing what exists:
- 8 cuts instead of 6.
- Every chunk cut off gets up as a small teddy after about 0.5 s (the big bear's split logic) and rushes at the player. Spamming cuts now floods the mat.
- The boss doesn't slow down while you draw (like Patchwork) and walks at about 70 uu/s.
- Heavier escort from the edges (Sunday's mix including Patchwork, about one every 1.2 s).
- At 4 cuts left it enrages: +50% speed and a short visual tell (it shakes, the button eye glints).
- All numbers in the JSON. Check with the auto-cut bot: a bot that only attacks the boss should lose hearts; tell me how many hearts a normal attempt costs.

## 3. Game modes screen
Replace the Hardcore entry on the title menu with a "Game modes" button. The modes screen lists each mode as a card: name, a two-line rule description, its score multiplier, its best week score, and if locked, why ("Finish the week to unlock"). Modes:
- Normal: the week as it is. x1.
- Hardcore: hearts carry over between nights, losing all hearts sends you back to Monday. x1.5. Unlock: finish the week.
- Ultra: no cards at all, all week. x1.5. Unlock: finish the week.
- Insane: Hardcore rules, and each card pick offers only 1 card, which you must take (you see what it is). x2. Unlock: beat Hardcore.
Each mode keeps its own week best (saved). The title screen shows the current mode's week best. Don't show modes that don't exist (no "coming soon").

## 4. Only if there's time: pincushion avoidance
A plushie got stuck on a pincushion for about 3 seconds. Give plushies a simple slide around pincushions in their steering (push sideways along the pincushion's edge) so they never stall.

Don't change the rest of the night gameplay. When done, tell me what to look at.
