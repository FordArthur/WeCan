#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <fcntl.h> 
#include <string.h>
#include <termios.h>
#include <unistd.h>

int set_interface_attribs (int fd, int speed, int parity) {
  struct termios tty;
  if (tcgetattr (fd, &tty) != 0) {
    fprintf(stderr, "error %d from tcgetattr\n", errno);
    return -1;
  }

  cfsetospeed(&tty, speed);
  cfsetispeed(&tty, speed);

  tty.c_cflag = (tty.c_cflag & ~CSIZE) | CS8;     // 8-bit chars
  // disable IGNBRK for mismatched speed tests; otherwise receive break
  // as \000 chars
  tty.c_iflag &= ~IGNBRK;         // disable break processing
  tty.c_lflag = 0;                // no signaling chars, no echo,
  // no canonical processing
  tty.c_oflag = 0;                // no remapping, no delays
  tty.c_cc[VMIN]  = 0;            // read doesn't block
  tty.c_cc[VTIME] = 5;            // 0.5 seconds read timeout

  tty.c_iflag &= ~(IXON | IXOFF | IXANY); // shut off xon/xoff ctrl

  tty.c_cflag |= (CLOCAL | CREAD);// ignore modem controls,
  // enable reading
  tty.c_cflag &= ~(PARENB | PARODD);      // shut off parity
  tty.c_cflag |= parity;
  tty.c_cflag &= ~CSTOPB;
  tty.c_cflag &= ~CRTSCTS;

  if (tcsetattr (fd, TCSANOW, &tty) != 0) {
    fprintf(stderr, "error %d from tcsetattr\n", errno);
    return -1;
  }
  return 0;
}

void set_blocking (int fd, int should_block) {
  struct termios tty;
  memset (&tty, 0, sizeof tty);
  if (tcgetattr (fd, &tty) != 0) {
    fprintf(stderr, "error %d from tggetattr\n", errno);
    return;
  }

  tty.c_cc[VMIN]  = should_block ? 1 : 0;
  tty.c_cc[VTIME] = 5;            // 0.5 seconds read timeout

  if (tcsetattr (fd, TCSANOW, &tty) != 0)
    fprintf(stderr, "error %d setting term attributes\n", errno);
}

int main(int argc, char *argv[]) {
  if (argc < 3 && 4 > argc) {
    fprintf(stderr, "Usage: %s [port] [file] [chunk_size?]\n", argv[0]);
    return 1;
  }
  int serial_fd  = open(argv[1], O_RDWR | O_NOCTTY | O_SYNC);
  if (serial_fd < 0) {
    fprintf(stderr, "error %d opening %s: %s\n", errno, argv[1], strerror(errno));
    return 1;
  }
  int file_fd = open(argv[2], O_RDWR | O_CREAT | O_APPEND);
  if (serial_fd < 0) {
    fprintf(stderr, "error %d opening %s: %s\n", errno, argv[2], strerror(errno));
    return 1;
  }

  set_interface_attribs(serial_fd, B9600, 0);
  set_blocking(serial_fd, 1);

  unsigned long read_buf_s = argc == 3? 16 : atoi(argv[3])
  , read_s;
  char read_buf[read_buf_s];
  memset(read_buf, '\0', read_buf_s);

  while (1) {
    if ((read_s = read(serial_fd, read_buf, read_buf_s)) < 0) {
      fprintf(stderr, "error reading from serial: %s\n", strerror(errno));
      return 1;
    }

    if (write(file_fd, read_buf, read_s) < 0) {
      fprintf(stderr, "error writing to file: %s\n", strerror(errno));
      return 1;
    }
  }
}
