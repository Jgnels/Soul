import sqlite3
db=r'D:\Unreal Projects\AoEAssetRenderLab\Content\VaultCache\FabLibrary\listings_v1.db'
con=sqlite3.connect(db); c=con.cursor()
terms=['tree','fort','forest','nature','gothic','ruin','harbour','harbor','water city','forge','alien castle','kingdom capital','modular castle']
for uid,title in c.execute("select uid,title from local_listing"):
    t=(title or '').lower()
    if any(x in t for x in terms):
        print(uid,'|',title)
con.close()
