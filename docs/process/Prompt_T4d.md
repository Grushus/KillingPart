# T4d prompt (from Simon): small polish before T5

Quick fixes, about 30 minutes, then T5. Build, quick test, one dv commit (include this file), DevNotes lines, screenshot of the card pick after the fixes.

## 1. Words
- Never call the heroine "she" or "her" in player-facing text. SKPHud.cpp line ~327: "Hold the left mouse button and drag to draw her cut" becomes "Hold the left mouse button and drag to draw the cut". Check every other player-facing string (HUD, JSON intros/rules, cards) for she/her meaning the heroine. "Cut her first" in the Mother hen intro is about the hen and stays.
- Sunday's rule text shows a dev note to players: "Everything at once (the boss comes in T6)" becomes "Everything at once".
- Saturday: "30% less chalk, and more of them" becomes "-30% chalk, more plushies" (numbers, no vague "them").

## 2. Card layout
- Remove the "boon" label (nobody knows the word, and the green text already says it's the good part). Keep the "loose thread" label above the red text.
- The loose thread must sit at the same height on every card: give the boon a fixed-height slot (room for two lines) instead of stacking by text height.
- Thimble: show the cap, "+1 [heart] (max 5)", and don't offer Thimble when hearts are already at the max.

## 3. Numbers
- Thimble's loose thread: "+30% plushies per wave" (was +1 plushie, too cheap). Per level, like the other cards.
- Move every card's numbers (boon and loose thread amounts, Extra inch cone angle and length per level, Magnet, Seam ripper radius) from code into KillingPart.json, with the card texts built from those numbers where practical, so Simon can balance cards by editing the JSON and restarting Play.

## 4. Seam ripper pins
Pin heads in mixed colours (pick from the pincushion's pin colours), not always pink.

## 5. Mother hen (Simon: still a bit too easy)
- She arrives with 3 chicks that burst out and rush straight at the player as soon as she enters (these 3 don't scatter first).
- She starts laying the moment she spawns, also while still outside the mat (every 1.5 s, scattering as now).
- Check Saturday's last waves (2 hens) stay beatable; if it's unfair, lower Saturday's hen count rather than her strength, and tell me.
