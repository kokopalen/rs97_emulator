#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <unistd.h>
#include <errno.h>
#include <termios.h>
#include <unistd.h>
#include <SDL.h>
#include <SDL_image.h>
#include <SDL_ttf.h>

uint16_t buf[320*480*2]={0};

int main(int argc, char* argv[])
{
  SDL_Init(SDL_INIT_VIDEO);
  SDL_ShowCursor(0);
  SDL_Surface *screen = SDL_SetVideoMode(320, 480, 16, SDL_SWSURFACE);

  SDL_Surface* img = IMG_Load(argv[1]);
  SDL_Surface *p = SDL_ConvertSurface(img, screen->format, 0);
  SDL_SoftStretch(p, NULL, screen, NULL);
  SDL_FreeSurface(img);
  SDL_FreeSurface(p);
  SDL_Flip(screen);
  
  // 86474
  long index=0;
  uint16_t *px = screen->pixels;
  for(int y=0; y<320; y++){
    for(int x=0; x<480; x++){
      buf[index++] = *px++;
    }
  }

  int fd = open("hex.bin", O_CREAT | O_WRONLY, S_IRUSR);
  long len = write(fd, buf, 320*480*2);
  close(fd);

  SDL_Delay(3000);
  SDL_Quit();
  return 0;    
}

