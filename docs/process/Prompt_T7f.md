# T7f prompt (from Simon): the real heroine

Simon made her in Tripo (image to 3D from the reference picture): one textured mesh, about 20k triangles, 2k PBR texture, relaxed pose with the arms down. It's in SourceArt/Heroine with a long file name ("Cute stylized chibi girl seamstress, fu... #32.glb"). Simon: "night and day" compared to the mannequin. Last task before T8. Build, quick test, dv commit (include this file), CLAUDE.md Status, DevNotes lines, screenshots. At the end, package and run the exe once.

## 1. Import
- Rename the file to SourceArt/Heroine/SM_Heroine.glb and import it to /Game/KillingPart/Heroine as a static mesh with its material and textures.
- Credit Tripo in the JSON "tools" text and the ending credits (Tripo AI, 3D model).

## 2. She replaces the mannequin
- Hide the mannequin mesh and every T7e/T7d body part (skirt, apron, hair, bow, pigtails). Keep the pawn, capsule, hitbox and all gameplay exactly as they are.
- Attach SM_Heroine to the pawn, feet on the mat, facing her forward direction. Scale it so she reads well from the game camera: about the height the mannequin had at 1.4x (the hitbox doesn't change).
- She turns to face the direction of each dash (smoothly, fast) and keeps that facing while idle.
- Lighting: make sure she reads at night, pastel and bright like the plushies (a little self-light or the same fill light), not dark. Check in a dark wave too: she should stay visible in her lamp circle.
- Shadow on the mat stays.

## 3. Procedural life (no skeleton, no animation assets)
- Idle: a gentle bob and a slow breathing scale (subtle).
- Dash: lean forward into the dash direction and stretch a little along it; a small squash when the dash ends, then settle.
- Hurt: a quick shake/flash as now.
- The shears attach at her right hand's position (find it on the mesh, a fixed offset is fine), still only visible during the dash and the beat, still closing on the snip.
- Everything runs in real time like her old animation (she isn't slowed by slow motion).

## 4. Wardrobe
The model has one baked texture, so only the Shears and Dash trail rows stay; hide the Dress, Apron and Hair-and-bow rows (keep their code and saved values for later). The wardrobe close-up shows the new model.

## 5. Check
Screenshots: game camera in a normal wave, a dark wave, the wardrobe close-up, and mid-dash. If anything about her looks broken (floating, sunk into the mat, wrong facing, too dark), fix it before committing.

When done, tell me what to look at.
