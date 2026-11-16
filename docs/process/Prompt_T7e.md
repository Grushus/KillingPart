# T7e prompt (from Simon): fix her look, effects HUD, small fixes

Simon played T7d. No punch: good. Shears: good. Impostor: worked and looked great. But her look is a "no": the mannequin with a board for an apron, a plum face and a huge bow looks bad. Last game task before T8; about 45-60 minutes. Build, quick test, dv commit per chunk (include this file), CLAUDE.md Status, DevNotes lines, screenshots. At the end, package and run the exe once.

## 1. Her look, quick fix (do this first, it's the fallback)
- Remove the board apron, the big bow and the big pigtails.
- Face, neck and arms: a warm skin tone (the mannequin's second material), not plum.
- A real dress silhouette: a flared skirt (a truncated cone or bell from the waist down to about the knees) in the dress colour, with a slightly lighter bodice tint on the torso. A thin apron on the front of the skirt (a slightly curved thin plane, not a box), in the apron colour.
- Pink hair: the head top tinted pink plus two small pigtails (sized like real hair, not balloons) and a small bow.
- Keep the wardrobe working with these parts. Close-up and game-camera screenshots.

## 2. Only if Simon puts a model in SourceArt/Heroine: swap to it
Simon may generate a stylized girl in Tripo (a static textured model, GLB or FBX, standing pose). If it's there:
- Import it, scale it to her current height, and use it as her visible body instead of the mannequin (hide the mannequin mesh, keep the pawn, capsule and hitbox exactly as they are).
- No skeleton: give it procedural life in code: a gentle idle bob and breathing scale, a lean and stretch in the dash direction during the dash, a small squash when the dash ends. The shears attach where her hand is.
- Wardrobe: if the model has one texture, keep only the shears and dash trail rows (hide the others when the model is in use).
- If it looks worse than section 1 from the game camera, keep section 1 and tell me.

## 3. Camera cuts off by default
Default Off. Under the setting, a short description: "Adds a camera effect for combos of 5+". Keep the feature (useful for the video).

## 4. Active effects on screen
During a night, so players remember what's active and how it stacks:
- Bottom left (where the card names are now): each card with its level, e.g. "Extra inch II".
- Bottom middle: the boons added up, in green, e.g. "+50% chalk", "Line cuts 2x wider", "+1 heart".
- Bottom right: the loose threads and curses added up, in red, e.g. "Plushies +20% speed", "+1 pincushion per wave".
Small text, readable, not covering the mat's play area.

## 5. Small fixes
- No blinking (purple) pincushions in dark waves; those spots get normal pincushions instead.
- Golden odds: Simon got 4 golden plushies in one wave of about 10 on Thursday (wave 2). Check whether that was a golden big bear plus its two golden cubs (expected) or a bug in the odds (golden should be about 1 in 40 spawns). Tell me which.

When done, tell me what to look at.
