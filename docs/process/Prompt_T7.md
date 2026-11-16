# T7 prompt (from Simon): audio

The game is silent; this task gives it sound. Simon picked the sounds (all from WeLoveIndies, free non-commercial licence, fine for the jam). Same routine: build, quick test, dv commit per chunk (include this file), CLAUDE.md Status, DevNotes lines. Hard stop: run `date` at each commit; if it's past 18:00, finish the current chunk and stop (juice is T7b, shipping T8; the real deadline is 02:59 CET, Simon's target is submitting around midnight). At the end, package a Windows build and run the exe once.

## 1. Rename and import
The files are in SourceArt/Audio/sound/<folder>/ and SourceArt/Audio/music/<folder>/ with long store names. Rename (move, don't delete) them into flat folders SourceArt/Audio/sfx and SourceArt/Audio/music with these names, then import to /Game/KillingPart/Audio:

| New name | Original file | Plays when |
| --- | --- | --- |
| sfx_snip_a | foley-kitchen-large-scissor-snip-single-03 | the snip (start with this one) |
| sfx_snip_b | gore-stab-red-cabbage-knife-pull-out-fast-squish-01 | option |
| sfx_squish_a | foley-food-crystal-noodles-squish-impact-04 | per plushie cut (Theatrical cut default: the softest) |
| sfx_squish_b | foley-food-fresh-lettuce-crunch-02 | option |
| sfx_squish_c | gore-rip-bellpepper-squish-rip-01 | per plushie cut (Director's cut default) |
| sfx_whoosh | foley-whoosh-bamboo-slow-04 | the dash (it's a slow whoosh: trim and/or pitch it up to fit a 0.25 s dash) |
| sfx_chalk_loop | object-plastic-rattle-slow-08 | loops quietly while drawing (Simon isn't sure about it; test) |
| sfx_squeak | animal-mouse-mouse-squeak-05 | a plushie's death squeak (random pitch, not on every plushie in a big cut, max 2 per snip) |
| sfx_hurt_a | impact-body-small-punch-hard-05 | losing a heart (start with this one) |
| sfx_hurt_b | weapons-bullet-large-impact-body-02 | option |
| sfx_killingpart_a | game-casino-slotmachine-ancient-egypt-jackpot-02 | 5+ combo "Killing part" (start with this one) |
| sfx_killingpart_b | game-casino-slotmachine-asia-reel-spin-01 | option |
| sfx_waveclear | game-casino-slot-machine-ui-08 | a wave is cleared |
| sfx_cardhover | game-casino-placing-card-on-poker-table-01 | mouse moves onto a card in a card pick (soft, random pitch) |
| sfx_click_a | ui-click-high-modern-click-04 | every button and picking a card (start with this one) |
| sfx_click_b | ui-click-modern-percussive-click-high-03 | option |
| sfx_tick | object-clock-tick-egg-timer-tick-soft-03 | each number of the 3-2-1 countdown; on "go", play the snip |
| sfx_lightsout | ui-switch-lightswitch-plastic-off-click-05 | a wave goes dark (play it loud and early: the screen goes dark right on the click) |
| sfx_pin | object-metal-chain-metal-chain-on-metal-impact-06 | Seam ripper pins hitting, and cutting through a pincushion |
| sfx_results | game-casino-slotmachine-ancient-egypt-playlines-01 | the week's ending screen and the endless run-over screen: reveal the stat lines one by one in time with it |
| music_menu | kf011612-dead-beds | title, modes, results, ending |
| music_night_a | kf011112-flexible | nights (Simon's first choice) |
| music_night_b | kf013903-sexy-girl | option |
| music_night_c | kf030502-driving-home | option |

Write SourceArt/Audio/CREDITS.txt: each new name, its original file name, "WeLoveIndies (weloveindies.com), free non-commercial licence". Add WeLoveIndies to the credits on the ending screen and to the JSON "tools" text.

## 2. Mixing
- Music crossfades between menu and night. While drawing (slow motion) the night music gets a low-pass filter and drops in volume, and comes back on release: the slow-motion moment should feel muffled. On dark waves, the night music stays low-passed for the whole wave (Simon may later try music_menu for dark waves; make that a JSON switch).
- Short sounds get small random pitch variation so repeats don't feel robotic. A big cut layers one snip + a few squishes, not 10 at full volume.
- Boss hits use the snip + squish too; the final split uses killingpart.
- All volumes, the chosen option per sound, and the switches live in the JSON.

## 3. Volume for players
In the pause menu and on the title screen: Music and Sounds volume, each in steps of 10% (left/right arrows like the modes screen), saved.

## 4. Sound test panel (for Simon only)
A dev panel to try the options in game: opened with the console command kp.SoundTest (and F8), hidden otherwise. It lists every sound role with its options (a/b/c), click to cycle and hear it, a play button, plus the night music track. Picks are saved locally and used right away. When Simon has chosen, he'll tell Claude Code to write his picks into the JSON as defaults. Not shown to players: no button anywhere in the menus.

When done, tell me what to listen for and how to open the sound test.
