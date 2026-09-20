# RB Combat fit decision

RB Combat was inspected before Soul tactical simulation was admitted.

What RB Combat is strong at:
- physical melee contact windows and weapon geometry;
- bows/projectiles;
- guard/block;
- individual combat authority adapters;
- embodied group orders, navigation and representation control.

What Soul requires canonically:
- deterministic hidden-hex legality;
- stack/group counts and casualties;
- initiative/wait/retaliation;
- multi-hex large creatures;
- seeded morale/luck;
- tactical terrain modifiers;
- spells and siege objectives.

Conclusion:
RB Combat remains installed through the RB Foundation legacy core stack and is available for physical presentation/adapters, but Soul does not force real-time sweep/contact logic into its canonical turn-based stack resolution.

Soul registers the separate domain Soul.TacticalSimulation rather than competing for Foundation's generic Combat authority. A future adapter may let RB Combat present selected hero/melee actions, but presentation must not independently alter canonical stack outcomes.
