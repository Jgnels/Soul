import sqlite3, html, re, json
db=r'D:\Unreal Projects\AoEAssetRenderLab\Content\VaultCache\FabLibrary\listings_v1.db'
uids={
'humans_hivemind':'826f1b55-01e2-4272-993d-ea142a264f9f',
'kingdom_capital':'09a1119d-feb6-4cf1-b7e7-a7651e9ac149',
'dark_alien':'454baa93-0c78-4a53-a57e-47943160d591',
'dwarves_forge':'b91e0942-7668-44cc-8b3a-6b977bd831ee',
'orcs_ruins':'2b14fc54-691f-4a37-8045-bef78d8b1ebc',
'vikings_water':'fd908d45-b194-4a7d-97c9-942c0cd22095',
'nature_forest':'d8d7b289-dfed-4236-97ab-941c2c6c3790',
'ravenhold':'3bfee149-0741-4892-ba2c-b7a245e45de7',
'landscapes':'c6a8fa58-84f1-4bb8-935d-a3467e9fe58d',
'coastal_ruins':'558372d8-3643-4d58-855d-738fbfdfcc96',
'desert':'b65e58ce-2e74-4c76-9fb0-8d64f6b81488',
'ancient_mountain':'05571320-3950-409f-a77f-e3ee4b36b5b5',
'castle_town':'42d4a792-2b66-423d-9b20-84d6b2c578d8',
'viking_village':'a72879ea-577a-4125-bde8-25fdd51060cf',
}
con=sqlite3.connect(db); c=con.cursor()
def clean(s):
    if not s:return ''
    s=re.sub(r'<br\s*/?>','\n',s,flags=re.I)
    s=re.sub(r'</p>|</h\d>|</li>','\n',s,flags=re.I)
    s=re.sub(r'<[^>]+>',' ',s)
    s=html.unescape(s)
    s=re.sub(r'[ \t]+',' ',s)
    s=re.sub(r'\n\s+','\n',s)
    return s.strip()
for key,uid in uids.items():
    row=c.execute("select title,description,average_rating,review_count,user_seller_name,category_path from local_listing where uid=?",(uid,)).fetchone()
    if not row:
        print('MISSING',key,uid);continue
    print('\n###',key,'|',row[0])
    print('seller=',row[4],'rating=',row[2],'reviews=',row[3],'category=',row[5])
    desc=clean(row[1])
    print(desc[:5000])
con.close()
