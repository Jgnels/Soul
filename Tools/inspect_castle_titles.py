import sqlite3
db=r'D:\Unreal Projects\AoEAssetRenderLab\Content\VaultCache\FabLibrary\listings_v1.db'
con=sqlite3.connect(db); c=con.cursor()
uids=[
'09a1119d-feb6-4cf1-b7e7-a7651e9ac149',
'454baa93-0c78-4a53-a57e-47943160d591',
'b91e0942-7668-44cc-8b3a-6b977bd831ee',
'2b14fc54-691f-4a37-8045-bef78d8b1ebc',
'fd908d45-b194-4a7d-97c9-942c0cd22095',
'3bfee149-0741-4892-ba2c-b7a245e45de7',
'826f1b55-01e2-4272-993d-ea142a264f9f',
'd8d7b289-dfed-4236-97ab-941c2c6c3790'
]
for uid in uids:
    r=c.execute("select uid,title,user_seller_name,media from local_listing where uid=?",(uid,)).fetchone()
    print(r[0] if r else uid,'|',r[1] if r else 'MISSING','| seller:',r[2] if r else '')
con.close()
