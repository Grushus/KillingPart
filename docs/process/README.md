# The process

Killing Part was made in four days by one person and two Claudes, with a simple split:

1. **Design in chat.** Simon talked the game through with Claude in a chat window: the pitch, the rules, what to cut, what a playtest showed. Each round ended with a written prompt for the next build task.
2. **Build with Claude Code.** That prompt went to Claude Code, working in the Unreal project. It wrote all the C++, built the editor, tested the game (in Play over the Unreal MCP, or with a bot that plays by itself), took screenshots, wrote dated lines in [DevNotes.md](../DevNotes.md), updated [CLAUDE.md](../CLAUDE.md) and committed to Diversion.
3. **Play, then decide.** Simon played the build and made the design calls, and the next prompt was written from what he found.

[DESIGN.md](../DESIGN.md) was the source of truth for the rules and the build order (tasks T1 to T8). [CLAUDE.md](../CLAUDE.md) is the standing brief Claude Code read at the start of every session: the engine, the code conventions that broke the build once, gotchas and a status log.

The files below are the prompts, unedited, in the order they were given. T1 (the project setup) and T2 (the core loop) were given in chat and have no file. A letter after the number means a follow-up to the same task, usually fixes from a playtest.

| Prompt | Day | What it asked for |
| --- | --- | --- |
| [T3](Prompt_T3.md) | Fri 13 Nov | Real slicing: procedural plushie bodies cut in half, gore, the Theatrical cut |
| [T3b](Prompt_T3b.md) | Sat 14 Nov | Make the halves read as cut plushies |
| [T3c](Prompt_T3c.md) | Sat | Slice where the line actually passed |
| [T4](Prompt_T4.md) | Sat | Content: plushie types, pincushions, seven nights, cards |
| [T4b](Prompt_T4b.md) | Sat | Fixes from the T4 playtest |
| [T4c](Prompt_T4c.md) | Sat | Card language, card levels, small fixes |
| [T4d](Prompt_T4d.md) | Sat | Small polish before T5 |
| [T5](Prompt_T5.md) | Sat | The day between nights ("broader, not stronger") |
| [T5b](Prompt_T5b.md) | Sat | Strip the day down to one screen |
| [T6](Prompt_T6.md) | Sun 15 Nov | Title, pause, tutorial, the boss, the ending |
| [T6b](Prompt_T6b.md) | Sun | Fixes from the T6 playtest |
| [T6c](Prompt_T6c.md) | Sun | Modes, the Play flow, the real boss |
| [T6d](Prompt_T6d.md) | Sun | Modes screen, clear rules, saving, stats |
| [T7](Prompt_T7.md) | Sun | Audio |
| [T7b](Prompt_T7b.md) | Sun | Sound picks, test commands, golden pincushion, juice |
| [T7c](Prompt_T7c.md) | Sun | Last tweaks before shipping |
| [T7d](Prompt_T7d.md) | Sun | Her look, camera cuts, variants, the impostor |
| [T7e](Prompt_T7e.md) | Sun | Fix her look, effects HUD, small fixes |
| [T7f](Prompt_T7f.md) | Sun | The real heroine (a Tripo AI model) |
| [T8](Prompt_T8.md) | Sun | Ship it: Practice, Shipping build, itch art |
| [T9b](Prompt_T9b.md) | Mon 16 Nov | This repo and the GitHub release |
