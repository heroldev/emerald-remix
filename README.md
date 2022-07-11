# Pokémon Emerald - [@heroldev](https://github.com/heroldev)'s remix

This is an edited version of Pokémon Emerald, based on [pret's decompilation](https://github.com/pret/pokeemerald).

The idea is that this ROM includes quality-of-life fixes to make Emerald the best it can be.

### Current implemented fixes:
- Allow save file RTC reset via Left + Select + B on title screen 
- Fixed VBlank implementation
- Fixed Battle Factory bug that occured while determing IVs for opponent parties (`battle_tower.c`, line 1832)
- Fixed Battle Dome IV bug where all opponent Pokémon had IVs of 3
- Fixed Battle Dome bug where results were being unfairly determined due to variables not being re-initialized
- Run indoors
- Modern Premier Balls (increments by 10 on all ball types)
- Used Repel prompt
- Removed "Overwrite already saved File?" prompt when saving
- Fixed transition pause when entering a Pokémon Center

### New Features
- Change battle music in the Frontier to `RG_VS_TRAINER` (FireRed/LeafGreen Trainer battle theme)

### WIP
- Multiboot rom to reset the RTC on Ruby/Sapphire/Emerald
- Hash out Champion's Cup Dome-like Frontier Facility

### Building/Setup
To set up the repository, see [INSTALL.md](INSTALL.md).
