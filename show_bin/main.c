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

uint16_t buf[10 * 1024 * 1024]={0};

int main(int argc, char* argv[])
{
  SDL_Init(SDL_INIT_VIDEO);
  SDL_ShowCursor(0);
  SDL_Surface *screen = SDL_SetVideoMode(320, 480, 16, SDL_SWSURFACE);
  
  int fd = open(argv[1], O_RDONLY);
  long len = read(fd, buf, sizeof(buf));
  close(fd);

  // 86474
  long index=atoi(argv[2]);
  uint16_t *p = screen->pixels;
  for(int y=0; y<320; y++){
    for(int x=0; x<480; x++){
      *p++ = buf[index++];
    }
  }
  SDL_Flip(screen);
  SDL_Delay(3000);
  SDL_Quit();
  return 0;    
}

