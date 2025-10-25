@echo off

set LDFLAGS=/DEBUG:FULL /SUBSYSTEM:CONSOLE
set LIBS=SDL3.lib kernel32.lib user32.lib winmm.lib gdi32.lib opengl32.lib shell32.lib

set LIBDIR=C:\dev\lib\sdl3\lib
set BINDIR=C:\dev\lib\sdl3\bin
set INCDIR=C:\dev\lib\sdl3\include

set SRC=gpu.c

shadercross.exe shader.vert.hlsl -o shader.vert.spv
shadercross.exe shader.frag.hlsl -o shader.frag.spv
shadercross.exe shader.vert.hlsl -o shader.vert.dxil
shadercross.exe shader.frag.hlsl -o shader.frag.dxil
REM %BINDIR%\shadercross.exe shader.vert.hlsl -o shader.vert.dxil
REM %BINDIR%\shadercross.exe shader.frag.hlsl -o shader.frag.dxil
REM dxc -T vs_6_0 shader.vert.hlsl -Fo shader.vert.dxil
REM dxc -T ps_6_0 shader.frag.hlsl -Fo shader.frag.dxil
cl /nologo /Zi main.c /I %INCDIR% /link %LDFLAGS% /LIBPATH:%LIBDIR% %LIBS%
if /i "%1"=="run" (
    .\main.exe
)
