@echo off

set FLAGS=/nologo /Zi
set LDFLAGS=/DEBUG:FULL /SUBSYSTEM:CONSOLE
set LIBS=SDL3.lib kernel32.lib user32.lib winmm.lib gdi32.lib opengl32.lib shell32.lib

set LIBDIR=C:\dev\lib\sdl3\bin\x64\
set INCDIR=C:\dev\lib\sdl3\SDL\include\

set SRC=main.c

copy %LIBDIR%\SDL3.dll .

cl %SRC% %FLAGS% /I %INCDIR% /link %LDFLAGS% /LIBPATH:%LIBDIR% %LIBS%
