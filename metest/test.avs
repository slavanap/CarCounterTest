loadplugin("metest.dll")
ffmpegsource2("c:\Users\Vyacheslav\Projects\TestVideo\day.avi")
converttorgb32
LanczosResize(width, height)
src
interleave(src, src.sample.LanczosResize(src.width, src.height))
