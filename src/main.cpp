// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program.  If not, see <http://www.gnu.org/licenses/>.
#include <stdio.h>
#include <SDL2/SDL.h>

#ifdef __EMSCRIPTEN__
#  include <emscripten.h>
#endif

#include "utility.h"

int main() {
    float r = mult(2.0f, 4.0f);
    if (r != 8.0f)
        return 1;

    printf("fsim starting\n");
    SDL_Init(SDL_INIT_VIDEO);

    SDL_Window *window;
    SDL_Renderer *renderer;
    SDL_CreateWindowAndRenderer(256, 256, 0, &window, &renderer);

    SDL_Surface *screen = SDL_CreateRGBSurface(0, 256, 256, 8, 0, 0, 0, 0);

    if (SDL_MUSTLOCK(screen)) SDL_LockSurface(screen);
    if (SDL_MUSTLOCK(screen)) SDL_UnlockSurface(screen);
    SDL_RenderPresent(renderer);

    SDL_Quit();
    return 0;
}

