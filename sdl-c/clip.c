#include <windows.h>
#include <windowsx.h>

#include <SDL.h>
#include <SDL_syswm.h>
#include <stdbool.h>

#pragma comment(lib, "user32.lib")

HWND get_raw_window(SDL_Window *window) {
    SDL_SysWMinfo wmInfo;
    SDL_VERSION(&wmInfo.version);
    SDL_GetWindowWMInfo(window, &wmInfo);
    return wmInfo.info.win.window;
}

void enum_clipboard_formats(SDL_Window *window) {
    /* HWND hwnd = get_raw_window(window); */

    /* if (!OpenClipboard(hwnd)) { */
    if (!OpenClipboard(NULL)) {
        printf("Couldn't open clipboard\n");
        return;
    }

    UINT format = 0;

    while (true) {
        format = EnumClipboardFormats(format);
        if (format == 0) {
            DWORD err = GetLastError();
            if (err == ERROR_SUCCESS) {
                printf("No more clipboard formats\n");
            } else {
                printf("Error getting clipboard formats: %d\n", err);
            }
            break;
        } else {
            printf("Got new clipboard format: %d\n", format);
        }
    }
}

bool copy_text(char *text, int len, SDL_Window *window) {
    HWND hwnd = get_raw_window(window);

    if (!OpenClipboard(hwnd)) {
        printf("Couldn't open clipboard\n");
        return false;
    }

    HGLOBAL text_mem = GlobalAlloc(GMEM_MOVEABLE, len + 1 * sizeof(TCHAR));

    if (text_mem == NULL) {
        CloseClipboard();
        printf("Mem is null\n");
        return false;
    }

    LPSTR text_ptr = GlobalLock(text_mem);
    memcpy(text_ptr, text, len);
    // make sure string is null-terminated
    // text_ptr[len] = 0;
    GlobalUnlock(text_ptr);

    EmptyClipboard();
    SetClipboardData(CF_TEXT, text_mem);

    CloseClipboard();
    return true;
}
