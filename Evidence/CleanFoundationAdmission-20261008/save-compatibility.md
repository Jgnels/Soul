# Save compatibility

No RBSave schema or domain authority changed. No migration is attempted.

| Fixture | Isolated slot |
|---|---|
| Retained/default | `Soul.VerticalCampaign` |
| Composition founder | `Soul.Composition3500.Founder` |
| Human authored proof | `Soul.Composition3500.HumanProof` |
| Exact Orc matchup proof | `Soul.Composition3500.OrcProof` |

Existing schema/version, region-set, ownership and unit identity validators remain. Native tests reject unsupported faction/unit pairs and cross-fixture restore shapes. A nine-region save is not silently expanded into 36 regions.

The ordinary Orc battle naturally lost with 0 allied / 7 hostile survivors and returned to River Ford. The separate built-in spell observer naturally won with 20 allied / 0 hostile survivors and returned to Orc Watch. Both used the existing automatic post-result RBSave path and controller F9 with exact state comparison, then restored their own saved bytes through controller F9 in a separate process. Controller F5 was separately qualified in the clean Composition input run. They use separate run-local user directories even though both intentionally exercise the OrcProof namespace.

Arbitrary camera pose persistence is not claimed: restoration selects/reframes the restored player region through the existing presentation adapter. The manual launcher keeps Founder, HumanProof and OrcProof user directories separate and performs no automatic inputs.
