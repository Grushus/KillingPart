# T7b prompt (from Simon): sound picks, test commands, golden pincushion, juice

Simon tested T7: "Everything sounds great." Chunks in priority order. Same routine: build, quick test, dv commit per chunk (include this file), CLAUDE.md Status, DevNotes lines, screenshots in Saved/Screenshots/KP. Hard stop: run `date` at each commit; if it's past 19:30, finish the current chunk and stop (T8 shipping starts then; the real deadline is 02:59 CET, Simon's target is about midnight). At the end, package a Windows build and run the exe once.

## 1. Simon's sound picks (write them into the JSON as the defaults)
- Snip: b. Squish: c in the Director's cut, a in the Theatrical cut. Hurt: a. Killing part: b. Click: b. Everything else as it is.
- Remove music_night_c entirely (file, asset, sound test option, credits).
- Night music: play music_night_a; when it ends, go on to music_night_b instead of looping; when b ends, go back to a, and so on. When a new wave starts while b is playing, crossfade back to a. (b only exists so very long fights don't loop.)
- The snip on "go" at the end of the 3-2-1 countdown is intentional, keep it.
- Two new sounds from Simon in SourceArt/Audio/new: enrage.wav and gameover.wav (WeLoveIndies too). Move them to sfx/ as sfx_enrage and sfx_gameover, import, add to CREDITS.txt. sfx_enrage plays on the boss's enrage tell; sfx_gameover on losing a night or a run ("Hugged to bits"), with the night music fading out.
- Still missing, reuse: a chick's peep = sfx_squeak pitched up and quieter (Simon approved); a new-plushie introduction = sfx_cardhover. All in the JSON so they can be swapped.

## 2. Between nights and test commands
- The results screen between nights gets a "Save & quit to home" button under "Next night" / "Try again" (not on card picks).
- kp.UnlockAll: unlocks every mode (for testing). kp.LockAll puts the unlocks back to what the save had earned.
- kp.TestEnding and kp.TestRunOver: show the week's ending screen and the endless run-over screen right away with made-up stats, so Simon can check the results sound and the stat reveal timing without finishing a week.

## 3. Golden pincushion (Simon's idea, approved)
- About 2% of pincushions spawn golden (JSON). Gold fabric, gold pin heads, a faint sparkle so it's noticed (also as a glint in the dark).
- Cutting through it costs a heart like any pincushion, but gives a big score: 3500 (JSON), shown with gold "+3500", and plays sfx_killingpart_a. Magnet works on it too (no heart lost, and it still scores).
- It works in combos: it counts as one more in the line's combo (xN), and its 3500 is added on top of the combo score.
- No introduction screen: like the golden plushie, it's a surprise.

## 4. Juice, in this order, as far as time allows
1. Hitstop: a few frames' freeze at the snip, longer for bigger cuts; small screen shake at the snip, bigger on 4+ and on the boss's final split (keep it tasteful, JSON numbers).
2. The dash trail fades out along its length instead of being a solid pink band.
3. Plushies look grey under the night light: bring their pastel colours back (more fill light on the mat or more self-light in M_KPPlush), and make the Theatrical stuffing read white, not cream.
4. The table's walnut pattern visibly repeats: bigger tiles plus random rotation or a macro variation.
5. Cards and menus look a bit plain ("a bit Roblox"): a nicer card style (a paper texture or subtle stitched border, a slight shadow), and if a rounded OFL font (Fredoka or Baloo 2 from Google Fonts) can be fetched quickly, use it for headings and add it to the credits.

Don't change gameplay numbers (except the golden pincushion). When done, tell me what to look at.
