# T5 prompt (from Simon): the day ("broader, not stronger")

Build the day between nights. Design decision (Simon, Sat 19:05): the day makes you broader, not stronger. Buttons never buy hearts, chalk or other power; they buy new cards for the card pool, looks, and a reroll. Losing a night is a pure retry, so you reach Sunday by getting better. Goals the game shows: finish the week, fill the graveyard, beat your week score.

All numbers and texts go in KillingPart.json so Simon can tune them. Same routine per chunk: build, quick test, dv commit (include this file in the first), CLAUDE.md Status, DevNotes lines and screenshots in Saved/Screenshots/KP. Run `date` at each commit: if it's past 22:30, finish the current chunk and stop. When done: update DESIGN.md to match ("The day" section, the shop, the pillars line about buttons, T5, cut list), then package a Windows build and run the exe once, including quit and relaunch to check the save.

Player text rules from T4d still apply: no "she"/"her" for the heroine, numbers instead of vague words, the same vocabulary as the cards (line, chalk, cut).

## 1. Buttons and the save
- Plushies drop buttons when cut (each type's "buttons" in the JSON): teddy 1, bunny 1, patchwork 2, big bear 3, mother hen 5, chick 0, golden 15. A small button pops out of the cut and flies to a button counter in the HUD.
- Buttons are kept whether the night is won or lost.
- USaveGame: buttons, graves and kill counts per type, revealed and bought workbench items, equipped looks, each night's best score and best combo, the current night, the furthest night reached, week completed. Saved at dawn, after a loss, and after every purchase. On launch, continue from the saved current night.
- Console: kp.Buttons N, kp.ResetSave, kp.Grave <type>, kp.Day.

## 2. Dawn screen and score
- Won night: the dawn screen shows the night's score, that night's best, the week score, the night's best combo, buttons earned, and any new graves (with what they revealed). Then "Into the day".
- Lost night: "Hugged to bits" with the same numbers, then "Into the day". After the day the same night is replayed.
- Each night keeps its best score ever. Week score = the sum of the best score on each night (attempts don't add up).

## 3. The graveyard (the yard)
- The first time a type of plushie is cut, it gets a grave: a small tombstone drawn in Slate with the plushie's name, a colour swatch or little picture of it, its epitaph, and a kill count ("143 buried") that keeps counting on every later night. A new grave appears with a small pop on the dawn screen.
- Every grave reveals something in the workbench (a card or a look). The grave says what it revealed.

| Grave | Epitaph | Reveals in the workbench |
| --- | --- | --- |
| Teddy | Wanted a hug. Got a seam. | Card: Extra inch |
| Pincushion (when you cut one) | Pointless, in the end. | Card: Magnet |
| Patchwork | Never slowed down. Until now. | Card: Seam ripper |
| Golden plushie (gold grave) | Worth every button. | Card: Lucky button (build it now: +50% buttons this night / the next card pick offers 2 cards instead of 3), and the look: golden shears |
| Bunny | Hopped once too often. | Look: dash trail colours |
| Big bear | Went to pieces. Twice. | Look: chalk colours |
| Mother hen | Counted her chicks before they hatched. | Look: shears colours |
| Chick | Never even got stuffed. | Look: dress colours |

## 4. The workbench
A Slate screen in the day. Items drawn like the cards, with a price in buttons. Can't afford: greyed out. Not revealed yet: a locked silhouette with a hint ("Bury a teddy").
- Cards: buying one adds it to the card pool for all future nights (it can then be offered between waves; it's not given to you). The pool starts with Long chalk, Pinking shears, Thimble, Tape measure. Prices: Extra inch 20, Lucky button 30, Magnet 40, Seam ripper 50.
- Looks: one purchase unlocks a set of 4 colours (pastel and bold options that fit the look), then pick any of them for free; the pick is saved. Price 25 each. Golden shears 60.
- Reroll: 35, available from the start. One free reroll on each card pick (a "Reroll" button under the cards).
- Judges play about ten minutes, so a normal Monday must earn at least 25 buttons: grave, then buy Extra inch, then it can appear on Tuesday. If Monday earns less, lower Extra inch's price, not the drops.

## 5. The day screen and the week calendar
- The day: the lights come back on, warm and calm. Three tabs or buttons: "Workbench", "Yard", "Next night" (or "Try again: <night>" after a loss).
- A week calendar across the top: Monday to Sunday, each with its best score (or a dash), won nights marked, the next night highlighted. Under it: the week score and "Graves 3/8".
- After Sunday: a simple "Week complete" placeholder with the week score (T6 makes the real ending and night select).

## Rules
Don't change the night gameplay. When done, tell me what to play to see each part (and how to reset the save).
