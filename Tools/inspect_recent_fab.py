import sqlite3, json, os, re
db=r'D:\Unreal Projects\AoEAssetRenderLab\Content\VaultCache\FabLibrary\listings_v1.db'
con=sqlite3.connect(db)
cur=con.cursor()
terms=['hivemind','kingdom','alien','castle','viking','harbour','harbor','gothic','ruin','forge','treefort','tree fort','nature','landscape','raven','modular']
print('MATCHING LOCAL LISTINGS')
rows=cur.execute("select uid,title,thumbnail from local_listing").fetchall()
for uid,title,thumb in rows:
    t=(title or '').lower()
    if any(x in t for x in terms):
        print(uid,'|',title)
print('\nRECENT DOWNLOAD META')
q="""
select l.uid,l.title,d.path,d.cache_size
from download_meta d left join local_listing l on l.uid=d.listing_uid
order by d.id desc limit 80
"""
for row in cur.execute(q):
    print(' | '.join('' if v is None else str(v) for v in row))
con.close()
