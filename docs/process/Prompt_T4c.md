# T4c prompt (from Simon): card language, card levels, small fixes

Simon played T4b. Lights out is amazing, Patchwork works great. Fixes below, then package. Same routine: build, quick test, dv commit (include this file), CLAUDE.md Status, DevNotes lines, screenshots to Saved/Screenshots/KP. Update the cards table in DESIGN.md to match when done. Timebox: about 75 minutes, T5 is next.

## 1. Card language (Simon: "the text on the cards has gotten far worse")
Rules for every card text:
- Numbers, never vague words: "+10%", not "a bit faster" or "a little faster".
- One vocabulary: the "line" is what you draw, "chalk" is how long it can be (the HUD bar says chalk), "cut" is what happens to plushies. Never "snip" on cards, never "she".
- Short, like a stat line. Use the HUD's heart icon inline where a heart is meant ("+1 [heart icon]").

The cards (boon / loose thread):

| Card | Boon | Loose thread |
| --- | --- | --- |
| Long chalk | +25% chalk | Plushies +10% speed |
| Pinking shears | Line cuts 2x wider | -10% chalk |
| Thimble | +1 [heart icon] | +1 plushie per wave |
| Extra inch | Line end cuts a cone ahead | Plushies +10% speed |
| Tape measure | +50% score | Plushies +5% speed |
| Seam ripper | Cut plushies throw pins at neighbours | +1 pincushion per wave |
| Magnet (new) | First pincushion you cut each wave is harmless | -15% chalk |

- Remove Double stitch (it fights Patchwork and nothing survives the first pass). Leave its code in, just take it out of the card pool, and note it under the parked ideas in DESIGN.md.
- Magnet: the first pincushion your line crosses each wave gets cut like a plushie (no heart lost, no score). Its preview ring is a different colour from the red warning, so you can see it's safe.

## 2. Card levels
- A card you already hold can be offered again as level II, then III (max). Show the level on the card ("Extra inch II"). Each level adds the boon again and the loose thread again.
- Extra inch becomes a cone instead of a full circle, pointing forward along the line's last direction: level I 30 degrees and about 70 uu long, II 50 degrees and 100 uu, III 70 degrees and 130 uu. Keep the current dashed preview, as a cone. Simon thought the 120 uu circle was too big for a first pick.
- Pinking shears: show the cut width as a faint band along the line in the preview while the card is held, so players see what it does (Simon didn't get it from the text).

## 3. Seam ripper pins
Simon still can't see the pins. Use the pincushion's pin model, bigger. Each pin visibly flies (a short bright trail, about 0.3 s real time) and then stays stuck in the plushie it hit, and in its halves after the cut, so the player sees afterwards what got pinned.

## 4. Mother hen
She only lays 1 or 2 chicks on her way in, so she isn't worth prioritising. Lay a chick every 1.5 s (was 3), with the first one as soon as she's on the mat.
- Chicks scatter instead of trailing behind her in a line (right now one line clears them all): each chick hops away from the hen in a random direction, about 200-350 uu, then comes at her from that side. Don't let chicks spawn or land within about 300 uu of her (no unfair instant hits). Killing the hen early still pays off, because fewer chicks get out.

## 5. Package
Package a Windows build and run the exe once: check the PBRgen materials, Lights out, Patchwork, the hen and the cards outside the editor. (This was asked for at the end of T4b but didn't happen.)

Don't change the core feel. When done, tell me what to play to see each change.
