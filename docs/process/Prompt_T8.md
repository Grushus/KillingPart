# T8 prompt (from Simon): ship it

Feature freeze: no new features (except Practice in chunk 0). Only fixes for anything broken. The jam's deadline is Monday 16 Nov 02:59 CET; Simon wants to submit around midnight to 01:00. Build, test, dv commit per chunk (include this file), CLAUDE.md Status, DevNotes lines. Tell me right away if something blocks shipping.

## 0. Practice (for judges short on time)
Judges play about 10 minutes and would never see Thursday's darkness or the boss. Add a "Practice" button on the modes screen (under the two cards): pick any night, Monday to Sunday, and play it with Normal rules. Practice never saves progress or records, never unlocks anything, and says so in one line ("Practice: any night, nothing is saved"). No god mode or other cheats; the real game stays as it is.

## 1. Release build
- Package a Windows **Shipping** build (no console, no F8 sound test, no kp commands, no on-screen debug). If Shipping breaks anything (JSON loading, saves, fonts, audio) and it isn't a quick fix, fall back to a Development build and tell me.
- Keep a Development package too, in a separate folder (Simon records the video with it, using kp commands).
- Before packaging: player defaults are right (first launch asks the cut question, camera cuts Off, volumes 80%, only Normal unlocked), no test saves or dev unlocks are baked in, only Lvl_Mat is cooked.
- The window title and exe name: Killing Part / KillingPart.exe. A version line on the title screen, small, e.g. "v1.0 jam build".
- The 40 s first-launch shader compile still shows "Preparing..." on the title, never a frozen screen.

## 2. The zip
- Zip the Shipping build as Saved/Release/KillingPart_Windows.zip, with the exe at the top level of the zip's folder.
- Add README.txt next to the exe: how to play in 5 lines (hold left mouse to draw, let go to cut, combos, don't get touched, don't cut pincushions), Esc to pause, "First launch compiles shaders for about 40 seconds, it isn't frozen", the content note (cartoon gore; the Theatrical cut has white stuffing and no blood), Windows 10/11 64-bit, credits.
- Report the zip size.

## 3. Clean-folder test
Extract the zip into a new empty folder outside the project (e.g. C:\KPTest), run the exe from there, and check: first launch (cut question, Preparing, title), a full Monday with a card pick, the results screen, pause and Save & quit, relaunch and Continue, both cuts, audio plays, the heroine model shows. Then delete the test folder and the test save so Simon starts fresh.

## 4. Screenshots and a GIF for itch
From the Development build (use kp commands to set up scenes), 1920x1080, into Saved/Release/Screens:
1. a line drawn through 4-5 plushies in slow motion (preview rings and the xN counter visible),
2. the moment after a big cut: halves flying, blood, "Killing part" text,
3. a card pick,
4. a Lights out wave with the chalk light,
5. The Firstborn boss fight,
6. the title screen and 7. the modes screen.
Plus cover.png at 630x500 (shot 2 cropped, with "Killing Part" in the title font) and a short GIF (about 6 s, 800 px wide, under 8 MB) of one full draw, dash and snip. Check `ffmpeg -version`; if it's missing, install it with `winget install Gyan.FFmpeg` (open a new shell afterwards so it's on the PATH).

And a cast picture, cast.png, 1920x1080: every plushie and hazard as a framed portrait in a grid (reuse the meet-a-new-plushie portrait captures), each with its name and one short line, like a character select screen but calmer: the dark plum background, cream cards with a stitched edge, the Fredoka font, a heading "The cast". Include: teddy, bunny, patchwork, big bear (and cub), mother hen (and chick), golden plushie, impostor, pincushion, golden pincushion, blinking pincushion, and The Firstborn as the biggest frame. Variants can be one frame together ("Variants: patchwork bunny, golden big bear...").

## 5. Final docs
Update DESIGN.md (the shipped state, the after-the-jam list) and CLAUDE.md. Make sure DevNotes.md has the last entries up to now.

When done, tell me: the zip path and size, the clean-folder test result, and the screenshot paths.
