# T6d prompt (from Simon): modes screen, clear rules, saving, stats

Simon played T6c. The boss is now great ("you actually have to move around and dodge"; he got it to 33 hits left without upgrades): don't touch its balance. Fixes below. This is the last change to menus and modes; after it only audio (T7) and shipping (T8). Hard stop: run `date` at each commit; if it's past 16:30, finish the current chunk and stop. Same routine: build, quick test, dv commit (include this file), CLAUDE.md Status, DevNotes lines, screenshots in Saved/Screenshots/KP. At the end, update DESIGN.md and package a Windows build and run the exe once.

## 1. Modes screen: less cluttered
Simon finds seven cards at once too clogged.
- Two tall (portrait) cards side by side: the left card is the week modes, the right card is the endless modes. Each card has left/right arrows to step through its modes (left: Normal, Hardcore, Brutal, Extreme, Insane, Final cut; right: Extended cut, Extended nightmare). Locked modes can be browsed too, shown greyed with the unlock reason.
- Under each card: "Continue" (only with a save) and "New game", for the mode currently shown.
- Back arrow top left stays. The "Made for the 2026 Epic AI MegaJam" footer must not overlap any button.

## 2. Rules as bullet points
No more "Like Hardcore, but..." (Simon finds it ambiguous). Each mode lists its own complete rules as short bullets, in plain English for non-native players. Use these (in the JSON):

Normal
- 7 nights, Monday to Sunday
- After each wave, pick 1 of 3 cards
- Cards and hearts reset every night
- Lose all hearts: replay that night

Hardcore
- 7 nights, Monday to Sunday
- After each wave, pick 1 of 3 cards
- Cards reset every night, hearts don't
- Lose all hearts: start over from Monday

Brutal
- 7 nights, Monday to Sunday
- After each wave you get 1 random card, no choice
- Cards reset every night, hearts don't
- Lose all hearts: start over from Monday

Extreme
- 7 nights, Monday to Sunday
- 1 random card at the start of each night, none after waves
- Hearts don't reset between nights
- Lose all hearts: start over from Monday

Insane
- 7 nights, Monday to Sunday
- No cards
- Hearts don't reset between nights
- Lose all hearts: start over from Monday

Final cut
- 7 nights, Monday to Sunday
- No cards
- Only 1 heart
- Lose your heart: start over from Monday

Extended cut
- Endless rounds, 1 more plushie every round
- New plushies along the way, bosses from round 30
- Pick 1 of 3 cards every 10 rounds, they last the whole run
- Hearts don't reset
- Lose all hearts: the run ends

Extended nightmare
- Everything from Extended cut
- Every round is dark

Every mode's last bullet follows the same pattern, "Lose all hearts: <what happens>", so the modes compare at a glance.

Unlock texts: "Finish Normal to unlock" etc.; Extended nightmare: "Finish round 50 in Extended cut to unlock".

## 3. Stats: high score and best time
- Labels: "High score" instead of "Record".
- Week modes also track active play time for a finished week: real time while the night is being played, drawing included (counted in real time, not slowed time). Not counted: countdown, card picks, pause, introductions, results screens. Shown as "Best time" (the fastest finished week). Endless modes keep "High score" and "Best round".
- "Continue" shows where the save is and its score so far, e.g. "Continue: Tuesday, 10,050" (endless: "Continue: Round 23, 41,200").
- The results screen and the ending also show the time.

## 4. Saving and the countdown
- Extended cut and Extended nightmare runs can be resumed: save at the start of every round (round, hearts, cards and levels, score, time); Continue starts that round again.
- The 3-2-1 countdown only after Continue (loading a save), not before every night.
- Pause menu: "Save & quit to home" and "Save & quit game" (replacing "Title screen" and "Quit"), so players know progress is kept. Keep Resume, the cut toggle and Restart night/round.

## 5. Two small updates
- The boss is renamed "The Firstborn" (Simon's pick): its name, the intro, the HUD label, anywhere else it shows. The ending line becomes "The Firstborn is in pieces. The workroom is quiet."
- The real jam deadline is Sunday 15 Nov 5:59 PM PST = Monday 16 Nov 02:59 CET (not 22:00 CET). Update the deadline in CLAUDE.md and DESIGN.md (T8) to that, with Simon's own target: submitted by about midnight CET, leaving a buffer.

Don't change gameplay or numbers. When done, tell me what to look at.
