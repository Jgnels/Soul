# Soul Balance Lab

Headless deterministic Python stress-test laboratory for Soul campaign mechanics.

This is **not production runtime code**. It must never be imported by Source/, Plugins/, or Content/.

## Authority boundary

The lab has three deliberately separate layers:

1. **LIVE MIRROR** — integer rules observed in current Source/SoulCore: day/action budget shape, logistics, hero XP curve, regiment veterancy, commander-memory decay/bounds, strategic candidate scoring, recruitment-building operational checks, and siege supply decay.
2. **DONOR REFERENCE** — Living Strategy revival/week-one-20260918 informs campaign-level candidate/reasoning, bounded memory influence, explicit opportunity cost, and explainable deterministic choice.
3. **LAB GLUE** — symmetric generated maps, generic seven-line costs/strength, aggregate battle resolution, defeated-army recovery, and some integration assumptions. These exist only to expose sensitivity and failure modes.

No proprietary Heroes or Total War formula is used.

## Run

From the Soul repository root:

    python Tools\\BalanceLab\\balance_lab.py --runs 1000 --seed-base 20260920
    python -m unittest Tools.BalanceLab.tests.test_balance_lab -v

The default run creates deterministic evidence under Evidence/BalanceLab/.
