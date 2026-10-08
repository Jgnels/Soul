"""Admit loose fixture-only changes without pretending unchanged assets were recooked."""
from pathlib import PurePosixPath

def verify_compatible_cook_profile(prior,current):
    loose_key='exact_additional_runtime_files';slots_key='save_slots'
    immutable=lambda p:{k:v for k,v in p.items() if k not in {loose_key,slots_key}}
    if immutable(prior)!=immutable(current):raise ValueError('Cook roots, launch/default policy or other asset-cook inputs changed')
    before=prior[loose_key];after=current[loose_key]
    if len(after)!=len(set(after)) or not set(before)<=set(after):raise ValueError('Existing loose dependencies changed or duplicates added')
    added=sorted(set(after)-set(before))
    for name in added:
        p=PurePosixPath(name)
        if p.parent!=PurePosixPath('Data/CampaignComposition') or p.suffix!='.json' or '..' in p.parts:
            raise ValueError('Only isolated Composition JSON fixtures may reuse this cook')
    old_slots=prior[slots_key];new_slots=current[slots_key]
    if any(new_slots.get(k)!=v for k,v in old_slots.items()):raise ValueError('Existing save namespace changed')
    added_slots={k:v for k,v in new_slots.items() if k not in old_slots}
    if len(set(new_slots.values()))!=len(new_slots) or any(not v.startswith('Soul.Composition3500.') for v in added_slots.values()):
        raise ValueError('Qualification save slots must remain separate Composition namespaces')
    return {'asset_cook_inputs_unchanged':True,'added_loose_fixtures':added,'added_isolated_save_slots':added_slots,'fresh_asset_cook':False}
