# Prompt T9b: WeLoveIndies intro, showcase repo, GitHub release (replaces T9)

Show me the plan, the file list and the README before anything is pushed.

## A clean public showcase repo
New folder: Documents\KillingPart-showcase (copy files there; the Unreal project itself stays on Diversion, not git). Install git and the GitHub CLI with winget if missing, then run gh auth login (I'll log in).

It should look like a professional repo, nothing that looks messy or accidental. Layout:
- README.md, .gitignore (a proper Unreal .gitignore), .gitattributes
- KillingPart.uproject, Config/, Source/ (only code written during the jam: the KillingPart folder and module files, no Epic template Variant_* code)
- Tools/ (the editor Python scripts and BuildEditor.bat)
- Prototype/ (the browser toy)
- Art/: SourceArt/Heroine (Tripo model), SourceArt/Fabric and SourceArt/Wood (PBRgen), no audio
- docs/: DESIGN.md, DevNotes.md, CLAUDE.md, screenshots and cast.png, and docs/process/ with all the Prompt_*.md files in order (T3 to T9b; leave out Prompt_T9.md, it is a broken duplicate) plus a short index explaining the workflow

Leave out: SourceArt/Audio (licence: not transferable), Content, Saved, Binaries, Intermediate, DerivedDataCache, .vs, _Archive, FeedbackImages, ItchPage.md, AGENTS.md, .diversion, .dvignore, .claude, .mcp.json, Build, stackdumps, logs, any stray or temp files. No file over 100 MB. Check every file for personal info (email addresses, local paths with my name) and remove it.

One clean initial commit with a clear message.

## README
- What the game is (2-3 lines), screenshots
- A placeholder for the YouTube video
- The download (the GitHub Release)
- Explaining the modjam which leads into the itch page
- The itch page (placeholder link)
- How it was made: 4 days, Claude in chat for design and prompts, Claude Code for all code, docs/process as the record
- A code map (which file does what)
- Briefly Why the C++ module is called CardIsland (the game pivoted on day 2)
- Credits and tools, including "Sound and music: WeLoveIndies"
- "Content and audio are not included in the repo for licence reasons; the playable build is in Releases."
- No open-source licence (all rights reserved)

## Then
Repo name: KillingPart, public. After I approve: push, then gh release create v1.1 with KillingPart_Windows_v1.1.zip attached. Release notes: what's in it and the ~40 s first-launch note.
