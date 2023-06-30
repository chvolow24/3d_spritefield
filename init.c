#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <math.h>
#include <time.h>
#include <pthread.h>
#include <SDL2/SDL.h>
#include <SDL2/SDL_timer.h>
#include <SDL2/SDL_image.h>
#include <SDL2/SDL_audio.h>

/* Dimensions of window to be created */
#define WINDOWHEIGHT 800
#define WINDOWWIDTH 1400

/* Number of total sprites to be rendered, and how far they move with each increment */
#define NUMSPRITES 5000
#define STEPSIZE 100

/* Time to sleep between steps */
#define ANIMATION_DELAY 10

/* Set GIFFRAMES to some nonzero integer if you want to create an animation */
#define GIFFRAMES 0

/* Struct to store information about a given sprite: size, position, real and screen coords */
typedef struct Sprite { 
  SDL_Texture* tex;
  SDL_Rect dest;
  int zreal;
  int xreal;
  int yreal;
  int hreal;
  int wreal;
} Sprite;

/* An array of sprites to be rendered */
Sprite** sprites; 

/* Threads for handling keystrokes */
pthread_t upkey;
pthread_t downkey;
pthread_t rightkey;
pthread_t leftkey;

pthread_mutex_t key_event_mutex;
uint8_t active_key = 0; /* One of 1,2,3,4: Up, Down, Left Right, or 0: none */

/* Threadsafe function to get the currently active key */
uint8_t get_active_key()
{
  uint8_t retval;
  pthread_mutex_lock(&key_event_mutex);
  retval = active_key;
  pthread_mutex_unlock(&key_event_mutex);
  return retval;
}

/* Threadsafe function to set the currently active key */
void set_active_key(uint8_t keyval)
{
  pthread_mutex_lock(&key_event_mutex);
  active_key = keyval;
  pthread_mutex_unlock(&key_event_mutex);
}

/* Create a sprite from a path to an image and xyz coords */
int init_sprite(Sprite* sp, char* path, SDL_Renderer* rend, int x, int y, int z)
{
  SDL_Surface* surface = IMG_Load(path);
  if (!surface) {
    fprintf(stderr, "\nError: failed to create surface.\n");
    return 1;
  }
  sp->tex = SDL_CreateTextureFromSurface(rend, surface);
  SDL_FreeSurface(surface);
  SDL_QueryTexture(sp->tex, NULL, NULL, &sp->dest.w, &sp->dest.h);
  sp->xreal = x;
  sp->yreal = y;
  sp->zreal = z;
  sp->hreal = sp->dest.h;
  sp->wreal = sp->dest.w;
  return 0;
}

/* Render a sprite */
int render_sprite(Sprite* sp, SDL_Renderer* rend)  
{
  return SDL_RenderCopy(rend, sp->tex, NULL, &(sp->dest));
}

/* Resize a sprite */
int sprite_scale(Sprite* sp, double f) {
  sp->dest.w *= f;
  sp->dest.h *= f;
  sp->hreal *= f;
  sp->wreal *= f;
  return 0;
}

/* Get screen coords and size for a sprite based on xyz coords */
void real_to_screen(Sprite* sp) {
  sp->dest.w = ((double) sp->wreal * 1 * (double) WINDOWHEIGHT) / ((double)sp->zreal);
  sp->dest.h = ((double) sp->hreal * 1 * (double) WINDOWHEIGHT) / ((double)sp->zreal);
  sp->dest.y = (((double) sp->yreal * 1 * (double) WINDOWHEIGHT) / ((double) sp->zreal)) + WINDOWHEIGHT/2;
  sp->dest.x = (((double) sp->xreal * 1 * (double) WINDOWHEIGHT) / ((double) sp->zreal)) + WINDOWWIDTH/2;
}

/* Move the animation and all sprites exactly one step */
void step(uint8_t direction) /* direction: 0 = towards screen, 1 = up, 2 = down, 3 = left, 4 = right */
{ 
  for (int i=0; i<NUMSPRITES; i++) {

    Sprite* sp = sprites[i];

    switch (direction) {
    /* Sprites fly toward screen. Send sprite to the back if it passes through screen. */
    case 0:
      if (sp->zreal - STEPSIZE > 0) {
        sp->zreal -= STEPSIZE;
      } else {
        sp->zreal = 50 * WINDOWWIDTH;
      }
      break;

    /* The remaining cases handle user movement amidst the sprite field */
    /* User moves up */
    case 1:
      sp->yreal += STEPSIZE;
      break;
    /* User moves down */
    case 2:
      sp->yreal -= STEPSIZE;
      break;
    /* User moves left */
    case 3:
      sp->xreal += STEPSIZE;
      break;
    /* User moves right */
    case 4:
      sp->xreal -= STEPSIZE;
      break;
    }
    /* re-compute sprite dimensions and coords */
    real_to_screen(sp);
  }
}

/* Directional move functions run on continuous loop, each in its own thread.
If movement is to stop, the thread is killed. */

void* move_up(void* nullargs) {
  printf("move up called");
  while (1) {
    step(1);
    SDL_Delay(10);
  }
}
void* move_down(void* nullargs) {
  while (1) {
    step(2);
    SDL_Delay(10);
  }
}
void* move_left(void* nullargs) {
  while (1) {
    step(3);
    SDL_Delay(10);
  }
}
void* move_right(void* nullargs) {
  while (1) {
    step(4);
    SDL_Delay(10);
  }
}

