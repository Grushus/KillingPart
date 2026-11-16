# T3 prompt (from Simon)

Proceed with T3 (slicing and gore), in chunks. DESIGN.md was just updated (partner perks, UV note): include it and this file in your first dv commit. After each chunk: build, quick test, dv commit, update the CLAUDE.md Status section, and add dev-log lines to DevNotes.md (bugs and fixes, and which screenshots to take; save the screenshots you take to Saved/Screenshots/KP).

1. Procedural plushie body: generate the body in C++ (UV sphere) in a UProceduralMeshComponent, looking the same as now. It needs proper UVs (u around the sphere, v top to bottom, seam at the back), normals and tangents, because a tiling plush fabric texture goes on it later (T4/T7). Check the UVs with a temporary checker or grid texture, then put the normal material back. The slice caps need sensible UVs too. Ears, eyes and the seam stay attached parts.
2. Slicing at the snip: SliceProceduralMesh along the cut (plane normal = cross(cut direction, up)), cap in a new section with a red cap material. Each attached part (ears, eyes) follows the half on its side of the plane. Halves get simple convex collision, simulate physics, fly apart along +/- the normal at the DESIGN.md speeds, spin, and are removed after about 6 s. Keep at most about 60 pieces. Check that a 5+ cut doesn't hitch.
3. EARLY PACKAGED CHECK, before going further: package a Windows Development build, run the exe, and confirm slicing, colours and the JSON data all work outside the editor. If slicing fails only in the packaged build, fix that first.
4. Gore: blood bursts at the snip (recolour an existing Niagara system from /Game/Cursor or /Game/Variant_TwinStick/FX if one fits, else a simple one), and make sure splats stay on the mat for the whole night.
5. Theatrical cut: white stuffing cap, white fluff bursts, no splats. A global setting with the console command kp.Cut theatrical|director for now (the menu toggle comes in T6).

Rules: don't change the feel or the numbers from T2. If slicing still isn't working after about 90 minutes of trying, stop and tell me what's failing. Don't switch to premade halves without asking me; that's a cut-list call. When done, tell me what to look at when I play.
