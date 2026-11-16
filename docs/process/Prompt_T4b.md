# T4b prompt (from Simon): fixes from the T4 playtest

Simon played the whole week on T4 and finished it ("it was difficult", died twice on Friday). Bunnies were great. DESIGN.md is updated with his decisions (cards table, the Wed/Thu nights, the pincushion row, T4b, T5-T7, cut list): include it and this file in the first dv commit. Same routine as before: build, quick test, dv commit per chunk, CLAUDE.md Status, DevNotes lines and screenshots in Saved/Screenshots/KP. Run `date` at each commit: if it's past 17:45, finish the current chunk and stop, because T5 starts this evening. Chunks in priority order:

## 1. Cards
- Rewrite every card's text in words a player can feel, not developer terms. Nobody knows what "the beat" or "cooldown" is.
- Remove Quick hands. Add Extra inch: the end of the line also snips everything within about 120 uu of the line's end point (counts in the combo), so a cut that comes up just short still gets the plushie. Show that circle as part of the chalk preview when the card is held, with preview rings on what it will catch. Loose thread: plushies +10% speed.
- Pinking shears: blade +100% wider (was +50%, too small to notice).
- Double stitch: drop the "beat is 0.15 s longer" catch. The real catch is already there: she ends where she started. Card text: boon "She cuts the line twice", loose thread "She ends where she started".
- Seam ripper: Simon couldn't see the pins. Make them visible: a few bright pins flying from each snipped plushie to the neighbours they cut, with a small hit on each.

## 2. Lights out (Thursday)
Right now the night isn't harder, because everything outside the lamp is still clearly visible. Make it really dark:
- Outside her lamp circle the mat and plushies are close to black. Plushies show only faint glinting button eyes (so you know something is there). Pincushions outside the light are invisible.
- The chalk line lights the mat while you draw: a soft light along the line, about 250 uu to each side, fading over about 1.5 s after the cut. Preview rings on plushies the line will cut still show. Drawing becomes how you see.
- UI stays readable. Check performance with a long line.

## 3. Pincushions
- They last until the wave ends (fade out at the wave's end) instead of 12 s. Simon thought their vanishing was a bug.
- From Thursday on, about a third of each wave's pincushions spawn in the space between her and the nearest group of plushies, so they're in the way.

## 4. Patchwork plushie (new type)
- Ignores slow motion: it moves at real speed while you draw and during the beat. It must look clearly different: sewn from mismatched pastel patches, one big X-stitched eye, a jittery walk, and it stays in full colour while the slow-motion tint is on.
- Normal speed otherwise, maybe a little slower than a teddy so it's fair. Same score as a teddy for now.
- One or two appear as a teaser in Tuesday's last wave; regular from Wednesday. Add a short name/intro the first time it appears if the night intro supports it.

## 5. Mother hen and chicks (only if there's time before 17:45)
- A hen plushie that walks slowly and lays a chick every few seconds. Chicks are small and fast and give no score; the hen gives a high score. Kill her first or get swarmed; leave her for a bigger combo at your own risk.
- Debuts on Saturday.

## Rules
Don't change the T2/T3 feel or the core numbers. When done, tell me what to play to see each change (night and console commands).
