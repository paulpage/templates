REM @echo off
REM set SDL2_LIB=\dev\lib\sdl2\bin\x64\Release\
REM set SDL2_INC=\dev\lib\sdl2\SDL2-2.28.4\include\
set SDL2_LIB=\dev\lib\sdl2\SDL2-2.28.5\lib\x64\
set SDL2_INC=\dev\lib\sdl2\SDL2-2.28.5\include\
REM cl /nologo /Zi /FC /MD main.c /I %SDL2_INC% /link /LIBPATH:%SDL2_LIB% user32.lib kernel32.lib gdi32.lib shell32.lib SDL2.lib SDL2main.lib /entry:WinMainCRTStartup /SUBSYSTEM:WINDOWS
REM cl /nologo /Zi /FC /MD main.c /I %SDL2_INC% /link /LIBPATH:%SDL2_LIB% user32.lib kernel32.lib gdi32.lib shell32.lib SDL2.lib SDL2main.lib /entry:WinMainCRTStartup

set SDL_PATH=\dev\lib\sdl2\bin\x64\Debug\
set SDLMAIN_PATH=\dev\lib\sdl2\bin\x64\Debug\
copy %SDL_PATH%\SDL2.dll .
copy %SDL_PATH%\SDL2.lib .
copy %SDL_PATH%\SDL2.pdb .
copy %SDLMAIN_PATH%\SDL2main.lib .


cl %CommonCompilerFlags% main.c clip.c /I ..\lib /I \dev\lib\sdl2\SDL2-2.28.4\include /link /DEBUG:FULL /SUBSYSTEM:CONSOLE /LIBPATH:C:\dev\lib\sdl2\SDL2-2.28.4\VisualC\SDL\x64\Debug\ /LIBPATH:C:\dev\lib\sdl2\SDL2-2.28.4\VisualC\SDLmain\x64\Debug\ SDL2.lib SDL2main.lib kernel32.lib user32.lib winmm.lib gdi32.lib opengl32.lib shell32.lib
