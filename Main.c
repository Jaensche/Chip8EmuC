#include <SDL2/SDL.h>
#include "SDL2/SDL_syswm.h"
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <windows.h>
#include <commdlg.h>
#include "CPU.c"

const int RES_X = 64;
const int RES_Y = 32; 

SDL_Window* initWindow()
{
	SDL_Window* window = NULL;
    window = SDL_CreateWindow
    (
        "CHIP8 by Jan", SDL_WINDOWPOS_UNDEFINED,
        SDL_WINDOWPOS_UNDEFINED,
        640,
        320,
        SDL_WINDOW_SHOWN
    );

    return window;
}

SDL_Renderer* getRenderer(SDL_Window* window)
{
    SDL_Renderer* renderer = NULL;
    renderer =  SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED);

    SDL_RenderSetLogicalSize(renderer, RES_X, RES_Y);    

    return renderer;
}

void killWindow(SDL_Window* window, SDL_Renderer* renderer)
{    
    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    SDL_Quit();
}

void render(SDL_Window* window, SDL_Renderer* renderer, unsigned char **gfx) 
{
    SDL_SetRenderDrawColor(renderer, 0, 0, 255, 255);
    SDL_RenderClear(renderer);

    SDL_SetRenderDrawColor(renderer, 255, 255, 255, 255);

    for (int y = 0; y < RES_Y; y++)
    {
        for (int x = 0; x < RES_X; x++)
        {
            if((*gfx)[y * RES_X + x] == 1)
            {
                SDL_RenderDrawPoint(renderer, x, y);
            }
        }
    }

    SDL_RenderPresent(renderer);
    SDL_UpdateWindowSurface(window);
}

long readFile(const char *filename, unsigned char **buffer)
{
    FILE *fileptr;
    long filelen;
    fileptr = fopen(filename, "rb"); // Open the file in binary mode
    fseek(fileptr, 0, SEEK_END);         // Jump to the end of the file
    filelen = ftell(fileptr);            // Get the current byte offset in the file
    rewind(fileptr);                     // Jump back to the beginning of the file

    *buffer = (unsigned char *)malloc(filelen * sizeof(unsigned char)); // Enough memory for the file
    if(*buffer == NULL) 
    {
        printf("Error! memory not allocated.");
        exit(0);
    }

    fread(*buffer, filelen, 1, fileptr);              // Read in the entire file
    fclose(fileptr);                                 // Close the file

    return filelen;
}

void initChip8(char *file)
{
    unsigned char *buffer;    
    long length = readFile(file, &buffer);

    Load(&buffer, length);
    free(buffer);  
}

char* openFileDialog(SDL_Window* window)
{
    SDL_SysWMinfo wmInfo;
    SDL_VERSION(&wmInfo.version);
    SDL_GetWindowWMInfo(window, &wmInfo);
    HWND hwnd = wmInfo.info.win.window;

    OPENFILENAME ofn;       // common dialog box structure
    TCHAR szFile[260] = { 0 };       // if using TCHAR macros
    char szFileName[MAX_PATH];
    // Initialize OPENFILENAME
    ZeroMemory(&ofn, sizeof(ofn));
    ofn.lStructSize = sizeof(ofn);
    ofn.hwndOwner = hwnd;
    ofn.lpstrFile = szFileName;
    ofn.nMaxFile = MAX_PATH;
    ofn.lpstrFilter = TEXT("All\0*.*\0Text\0*.TXT\0");
    ofn.nFilterIndex = 1;
    ofn.lpstrFileTitle = NULL;
    ofn.nMaxFileTitle = 0;
    ofn.lpstrInitialDir = NULL;
    ofn.Flags = OFN_PATHMUSTEXIST | OFN_FILEMUSTEXIST;

    if (GetOpenFileName(&ofn) == TRUE)
    {
        char *copy = malloc(strlen(ofn.lpstrFile) + 1); 
        strcpy(copy, ofn.lpstrFile);
        return copy;
    }
}

int main (int argc, char** argv)
{
    srand(time(0));

    unsigned char *gfx = (unsigned char *)calloc(RES_X * RES_Y, sizeof(char));
    Reset(&gfx);

    SDL_Window* window = initWindow();
    SDL_Renderer* renderer = getRenderer(window);
    SDL_Event event = { 0 };

    char* programFile = openFileDialog(window);    
    initChip8(programFile);

    int exit = 0;
    while(!exit) 
    {
       while(SDL_PollEvent(&event)) 
       {
            switch(event.type) 
            {
                case SDL_QUIT:
                    exit = 1;
                    break;
                case SDL_KEYDOWN:
                    if(event.key.keysym.sym == SDLK_ESCAPE) 
                    {    
                        exit = 1;
                    }
                    if(event.key.keysym.sym = SDLK_F1)
                    {
                        Reset(&gfx);
                        programFile = openFileDialog(window);    
                        initChip8(programFile);
                    }
                    break;
                case SDL_WINDOWEVENT:
                    switch(event.window.event) 
                    {
                        case SDL_WINDOWEVENT_CLOSE:
                            exit = 1;
                            break;
                    }
                    break;
                default: 
                    break;
            }
        }

        if(cycle(&gfx))
        {
            render(window, renderer, &gfx);            
        }

        Sleep(50);
    }

    killWindow(window, renderer);
    free(gfx);

    return 0;
}