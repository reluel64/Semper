#ifdef WIN32
"[Surface-Meta]\n"
"Name=Surface registry\n"
"Author=Margarit Alexandru-Daniel\n"

"[Surface]\n"
"SurfaceColor=0x0\n"
"AutoSize=1\n"
"Updatefrequency=-1\n"

"[Arrow]\n"
"Object=String\n"
"Text=\u25b2\n"
"W=20\n"
"H=20\n"
"Y=-4\n"
"X=$surfaceW$/2-10\n"
"Volatile=1\n"
"FontColor=0;0;0;128\n"

"[Background]\n"
"Volatile=1\n"
"Object=Image\n"
"BackColor=0;0;0;128\n"
"W=max([Title:X]+[Title:W],[Text:X]+[Text:W])+5\n"
"H=max([Title:Y]+[Title:H],[Text:Y]+[Text:H])-5\n"
"Y=-4R\n"
"UpdateAction=Update(Arrow)\n"

"[Title]\n"
"Volatile=1\n"
"Object=String\n"
"Text=$Title$\n"
"Y=5r\n"
"FontShadow=1\n"
"X=5\n"
"UpdateAction=Update(Background);ForceDraw()\n"
"[Text]\n"
"Volatile=1\n"
"Object=String\n"
"Text=$Text$\n"
"FontShadow=1\n"
"Y=5R\n"
"X=5\n"
"FontSize=10\n"
"FontName=Segoe UI\n"
"UpdateAction=Update(Background);ForceDraw()\n"
#elif __linux__
"[Surface-Meta]\n"
"Name=Surface registry\n"
"Author=Margarit Alexandru-Daniel\n"

"[Surface]\n"
"SurfaceColor=0x0\n"
"AutoSize=1\n"
"Updatefrequency=16\n"

"[Arrow]\n"
"Object=String\n"
"Text=\u25b2\n"
"W=20\n"
"H=20\n"
"Y=-4\n"
"X=$SurfaceW$/2-10\n"
"Volatile=1\n"
"FontColor=0;0;0;128\n"
"FontName=DejaVu Sans\n"

"[Background]\n"
"Volatile=1\n"
"Object=Image\n"
"BackColor=0;0;0;128\n"
"W=max([Title:X]+[Title:W],[Text:X]+[Text:W])+5\n"
"H=max([Title:Y]+[Title:H],[Text:Y]+[Text:H])-5\n"
"Y=-1R\n"
"UpdateAction=Update(Arrow)\n"

"[Title]\n"
"Volatile=1\n"
"Object=String\n"
"Text=$Title$\n"
"Y=5r\n"
"FontShadow=1\n"
"X=5\n"
"UpdateAction=Update(Background);ForceDraw()\n"
"FontName=DejaVu Sans\n"

"[Text]\n"
"Volatile=1\n"
"Object=String\n"
"Text=$Text$\n"
"FontShadow=1\n"
"Y=5R\n"
"X=5\n"
"FontSize=10\n"
"FontName=DejaVu Sans\n"
"UpdateAction=Update(Background);ForceDraw()\n"
#endif
#if 0
[Surface-Meta]
Name=Relu's launcher
Author=Margarit Alexandru-Daniel

[Surface]
UpdateFrequency=1000


;----------------Sources---------------
[Surface-Variables]
;-------Right ttip------
;P1X=20
;P1Y=100
;P2X=0
;P2Y=110
;P3X=20
;P3Y=120
;----------Bottom------
P1X=100
P1Y=20
P2X=110
P2Y=0
P3X=120
P3Y=20


[Frame]
Object=Vector
Path=Rect(0,0,200,200);Stroke(0;255;0;255)
Path1=PathSet($P1X$,$P1Y$,0,Arrow);Stroke(0;255;0;255)
Arrow=LineTo($P2X$,$P2Y$);LineTo($P3X$,$P3Y$);
Path2=Join(Path);Union(Path1);Fill(0;0;0;200)
#endif
