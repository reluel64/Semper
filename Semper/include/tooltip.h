#ifdef LEGACY_TOOLTIP
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
"[Surface]\n"
"UpdateFrequency=10\n"
"SurfaceColor=0;0;0;0\n"
"AutoSize=1\n"
"\n"
";----------------Sources---------------\n"
"\n"
"[Surface-Variables]\n"
"TitleString=$ScreenW$\n"
"TextString = This is a small text to show some stuff$NL$on two lines\n"
"OX = 50\n"
"OY = 0\n"
"Orientation = 2\n"
"\n"
"[C]\n"
"Volatile=1\n"
"Source=Calculator\n"
"Function=$Orientation$\n"
"AlwaysDo=1\n"
";-----------tooltip_top----------\n"
"SourceEqual = 1 \n"
"SourceEqualAction  = \n"
"Variable(P1X,min($OX$+0,[Frame:W])); \n"
"Variable(P1Y,0);\n"
"Variable(P2X,min($OX$+8.083,[Frame:W]));\n"
"Variable(P2Y,8.083);\n"
"Variable(P3X,min(max($OX$-8.083,0),[Frame:W]));\n"
"Variable(P3Y,8.083);\n"
"Variable(RY,8);\n"
"Variable(RX,0)\n"
";----------tooltip_bottom----------\n"
"SourceEqual1 = 2 \n"
"SourceEqualAction1 =\n"
"Variable(P1X,min($OX$+0,[Frame:W])); \n"
"Variable(P1Y,10+max([Text:Y]+[Text:H],[Title:Y]+[Title:H])+8);\n"
"Variable(P2X,min($OX$+8.083,[Frame:W]));\n"
"Variable(P2Y,10+max([Text:Y]+[Text:H],[Title:Y]+[Title:H]));\n"
"Variable(P3X,min(max($OX$-8.083,0),[Frame:W]));\n"
"Variable(P3Y,10+max([Text:Y]+[Text:H],[Title:Y]+[Title:H]));\n"
"Variable(RY,0);\n"
"Variable(RX,0)\n"
";----------tooltip_left----------\n"
"SourceEqual2 = 3\n"
"SourceEqualAction2 =\n"
"Variable(P1X,0); \n"
"Variable(P1Y,min($OY$+0,[Frame:H]));\n"
"Variable(P2X,8.083);\n"
"Variable(P2Y,min($OY$+8.083,[Frame:H]));\n"
"Variable(P3X,8.083);\n"
"Variable(P3Y,min(max($OY$-8.083,0),[Frame:H]));\n"
"Variable(RY,0);\n"
"Variable(RX,8)\n"
";----------tooltip_right------------\n"
"SourceEqual3 = 4\n"
"SourceEqualAction3 =\n"
"Variable(P1X,10+max([Title:W],[Text:W])+8); \n"
"Variable(P1Y,min($OY$+0,[Frame:H]));\n"
"Variable(P2X,10+max([Title:W],[Text:W]));\n"
"Variable(P2Y,min($OY$+8.083,[Frame:H]));\n"
"Variable(P3X,10+max([Title:W],[Text:W]));\n"
"Variable(P3Y,min(max($OY$-8.083,0),[Frame:H]));\n"
"Variable(RY,0);\n"
"Variable(RX,0)\n"
"UpdateAction = Update(Title);Update(Text);Update(Frame);Update(Frame);ForceDraw()\n"
"\n"
"\n"
"[Frame]\n"
"Volatile=1\n"
"Object=Vector\n"
"Path=Rect($RX$,$RY$,10+max([Title:W],[Text:W]),10+max([Text:Y]+[Text:H],[Title:Y]+[Title:H]));Fill(255;0;0;200);\n"
"Path1=PathSet($P1X$,$P1Y$,1,Arrow);Offset($TX$,$TY$);\n"
"Arrow=LineTo($P2X$,$P2Y$);LineTo($P3X$,$P3Y$)\n"
"Path2=Join(Path);Union(Path1);\n"
"BackColor=0;255;0;0\n"
"\n"
"[Title]\n"
"volatile = 1\n"
"Object = String\n"
"Text = $TitleString$\n"
"X = $RX$ + 5\n"
"Y = $RY$ + 5\n"
"FontName = Segoe UI\n"
"StringAlign = Center\n"
"FontShadow = 1\n"
"\n"
"[Text]\n"
"Volatile = 1\n"
"Object = String\n"
"Source=C\n"
"Text = $Textstring$\n"
"X = $RX$ + 5\n"
"Y = [Title:Y]+[Title:H]+5\n"
"FontName = Segoe UI\n"
"FontSize = 10\n"
"FontShadow = 1"
#endif
#else
"[Surface]\n"
"UpdateFrequency=-1\n"
"AutoSize=1\n"
"\n"
"[Background]\n"
"Volatile=1\n"
"Object=Image\n"
"W=10+max([Title:W],[Text:W])\n"
"H=5+max([Text:Y]+[Text:H],[Title:Y]+[Title:H])\n"
"\n"
"[Title]\n"
"Volatile=1\n"
"Object = String\n"
"X = 5\n"
"Y = 5\n"
"FontName = Segoe UI\n"
"StringAlign = Center\n"
"FontShadow = 1\n"
"FontSize=12\n"
"\n"
"[Text]\n"
"Volatile=1\n"
"Object = String\n"
"X = 5\n"
"Y =[Title:Y]+[Title:H]+5\n"
"FontName = Segoe UI\n"
"FontSize = 10\n"
"FontShadow = 1\n"
#endif