/* Takes a bmp screenshot and saves to the 'images' subdirectory, with index i included in filename. */
void screenshot(int i, SDL_Renderer* rend)
{
  char filename[30];
  sprintf(filename, "images/screenshot%3d.bmp", i);
  printf("\nSaved %s", filename);
  SDL_Surface *sshot = SDL_CreateRGBSurface(0, WINDOWWIDTH, WINDOWHEIGHT, 32, 0x00ff0000, 0x0000ff00, 0x000000ff, 0xff000000);
  SDL_RenderReadPixels(rend, NULL, SDL_PIXELFORMAT_ARGB8888, sshot->pixels, sshot->pitch);
  SDL_SaveBMP(sshot, filename);
  SDL_FreeSurface(sshot);
}

int main() 
{
  srand(time(NULL));

  /* Allocate space for the sprite stars. Freed on program exit. */
  size_t arraysize = NUMSPRITES * sizeof(Sprite*);
  sprites = malloc(arraysize);

  /* Initialize SDL Video */
  if (SDL_Init(SDL_INIT_VIDEO) != 0) {
    fprintf(stderr, "\nError initializing SDL: %s\n", SDL_GetError());
    return 1;
  }
  printf("Initialization successful.\n");

  SDL_Window* win = SDL_CreateWindow("spritefield", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, WINDOWWIDTH, WINDOWHEIGHT, 0);

  if (!win) {
    fprintf(stderr, "\nWindow initialization failed: %s\n", SDL_GetError());
    SDL_Quit();
    return 1;
  }
  printf("Window initialized successfully.\n");

  Uint32 render_flags = SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC;
  SDL_Renderer* rend = SDL_CreateRenderer(win, -1, render_flags);
  if (!rend)
  {
    fprintf(stderr, "\nError creating renderer: %s\n", SDL_GetError());
    SDL_DestroyWindow(win);
    SDL_Quit();
    return 1;
  }

  SDL_RenderClear(rend);

  /* Create, size, and render all of the sprites */
  for (int i=0; i<NUMSPRITES; i++) {
    Sprite* treesprite;
    treesprite = malloc(sizeof(Sprite));
    int sprite_selection = rand() % 2;
    char* path;
    int posneg = rand() % 2 == 0 ? -1 : 1;
    int posneg2 = rand() % 2 == 0 ? -1 : 1;
    if (sprite_selection == 0) {
      path = "resources/tree1.png";
      init_sprite(treesprite, path, rend, posneg * rand()%WINDOWWIDTH * 20, posneg2 * rand()%WINDOWHEIGHT * 20, (rand()%50 +1) * WINDOWWIDTH);
      sprite_scale(treesprite, 2.8);

    } else {
      path = "resources/tree2.png";
      init_sprite(treesprite, path, rend, posneg * rand()%WINDOWWIDTH * 20, posneg2 * rand()%WINDOWHEIGHT * 20, (rand()%50 +1) * WINDOWWIDTH);
      sprite_scale(treesprite, 2.8);
    }
    real_to_screen(treesprite);
    sprites[i] = treesprite;
  }

  bool quit = false;
  SDL_Event e;

  int frame_index = 0;
  int screenshot_index = 0;

  /* Animation loop */
  while (!quit) {
    while (SDL_PollEvent(&e)) {
      if (e.type == SDL_QUIT){
        quit = true;
      } else if (e.type == SDL_KEYDOWN) {
        switch (e.key.keysym.scancode) {
        case SDL_SCANCODE_W:
        case SDL_SCANCODE_UP:
          if (get_active_key() != 1) {
            pthread_create(&upkey, NULL, move_up, NULL);
            set_active_key(1);
          }
          break;
        case SDL_SCANCODE_S:
        case SDL_SCANCODE_DOWN:
          if (get_active_key() != 2) {
            pthread_create(&downkey, NULL, move_down, NULL);
            set_active_key(2);
          }
          break;

        case SDL_SCANCODE_A:
        case SDL_SCANCODE_LEFT:
          if (get_active_key() != 3) {
            pthread_create(&leftkey, NULL, move_left, NULL);
            set_active_key(3);
          }
          break;
        case SDL_SCANCODE_D:
        case SDL_SCANCODE_RIGHT:
          if (get_active_key() != 4) {
            pthread_create(&rightkey, NULL, move_right, NULL);
            set_active_key(4);
          }
          break;
        default:
          break;
        }
      } else if (e.type == SDL_KEYUP) {
        switch (e.key.keysym.scancode) {
          case SDL_SCANCODE_W:
          case SDL_SCANCODE_UP:
          pthread_cancel(upkey);
          break;
          case SDL_SCANCODE_S:
          case SDL_SCANCODE_DOWN:
          pthread_cancel(downkey);
          break;
          case SDL_SCANCODE_A:
          case SDL_SCANCODE_LEFT:
          pthread_cancel(leftkey);
          case SDL_SCANCODE_D:
          case SDL_SCANCODE_RIGHT:
          pthread_cancel(rightkey);
          break;
          default:
            break;
        }
      }
    }
    SDL_RenderClear(rend);
    for (int i=0; i<NUMSPRITES; i++) {
      Sprite* sp = sprites[i];
      render_sprite(sp, rend);
    }
    SDL_RenderPresent(rend);

    if (frame_index >= 100 && screenshot_index < GIFFRAMES) {
      screenshot(screenshot_index,rend);
      screenshot_index++;
      /* add delay to avoid errors in image rendering */
      SDL_Delay(10);
    }
    frame_index++;
    

    /* Sprites step toward the screen every turn */
    step(0);
    SDL_Delay(1);

  }
  SDL_DestroyWindow(win);
  SDL_Quit();
}
