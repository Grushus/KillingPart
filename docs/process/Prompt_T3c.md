# T3c prompt (from Simon): slice where the line actually passed

This was meant to be step 4 of T3b, but it never reached you (the file on disk was the older version). T3b itself looks much better, keep all of it, including halfCapUpShare 0.33.

Simon's playtest: a line that clearly passes near the edge of a plushie still cuts it exactly in half, because AKPPlushie::Slice puts the slice plane through BodyMesh->GetComponentLocation().

Change: the slice plane goes through the point on her dash path closest to the plushie's centre, so a line through the middle gives two even halves and a line near the edge shaves off a small piece.

- At mark time (the dash hit test), store on the plushie the signed perpendicular offset from its centre to the dash segment, along the cut normal (cross(cut direction, up)), next to the cut direction you already store.
- In Slice, use Center + Normal * Offset as the plane origin. The hit test is generous (radius + blade width), so clamp the offset to at most about 75% of the radius, so there's always a visible piece.
- The white cut line drawn across a marked plushie during the beat should sit at the same offset, so the preview matches the result.
- Ears, eyes and stitches must go with the piece they're on, tested against the offset plane, not the centre.
- Check that the halves' physics and cap material look right on a small piece too.

Rules don't change: every plushie the line hits still dies, same score. Timebox about 30 minutes. Build, test, dv commit (include this file), DevNotes line. Screenshots to Saved/Screenshots/KP: one even cut and one edge cut, from the game camera.
