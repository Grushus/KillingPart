# T6c prompt (from Simon): modes, the Play flow, the real boss

Simon played T6b. Decisions below are his (Sun 12:53). This is the LAST feature task: after it, only audio/juice (T7) and shipping (T8). Hard stop: run `date` at each commit; if it's past 15:00, finish the current chunk and stop. Do chunks 1-4 first; chunk 5 (Extended cut) only after them. Same routine: build, quick test, dv commit (include this file), CLAUDE.md Status, DevNotes lines, screenshots in Saved/Screenshots/KP. At the end: update DESIGN.md (modes, boss, flow; if chunk 5 didn't get built, put its design under a new "After the jam" section), package a Windows build and run the exe once.

## 1. Texts (quick)
- Monday's card-pick tutorial pill: "Clear a wave to get a card. Cards reset every night." (replaces "Pick a card. Green helps, red is the catch").
- Under the cards: "Cards reset every night" (replaces "Cards last until dawn").
- In the mode where the card is given, not chosen: no heading, no "Your card: take it". Just the card, click it to take it.

## 2. Modes: a ladder, no multipliers
Multipliers go (Simon: they have no point; each mode keeps its own week best instead). Six week modes, each unlocked by finishing the one before it, each with its own save slot and its own week best. All except Normal use Hardcore rules (hearts carry over between nights, losing sends you back to Monday). Mode names and texts live in the JSON so Simon can rename them.

| Mode | Card text (plain words for non-native players) | Unlock |
| --- | --- | --- |
| Normal | Survive 7 nights. Pick a card after each wave. Cards and hearts reset every night. Lose: try the night again. | from the start |
| Hardcore | Like Normal, but hearts don't reset between nights. Lose: back to Monday. | finish Normal |
| Brutal | Like Hardcore, but you can't choose: one random card after each wave. | finish Hardcore |
| Extreme | Like Hardcore, but only one random card, at the start of each night. | finish Brutal |
| Insane | Like Hardcore, with no cards at all. | finish Extreme |
| Final cut | No cards and only 1 heart. Lose: back to Monday. | finish Insane |

(Today's "Insane" with one forced card becomes Brutal; today's "Ultra" with no cards becomes Insane and gets Hardcore rules.) Locked cards say why: "Finish Hardcore to unlock".

Layout of the modes screen: two rows of three for the ladder (Normal, Hardcore, Brutal / Extreme, Insane, Final cut), then a third row with the two endless modes as wider cards (Extended cut, Extended nightmare, chunk 5), since they're a different kind of mode.

### 2b. Darkness after Thursday (all week modes)
Lights out shouldn't be a one-night thing, but it shouldn't be luck either. After Thursday, exactly 3 waves across Friday, Saturday and Sunday are dark, never more, never fewer, never the boss wave, and at most 2 in one night. They're picked at random when a week starts and saved with that week's save, so retrying a night gives the same dark waves. A dark wave announces itself ("Lights out!") as it starts and uses Thursday's darkness exactly (lamp circle, glinting eyes, chalk light). The count (3) and the per-night max (2) are in the JSON.

## 3. The Play flow
- Title menu: Play, Cut: Director's (blood) / Theatrical (no blood), Quit. ("New week" and "Game modes" merge into Play.)
- Play opens the modes screen. Under each unlocked mode card, two buttons: "Continue: <night>" (only if that mode has a save) and "New game". Back is an arrow in the top-left corner.
- Every night starts with a 3-2-1 countdown over the mat before plushies move (after loading a save, after the results screen, after a retry), so nobody gets thrown straight in.
- Remove night select. After the week (the ending screen), one button: "Back to home" (the title screen).
- Week score in a save = the sum of its nights' scores (in Normal the best of each night's attempts). The mode's record = its best finished week.

## 4. The boss: the hardest fight of the week
Simon could still win by just focusing it (long chalk crosses the mat before the escort arrives). New rules, all numbers in the JSON:
- 50 hits instead of 8. Under its name: "The first one" and a heart icon with "x50" counting down; every line through it is one hit.
- A chunk comes off every 10 hits (5 chunks, each gets up as a small teddy as now); at 0 the final split and the big finish.
- Every hit also spawns one random plushie or pincushion from the edges (any type except golden, Sunday's mix).
- The whole fight, every 3 s, 7 chicks come in from random points just outside the mat. Chicks give no score, so the fight can't be farmed; only hits on the boss and plushies cut along the way score. Spawning never stops until the boss is dead: the player can never be left alone with it.
- Enrage at 20 hits left (as now, +50% speed and the tell).
- Cap the number of plushies alive at once (about 45) for performance; spawns wait while at the cap.
- Fix: knockback must keep the boss on the mat (it could end up under the night name at the top).
- Check with the bot: the long-chalk boss rush must no longer win. Tell me how long a fight takes and how many hearts it costs.

## 5. Extended cut and Extended nightmare (endless modes), last
Simon's design. Unlocked by finishing Normal. Its own save. Records: best score and highest round, shown on its mode card.
- Round based, no nights and no night rules, hearts carry over (Thimble can add, max 5), losing ends the run.
- A round counter in a corner of the screen during play ("Round 12").
- Truly endless: no hand-written list of rounds. Every round is built from formulas whose numbers live in the JSON ("endless"), for example:
  - plushies(r) = 3 + (r - 1): one more every round (spawns wait at the alive cap, so huge rounds stay playable),
  - speed(r) = base + step x r, capped at a max speed; spawn interval shrinks toward a minimum,
  - a new plushie type joins the mix every 5th round, in order: pincushions (5), bunny (10), patchwork (15), big bear (20), mother hen (25); after that, every 5th round raises the share of the harder types,
  - pincushions per round grow slowly with r, up to a cap,
  - bosses: from round 30 every 10th round is a boss round; bosses(r) = 1 + floor((r - 30) / 20) at once; hits per boss start at about 15 and grow by 5 each boss round; same spawning rules as Sunday's boss,
  - a card every 10th round (pick 1 of 3), kept for the whole run, until every card is at level III (Thimble only while hearts are below 5); when nothing is left to offer, skip the pick.
- No darkness in Extended cut (it's the plain endless mode, no luck involved); darkness is what Extended nightmare is for.
- The results screen after a run: round reached, score, and the two records (with "New record!").
- Extended nightmare: the same endless rounds, but every round is dark (bosses too). Its own save and records. Unlock: reach round 50 in Extended cut (in the JSON).

When done, tell me what to look at.
