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
#include "bg.h"
#include "fg.h"

extern uint8_t bg[];
extern uint8_t fg[];

SDL_Surface *screen;
SDL_Surface *bg_px;
SDL_Surface *fg_px;

int set_interface_attribs(int fd, int speed)
{
  struct termios tty;

  if(tcgetattr(fd, &tty) < 0){
    printf("Error from tcgetattr: %s\n", strerror(errno));
    return -1;
  }
  cfsetospeed(&tty, (speed_t)speed);
  cfsetispeed(&tty, (speed_t)speed);

  tty.c_cflag|= (CLOCAL | CREAD); // ignore modem controls
  tty.c_cflag&= ~CSIZE;
  tty.c_cflag|= CS8;      // 8-bit characters
  tty.c_cflag&= ~PARENB;  // no parity bit
  tty.c_cflag&= ~CSTOPB;  // only need 1 stop bit
  tty.c_cflag&= ~CRTSCTS; // no hardware flowcontrol

  // setup for non-canonical mode
  tty.c_iflag &= ~(IGNBRK | BRKINT | PARMRK | ISTRIP | INLCR | IGNCR | ICRNL | IXON);
  tty.c_lflag &= ~(ECHO | ECHONL | ICANON | ISIG | IEXTEN);
  tty.c_oflag &= ~OPOST;

  // fetch bytes as they become available
  tty.c_cc[VMIN] = 1;
  tty.c_cc[VTIME] = 1;

  if(tcsetattr(fd, TCSANOW, &tty) != 0){
    printf("Error from tcsetattr: %s\n", strerror(errno));
    return -1;
  }
  return 0;
}

void set_mincount(int fd, int mcount)
{
  struct termios tty;

  if(tcgetattr(fd, &tty) < 0){
    printf("Error tcgetattr: %s\n", strerror(errno));
    return;
  }

  tty.c_cc[VMIN] = mcount ? 1 : 0;
  tty.c_cc[VTIME] = 5;        /* half second timer */

  if(tcsetattr(fd, TCSANOW, &tty) < 0){
    printf("Error tcsetattr: %s\n", strerror(errno));
  }
}

void move(int x, int y)
{
  SDL_Rect rt={0};

  rt.x = x;
  rt.y = y;
  SDL_SoftStretch(bg_px, NULL, screen, NULL);
  SDL_BlitSurface(fg_px, NULL, screen, &rt);
  SDL_Flip(screen);
}

int main(int argc, char* argv[])
{
  int fd;
#if defined(DEBUG)
  const char *port = "/dev/ttyUSB0";
#else
  const char *port = "/dev/ttyS1";
#endif

  fd = open(port, O_RDWR | O_NOCTTY | O_SYNC);
  if(fd < 0){
    printf("Error opening uart port(%s): %s\n", port, strerror(errno));
    return -1;
  }
  // baudrate 115200, 8 bits, no parity, 1 stop bit
  set_interface_attribs(fd, B115200);

  SDL_Event event;
  if(SDL_Init(SDL_INIT_VIDEO) != 0){
    printf("%s, failed to SDL_Init\n", __func__);
    return -1;
  }
  SDL_ShowCursor(0);
 
  screen = SDL_SetVideoMode(320, 480, 16, SDL_HWSURFACE);
  SDL_RWops *bg_rw = SDL_RWFromMem(bg, sizeof(bg)); 
  SDL_Surface *bg_surf = IMG_Load_RW(bg_rw, 1);
  SDL_RWops *fg_rw = SDL_RWFromMem(fg, sizeof(bg)); 
  SDL_Surface *fg_surf = IMG_Load_RW(fg_rw, 1);

  bg_px = SDL_ConvertSurface(bg_surf, screen->format, 0);
  fg_px = SDL_ConvertSurface(fg_surf, screen->format, 0);
  SDL_SetColorKey(fg_px, SDL_SRCCOLORKEY, SDL_MapRGB(screen->format, 0, 0, 0));
  move(150, 120);

  int len=0;
  unsigned char buf[255]={0};
  int loop=1, x=150, y=120;
	while(loop){
    SDL_Delay(10);
    len = read(fd, buf, sizeof(buf) - 1);
    if(len > 0){
      static char dig_buf[10]={0};
      static int dig_index=0;

      for(int cnt=0; cnt<len; cnt++){
        switch(buf[cnt]){
        case '\r':
          //y = atoi(dig_buf);
          y = (atoi(dig_buf) * 480) / 1020;
          dig_index = 0;
          memset(dig_buf, 0, sizeof(dig_buf));
          break;
        case '\n':
          move(x, y);
          break;
        case ',':
          //x = atoi(dig_buf);
          x = (atoi(dig_buf) * 320) / 1020;
          dig_index = 0;
          memset(dig_buf, 0, sizeof(dig_buf));
          break;
        default:
          dig_buf[dig_index++] = buf[cnt];
          if(dig_index >= sizeof(buf)){
            printf("Invalid input(len: %d)", dig_index);
            dig_index = 0;
          }
          break;
        }
      }
    }
    else if(len < 0){
      printf("Error from read: %d: %s\n", len, strerror(errno));
    }

    SDL_PollEvent(&event);
    if(event.type == SDL_KEYDOWN){
      if(event.key.keysym.sym == SDLK_ESCAPE){
        loop = 0;
        break;
      }
    }   
  }
  SDL_FreeSurface(bg_surf);
  SDL_FreeSurface(fg_surf);
  SDL_FreeSurface(bg_px);
  SDL_FreeSurface(fg_px);
  SDL_Quit();
  return 0;    
}

