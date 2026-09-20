"""Non-UE Soul roster asset audit. Reads catalog/cache metadata only."""
from __future__ import annotations
import json
import re
import sqlite3
from pathlib import Path

CATALOG = Path(r"C:\Users\Jeff\Documents\GitHub\Copperlight-Asset-Catalog\catalog\products.json")
CACHE_ROOTS = [
    Path(r"D:\AssetCaches\EpicVaultCache\FabLibrary"),
    Path(r"D:\Caches\DesktopVaultCache\FabLibrary"),
]
OUT = Path(r"D:\RefinedBadger\Worktrees\Soul-roster-casting-20260920\Evidence\soul_roster_asset_inventory_20260920.json")
KEY_TITLES = {
    "Knights (Pack)", "Dwarfs Pack", "14 Orcs Pack", "Viking", "Viking (Customized)",
    "Norse Shield maiden Low-poly 3D model", "Quadruped Fantasy Creatures",
    "ANIMAL VARIETY PACK", "Fantasy Animal (Pack)", "Fantasy Characters (Pack)",
    "Fantasy Enemies (Pack)", "Fantasy Warriors (Pack)", "10 Creatures (Pack)",
    "Humanoids Creatures Pack", "Humanoids Monsters Pack", "NPC King And Queen",
    "Primitive Characters (Pack)", "92 Animations For Warrior", "Spear And Shield Animations",
}
PARAGON_PREFIX = "Paragon:"

def load_catalog() -> list[dict]:
    return json.loads(CATALOG.read_text(encoding="utf-8"))

def relevant_catalog_rows(products: list[dict]) -> list[dict]:
    rows = []
    for p in products:
        name = p.get("ProductName") or ""
        if p.get("Owned") is True and (name in KEY_TITLES or name.startswith(PARAGON_PREFIX)):
            rows.append({
                "title": name,
                "publisher": p.get("Publisher"),
                "source_product_id": p.get("SourceProductId"),
                "owned": p.get("Owned"),
                "owned_confidence": p.get("OwnedConfidence"),
                "catalog_locally_available": p.get("LocallyAvailable"),
                "local_path": p.get("LocalPath"),
                "local_paths": p.get("LocalPaths") or [],
                "evidence_states": p.get("EvidenceStates") or [],
            })
    return sorted(rows, key=lambda x: x["title"].casefold())

def read_db(db_path: Path) -> dict[str, dict]:
    con = sqlite3.connect(f"file:{db_path}?mode=ro", uri=True)
    rows = con.execute("select uid,title from local_listing order by title").fetchall()
    return {title: {"listing_uuid": uid, "db": str(db_path)} for uid, title in rows}

def manifest_summary(product_dir: Path) -> dict:
    manifest = product_dir / "unreal-engine" / "manifest"
    result = {"product_dir": str(product_dir), "manifest": str(manifest), "parse": "missing", "selected_files": []}
    if not manifest.exists():
        return result
    try:
        data = json.loads(manifest.read_text(encoding="utf-8"))
    except (UnicodeDecodeError, json.JSONDecodeError):
        result["parse"] = "binary_or_non_json"
        return result
    files = [entry.get("Filename", "") for entry in data.get("FileManifestList", [])]
    rx = re.compile(r"(SKM?_.*\.uasset$|Skeleton\.uasset$|/Mesh(?:es)?/.*\.uasset$|/Weapons?/.*\.uasset$|wolf|griff|dragon)", re.I)
    reject = re.compile(r"(texture|material|physicsasset|_mat\.|/materials?/)", re.I)
    selected = [f for f in files if rx.search(f) and not reject.search(f)]
    result.update({
        "parse": "json",
        "fab_action_url": data.get("CustomFields", {}).get("Vault.ActionURL"),
        "file_count": len(files),
        "selected_files": selected,
    })
    return result

def cached_products() -> tuple[dict[str, dict], list[dict]]:
    by_title: dict[str, dict] = {}
    manifests: list[dict] = []
    for root in CACHE_ROOTS:
        db = root / "listings_v1.db"
        if db.exists():
            by_title.update(read_db(db))
        if not root.exists():
            continue

        for product_dir in root.iterdir():
            if not product_dir.is_dir():
                continue
            suffix = product_dir.name.rsplit("-", 1)[-1].casefold()
            title = next((t for t, info in by_title.items()
                          if (info.get("listing_uuid") or "").casefold().startswith(suffix)), None)
            if title and (title in KEY_TITLES or title.startswith(PARAGON_PREFIX)):
                item = manifest_summary(product_dir)
                item["title"] = title
                item["listing_uuid"] = by_title[title]["listing_uuid"]
                manifests.append(item)
    dedup = {}
    for item in manifests:
        key = (item["title"], item["product_dir"])
        dedup[key] = item
    return by_title, sorted(dedup.values(), key=lambda x: (x["title"].casefold(), x["product_dir"]))

def main() -> None:
    products = load_catalog()
    catalog_rows = relevant_catalog_rows(products)
    db_rows, manifests = cached_products()
    for row in catalog_rows:
        live = db_rows.get(row["title"])
        row["live_cache_listing"] = live
    paragon_local = sorted(
        [{"title": t, **info} for t, info in db_rows.items() if t.startswith(PARAGON_PREFIX)],
        key=lambda x: x["title"].casefold(),
    )
    payload = {
        "generated_by": "Tools/audit_soul_roster_assets.py",
        "mode": "NON_UE_METADATA_ONLY",
        "catalog_path": str(CATALOG),
        "catalog_relevant_owned": catalog_rows,
        "live_manifest_summaries": manifests,
        "live_paragon_listings": paragon_local,
    }

    OUT.parent.mkdir(parents=True, exist_ok=True)
    OUT.write_text(json.dumps(payload, indent=2, ensure_ascii=False) + "\n", encoding="utf-8")
    print(f"Wrote {OUT}")
    print(f"Relevant owned catalog rows: {len(catalog_rows)}")
    print(f"Live manifest summaries: {len(manifests)}")
    print("Live Paragons:", ", ".join(x["title"] for x in paragon_local) or "none")

if __name__ == "__main__":
    main()
