# T3b prompt (from Simon): make the halves read as cut plushies

Simon's playtest of T3: it plays exactly as before, good. But the halves "look super untextured, and just like pieces of sausage": seen from the high camera, a half lying red side up is a flat red disc. Keep what works: seeing a plushie's button eyes on a half is gory and creepy, and he likes it.

Timebox: about 60-90 minutes, then T4. Build, test, dv commit (include this file), add DevNotes lines. Save before/after screenshots to Saved/Screenshots/KP (same kind of cut, Director's and Theatrical) so Simon can compare them.

1. Cap material, Director's cut. Instead of flat red, make it layered from the rim inwards:
   - a thin outer band in that plushie's own pastel colour (the fabric skin, about 6-8% of the radius),
   - a thin pale pink/white band of stuffing just inside it,
   - the centre glossy dark red (low roughness, so it looks wet) with darker blotches from a noise texture.
   Hint: every point on the cap's rim lies on the sphere, so (distance from the body's centre) / (body radius) is 1 at the rim for any cut, centred or not. That gives the radial bands without depending on the cap UVs. This needs the body centre, radius and colour per half, so each half gets its own dynamic material instance (today every cap shares one).
2. Cap material, Theatrical cut: same pastel rim band, then white fluffy stuffing (noise, slightly lumpy), brightened enough to read as white under the night light (the EyeAdaptationInverse trick, but don't make it fully unlit).
3. How the halves settle: right now they're forced to land red side up, which makes the disc look. Try letting them settle at a natural tilt (cap facing sideways or partly up, so you see the dome, an ear or an eye and some of the cut face), and keep red side up for maybe a third of them. Show Simon a screenshot of each version; he picks.
4. Plush fabric on the bodies: if Simon has put a PBRgen fabric material in the project (ask him), apply it to the bodies and the halves' outer surfaces, tinted by the pastel colours. If not, skip this and say so.

Don't change gameplay numbers.
