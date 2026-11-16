# T4 prompt (from Simon): content

Proceed with T4 from DESIGN.md, in the chunks below, in this order (most important first). After each chunk: build, quick test, dv commit (include this file in the first), update the CLAUDE.md Status section, DevNotes lines with screenshots to take (save to Saved/Screenshots/KP). Run `date` at each commit: if it's past 17:30 and chunk 3 isn't committed, stop after the current chunk and tell me where we are, because T5 has to start Saturday evening.

Defaults below are mine; if one turns out to be a bad idea while building, tell me instead of changing it quietly.

## 1. Materials (quick, do first)

Simon made both in PBRgen with the Unreal export. They're in SourceArt/Fabric (T_Soft_Minky_Plush_*) and SourceArt/Wood (T_Worn_Walnut_Wood_*), 2K: BC (base colour), N (normal, DirectX), ORM (R AO, G roughness, B metallic) and H (height). Each folder has a README.txt and an import_pbrgen.py from PBRgen. Read the README for the texture settings; use the script only if it helps, and keep the final assets under /Game/KillingPart/Materials (not /Game/PBRgen). Skip the height/displacement map: our plushie bodies are procedural meshes without Nanite.
- Plush fabric on the plushie bodies and on the halves' outer surfaces, tiled small enough to read as fabric from the game camera, tinted by each plushie's pastel colour (the BC is light neutral grey on purpose). Consider the Cloth shading model with a fuzz colour (a lighter tint of the pastel) on top of the maps: that soft rim is what reads as plush from far away. Keep the night-light readability: the plushies should look more pastel than now, not greyer.
- Wood on the table, tiled so the grain reads at table scale (the cutting mat covers most of it).
- Before/after screenshots from the game camera.

## 2. Plushie types (data in KillingPart.json)

- Teddy: the current plushie.
- Bunny: long ears, moves in hops (pause, hop, pause), so timing the cut matters.
- Big bear: bigger and slower. When cut, the two halves stand up as two small teddies after a short beat, at the halves' positions (a cute "pop", not a fade). Small teddies split normally.
- Pincushion: stationary hazard from DESIGN.md (radius 46 uu, lives 12 s, min(3, wave - 2) per wave). Red warning ring while drawing. Cutting through one costs a heart. A round cushion with pins sticking out.
- Golden plushie: rare (about 1 in 40 spawns from Tuesday on), fast, gold, worth 5x score for now. Buttons come in T5, so leave a hook for them.
- Each type debuts on its night and stays in the mix on later nights: teddy Mon, pincushions Tue, bunny Wed, big bear Fri, golden from Tue.

## 3. Nights, waves and the card pick

- Nights from JSON: a night is a list of waves (type mix, count, speed, spawn interval). Use the toy's formula for the base numbers. Monday: 4 short waves (a judge should finish it in about 2 minutes). Other nights: 5-6 waves.
- Night rules, only on their own night: Tue Pins (first night with pincushions), Wed Hop (bunnies debut, more of them), Thu Lights out (darken the mat, a warm lamp circle around her of about 500 uu radius, plushies outside it are dim silhouettes but their preview rings still show), Fri Big bears, Sat Short chalk (-30% chalk, more plushies). Sun is the boss in T6: for now Sunday is a hard normal night.
- Between waves: pick 1 of 3 cards (Slate, mouse click, readable at a glance: name, boon in one colour, loose thread in another). Cards last for the rest of the night. Hearts carry over between waves.
- Cards to build, in this order: Long chalk, Pinking shears, Thimble, Quick hands, Tape measure, Double stitch, Seam ripper. Skip Lucky button (needs buttons, T5) and Bias cut (only if everything else is done and there's time; ask me first).
- Clearing the last wave shows a simple "Dawn" placeholder screen, then click for the next night (T5 replaces this with the day). Losing all hearts: replay the same night.
- Console: kp.Night N, kp.Wave N, kp.Card <name>, kp.God.

## Rules

Don't change the core feel or the T2/T3 numbers. Big cuts must still feel like one hit with the new types. When done, tell me what to play to see each thing (which night, which console command).
