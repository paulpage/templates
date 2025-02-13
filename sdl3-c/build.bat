@echo off

set FLAGS=/nologo /Zi
set LDFLAGS=/DEBUG:FULL /SUBSYSTEM:CONSOLE
set LIBS=SDL3.lib kernel32.lib user32.lib winmm.lib gdi32.lib opengl32.lib shell32.lib

REM set LIBDIR=C:\dev\lib\sdl3\lib
set LIBDIR=C:\dev\ext\SDL\build
set BINDIR=C:\dev\lib\sdl3\bin
set INCDIR=C:\dev\lib\sdl3\include

REM set SRC=render.c
set SRC=gpu.c

REM copy %LIBDIR%\SDL3.dll .

cl /Zi %SRC% SDL3.lib %FLAGS% /I %INCDIR% /link %LDFLAGS% /LIBPATH:%LIBDIR% %LIBS%

%BINDIR%\shadercross.exe shaders\2d.vert.hlsl -o shaders\2d.vert.spv
%BINDIR%\shadercross.exe shaders\2d.frag.hlsl -o shaders\2d.frag.spv


REM glslangValidator -V shaders/2d.vert -o shaders/2d.vert.spv
REM glslangValidator -V shaders/2d.frag -o shaders/2d.frag.spv
