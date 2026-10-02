# Full-sequence gameplay reference findings

Reviewed 2026-10-01 through complete transcripts plus dense storyboards, then denser inspection around commands, camera changes, encounters, and battle phases.

## Sources
- Total War: Warhammer III, Karl Franz opening walkthrough (81 minutes): https://www.youtube.com/watch?v=exMyDXLEsN4
- Bannerlord campaign opening: https://www.youtube.com/watch?v=CMxEENNl6kw
- Bannerlord battlefield command guide: https://www.youtube.com/watch?v=QTxrRzv0ncw

## Findings that apply to Soul
- Campaign armies must read as primary selectable world actors. Army strength, selection, remaining movement, and legal destinations stay visible after selection.
- Encounters need a decision/setup beat before simulation advances.
- Battles start paused in deployment, with persistent pause state and a clear start control.
- The player retains an embodied hero camera while issuing group commands; a separate commander view is optional and reversible.
- Formation cards stay visible, show role/count/order/morale, and make the selected formation unmistakable in both HUD and world.
- Hold, advance, charge, fallback, and face must produce visibly different spatial behavior.
- Unit roles are read at formation scale. The player should not need to identify dozens of small individuals.
- Spell controls and available mana remain visible; casting cannot depend on remembered hidden keys.
- Typical early reference battles develop through approach, ranged pressure, commitment, flank, collapse, and rout over several minutes.
- Campaign management panels appear contextually and leave the world readable rather than covering it with a debug transcript.

## Immediate Soul acceptance checks
1. Campaign selection and movement are understandable without prior instruction.
2. Battle opens paused and remains inert until the player starts it.
3. First-person, third-person, and commander cameras are discoverable and reversible.
4. Formation selection, current orders, spells, mana, morale, and reserves remain visible.
5. Elephant body renders; Kraken and Griffin face their actual movement.
6. A normal battle lasts long enough to issue and observe several orders.
7. The result returns to campaign and visibly updates strategic state.
