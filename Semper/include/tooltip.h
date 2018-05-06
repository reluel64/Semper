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
Name=Relu\'s launcher
Author=Margarit Alexandru-Daniel

[Surface]
UpdateFrequency=1000
SurfaceColor=0;0;0;0
AutoSize=1
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


RX = 0
RY = 8
TX = 100
TY = 0

P1X = 0
P1Y = 0
P2X = 8.083
P2Y = 8.083
P3X = -8.083
P3Y = 8.083
Angle = 0

[C]
Source=Calculator
Function=[C]+1
Volatile=1

[Frame]
Volatile=1
Object=Vector
Path=Rect($RX$,$RY$,200,100);Stroke(0;255;0;255);
Path1=PathSet($P1X$,$P1Y$,1,Arrow);Rotate($Angle$);Offset([Frame:W]/2,$TY$);
Arrow=LineTo($P2X$,$P2Y$);LineTo($P3X$,$P3Y$)
Path2=Join(Path);Union(Path1);Fill(255;0;0;200);
BackColor=0;255;0;0

[Title]
Object = String
Text = Tooltip title
X = $RX$ + 5
Y = $RY$ + 5
FontName = Segoe UI
StringAlign = Center
FontShadow = 1
W=[Frame:W] - $RX$ - 8


[Content]
Object = String
Source=C
Text = This is just an example text to show how the content is displayed in the tooltip %0
X = $RX$ + 5
Y = 5R
FontName = Segoe UI
FontSize = 10
FontShadow = 1
W=[Frame:W] - $RX$ - 8

#endif
