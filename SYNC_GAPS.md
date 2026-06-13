# Sync Gaps — Full Feature List

Everything that needs to work for this to feel like playing Skyrim together rather than two people playing nearby.

---

## Movement & World

- **Fast travel** — not synced; one player can fast travel, leaving the other behind
- **Horse mounting/dismounting** — several silent failure paths, can desync
- **Swimming state** — not synced
- **Jumping/falling animation** — not always reflected on remote player
- **Doors** — open/closed state desyncs when activated at the wrong moment
- **Cell transitions** — brief period where remote player disappears and reappears wrong

---

## NPCs

- **NPC ownership gap** — when the owning player leaves a cell, NPCs freeze or stutter until another client claims them
- **Dialogue gating** — only the player whose client owns the NPC can advance dialogue or quests through them; the other player is a spectator
- **Random NPC encounters** — courier, beggars, strangers walking up only appear for one player
- **Follower NPCs** — unclear if both players see each other's followers correctly at all times
- **NPC combat targeting** — NPCs may focus one player entirely and ignore the other
- **NPC AI packages** — partially implemented; NPCs can behave differently on each screen
- **Dragon sync** — dragon landing and attack coordination; dragon check hardcoded to false in parts of character sync

---

## Quests

- **Join-state catchup** — connecting mid-session does not sync the leader's current quest progress to the joining player
- **Misc/radiant quests blocked** — server drops None and Miscellaneous type quests by default
- **Radiant alias fill** — random quest targets (which dungeon, which NPC) are picked independently per client; both players get the same quest but are sent to different locations
- **Quest triggered by NPC encounter** — if an NPC starts a quest on one player's screen, the other player's quest journal is not updated
- **Quest completion events** — only the player who does the final step may get the completion; the other player may stay stuck at the last stage
- **Werewolf and vampire transformation quests** — explicitly excluded from sync
- **Skill experience quest** — explicitly excluded from sync

---

## Combat

- **Projectile origin** — arrows and spells recalculate origin locally; they land in different places for each player
- **Weapon drawn/sheathed state** — not synced; remote player looks unarmed during combat
- **Kill moves** — one player sees the finisher, the other sees the enemy still alive for a moment
- **Stagger** — sync unclear; remote player may not visibly react to hits
- **Sneak attack** — damage multiplier may not apply correctly when hitting an NPC the other player owns
- **Blocking and parry** — not reflected on the remote player's model

---

## Health & Stats

- **Remote player health bar** — display disabled due to an underlying crash bug in health sync
- **Health sync crash** — ActorValueService occasionally crashes with no clear cause during health updates
- **Stamina and magicka** — not shown on the other player's UI
- **Dragon souls** — intentionally not synced; only the player present gets the soul
- **Skill level-up** — not replicated to the other player; each player levels independently
- **Perks** — entirely per-player; each player has their own separate skill tree

---

## Magic

- **Conjured creatures** — ownership is local; unclear if the summoned creature is visible to both players
- **Healing spells** — Restoration cast at the other player probably does not heal them
- **Illusion spells** — only applies on the casting player's client; NPCs react differently per screen
- **Shout cooldown** — not shown for the remote player
- **Enchantment conditions** — not implemented; enchanted weapons behave differently per client

---

## Inventory & Loot

- **Dropped item pickup** — fails for world items with dynamic form IDs (items dropped by players or NPCs)
- **Container looting** — both players can loot the same chest independently, duplicating items
- **Crafting** — smithing, alchemy, and enchanting are entirely local; results not visible to the other player
- **Mining and woodchopping** — not synced
- **Gold and shops** — each player has their own economy; shop inventories are independent

---

## Equipment & Appearance

- **Weapon draw state** — (also listed under combat) not synced
- **NPC armour disappearing** — periodic 1-second workaround re-dresses naked NPCs locally; not broadcast to other player
- **Beast form appearance** — werewolf and vampire lord transformations trigger a full respawn message instead of an appearance change; can look wrong or lag
- **Equipment changes** — equipped item changes do not always reflect correctly on the remote character model

---

## World State

- **Dungeon cleared state** — if one player clears a dungeon, respawn timer and remaining loot may differ for the other
- **Container lock state** — not synced; a chest one player unlocked may appear locked to the other
- **Levers and switches** — sometimes desynced
- **Weather** — sync works but has timing and ownership edge cases

---

## UI & Feedback

- **Other player health bar** — not shown (see Health & Stats)
- **Other player stamina/magicka** — not shown
- **Quest markers** — can point to different locations per player on radiant quests
- **Map markers** — unclear whether discovered locations are shared between players
- **Crime and bounty** — if one player commits a crime, the other player's bounty is unaffected

---

## Transformations

- **Werewolf transformation** — appearance desynced across clients
- **Vampire Lord transformation** — appearance desynced across clients
- **Vampire feeding** — not synced

---

## DLC

- **Dawnguard** — intro triggered by courier (random encounter, misc quest); both are currently broken paths
- **Hearthfire** — house building is entirely local
- **Dragonborn** — Solstheim content likely has the same NPC and quest issues as the base game; untested

---

## Already Fixed (This Fork)

- Remote actor animation variables snapping to future state instead of current — fixed
- Direction jumping between snapshots instead of lerping — fixed
- Actors sliding when the interpolation buffer runs dry instead of stopping — fixed
- Duplicate movement packets causing jitter — fixed

---

## Notes on What Is and Is Not Fixable

Some gaps here are bugs in the mod that can be fixed with reasonable effort. Others are architectural limits of building multiplayer on top of a single-player engine.

**Reasonable to fix:** join-state quest catchup, misc quest sync, projectile origin, dropped item pickup, health bar crash, door desync, weapon draw state, beast form appearance.

**Hard but possible:** container loot deduplication, NPC ownership handoff, cell transition smoothing, conjuration visibility.

**Requires engine-level work:** dialogue gating (only one player can talk to an NPC), radiant quest alias sync, random encounter spawning for both players simultaneously, per-player vs shared economy decisions.
