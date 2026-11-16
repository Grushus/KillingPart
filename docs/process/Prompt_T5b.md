# T5b prompt (from Simon): strip the day down to one screen

Simon played T5. It works, but with this little content the workbench and the yard have no purpose: Monday paid 40 buttons, the first card cost 20, every purchase is a formality, and the screens are detours from cutting. Decision (Simon, Sat 20:32): remove them. About 30-45 minutes. Build, quick test, dv commit (include this file), CLAUDE.md Status, a DevNotes line with the decision and why, screenshot of the new screen. Then package and run the exe once (check the save still works across a relaunch).

## Remove
- Buttons entirely: drops, the flying button, the HUD counter, button fields in the save, the "buttons" entries in the JSON.
- The workbench, the yard, graves, epitaphs, kill counts, reveals and unlocks.
- Reroll.
- Lucky button (its boon was buttons).
- Looks/colour sets for now (a wardrobe may come back later in the main menu, with all colours available).
Delete code that's no longer used where that's simple and safe; otherwise disconnect it. Keep the JSON tidy.

## Change
- All cards are in the card pool from the start: Long chalk, Pinking shears, Thimble, Extra inch, Tape measure, Seam ripper, Magnet.
- One screen after each night, won or lost, replacing dawn + the day:
  - the week calendar on top (Monday to Sunday, each night's best score or a dash, won nights marked, the next night highlighted),
  - the night's results: score, that night's best (mark a new best), best combo, and the week score,
  - one button: "Next night" after a win, "Try again" after a loss ("Hugged to bits" as the heading). After Sunday: "Week complete!" with the week score and "Play the week again".
- The save keeps: each night's best score and best combo, the current night, the furthest night, week completed. On launch, go straight into the saved night (T6 adds the title screen).
- Update DESIGN.md to match: remove the day, buttons, workbench and graveyard (pitch, pillars, "The day", T5, cut list). The pillar "Every night counts" becomes about the week score and the calendar. Add under T6 stretch: a wardrobe/customise button in the main menu, and an Insane mode after Hardcore is beaten (Hardcore rules, but each card pick offers only 1 card and you must take it).

Don't change night gameplay. When done, tell me what to look at.
