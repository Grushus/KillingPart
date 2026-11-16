# T7c prompt (from Simon): last tweaks before shipping

Simon played T7b: dash trail, pastel plushies, table, cards, golden pincushion, enrage, game over and chick peep all approved. This is the last game task before T8 (shipping). Hard stop: run `date` at each commit; if it's past 20:00, finish the current chunk and stop. Same routine: build, quick test, dv commit (include this file), CLAUDE.md Status, DevNotes lines. At the end, update DESIGN.md (hen, combo rule, the "After the jam" list below) and package a Windows build and run the exe once.

## 1. Quick changes
- Night music: remove music_night_b (asset, sound test, credits, JSON). music_night_a simply loops.
- Chicks no longer count in the combo (they give no score, and counting them let a line through a crowd of boss-fight chicks score a huge combo: 10 chicks + 1 plushie was 100 x 11 x 11). A line that cuts only chicks shows no combo text. The golden pincushion still counts.
- Simon didn't notice the hitstop or the screen shake. Make both about 1.5x stronger (longer freeze, bigger shake), still tasteful, numbers in the JSON.

## 2. Mother hen keeps her distance
She's still too easy to just cut on the way in. New behaviour: she walks in until she's about 650 uu from the player, then keeps that distance, circling and backing away if the player comes closer, while laying chicks every 1.5 s (and she still arrives with 3 rushing chicks). Killing her first now takes a committed dash across the mat. Numbers in the JSON. Check the bot can still beat Saturday.

## 3. Two curse modes after Final cut (Simon's idea, cheap: reuses the cards' loose threads)
Add two week modes at the top of the ladder, same rules as Final cut (no boons, 1 heart, lose it: start over from Monday), plus curses:
- Impossible (unlock: finish Final cut): at the start of each night, pick 1 of 3 curses. A curse is an existing card's loose thread on its own, without its boon (e.g. "Plushies +10% speed", "-15% chalk", "+30% plushies per wave", "+1 pincushion per wave"). Curses last that night, like cards.
- Godlike (unlock: finish Impossible): the same, but 1 random curse each night, no choice.
Curse cards use the card style with only the red part. Bullet rules on the modes screen like the others, ending with "Lose your heart: start over from Monday". Their own saves and records.

## 4. Only if it's before 19:30: the blinking pincushion
A pincushion variant that teleports, from Thursday on (about 1 in 4 pincushions; Extended cut from round 20). It's purple (fabric and pin heads), not red, so it reads as its own thing. Every 4 s it vanishes and reappears elsewhere on the mat. 1 s before, a faint ring marks where it will appear and pulses twice in that second, each pulse with a quiet, lower-pitched sfx_tick (so it doesn't sound like the countdown; at most one blink sound at a time, however many are on the mat). Same rules as a pincushion otherwise (Magnet works). A short introduction like the other new hazards. Numbers in the JSON.

## After the jam (write into DESIGN.md, don't build)
- Right-click to cancel a line: decided against for now (time is nearly frozen while drawing, so a free cancel removes the commitment of each line).
- The hen with 5 hearts or bursting into chicks: decided against (too close to the boss and the big bear).

When done, tell me what to look at.
