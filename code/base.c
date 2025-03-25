/** Solaris:
 * Descripción: Receptor/transmisor de la base
 * Autor: Marcos Ávila Navas
 * Version 2.0.0
 *         | | +-- Avances
 *         | +---- Cambios al diseño
 *         +------ Cambios al protocolo
 */

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <fcntl.h> 
#include <string.h>
#include <termios.h>
#include <unistd.h>

#define HEADER "Tiempo (s),Temperatura (ºC),Presión (Pa),Altitud (m),Nivel de CO (ppm)\n"

typedef struct Chunk {
  uint16_t time;
  float temp;
  float pres;
  float monox;
} Chunk;

/** set_interface_attribs : int, int, int -> int
 * Ajusta el puerto para tener la interfaz apropiada para la lectura de datos
 */
int set_interface_attribs (int fd, int speed, int parity) {
  /* Los detalles no son importantes, pero básicamente configuramos una interfaz
   * no-canónica, con VMIN = 0 y VTIME = 20
   */
  struct termios tty;
  if (tcgetattr (fd, &tty) != 0) {
    fprintf(stderr, "error %d from tcgetattr\n", errno);
    return -1;
  }

  cfsetospeed(&tty, speed);
  cfsetispeed(&tty, speed);

  tty.c_cflag = (tty.c_cflag & ~CSIZE) | CS8;
  tty.c_iflag &= ~IGNBRK;
  tty.c_lflag = 0;
  tty.c_oflag = 0;
  tty.c_cc[VMIN]  = 0;
  tty.c_cc[VTIME] = 20;
  tty.c_iflag &= ~(IXON | IXOFF | IXANY);
  tty.c_cflag |= (CLOCAL | CREAD);
  tty.c_cflag &= ~(PARENB | PARODD);
  tty.c_cflag |= parity;
  tty.c_cflag &= ~CSTOPB;
  tty.c_cflag &= ~CRTSCTS;

  if (tcsetattr (fd, TCSANOW, &tty) != 0) {
    fprintf(stderr, "error %d from tcsetattr\n", errno);
    return -1;
  }
  return 0;
}

static
int report(FILE* src, FILE* dst) {
  Chunk chunk;
  fread(&chunk, sizeof(Chunk), 1, src);
  const int ret = fprintf(dst, "%d,%f,%f,%f\n", chunk.time, chunk.temp, chunk.pres, chunk.monox);
  fprintf(stdout, "%d,%f,%f,%f\n", chunk.time, chunk.temp, chunk.pres, chunk.monox);
  return ret;
}

int main(int argc, char *argv[]) {
  if (argc < 3 && 4 > argc) {
    fprintf(stderr, "Usage: %s [port] [file] [chunk_size?]\n", argv[0]);
    return 1;
  }
  FILE* serial  = fopen(argv[1], "r");
  if (!serial) {
    fprintf(stderr, "error %d opening %s: %s\n", errno, argv[1], strerror(errno));
    return 1;
  }
  FILE* file = fopen(argv[2], "w+");
  if (!file) {
    fprintf(stderr, "error %d opening %s: %s\n", errno, argv[2], strerror(errno));
    return 1;
  }

  set_interface_attribs(fileno(serial), B9600, 0);

  long read_buf_s = argc == 3? 16 : atoi(argv[3])
     , read_s;
  uint32_t recv = 0
         , top;
  char read_buf[read_buf_s];

  printf(
    "\033c"
    "=== Programa de lectura WeCan ===\n\n"
    HEADER
  );

  fprintf(file, HEADER);

  while (1) if (fread(&top, sizeof(uint32_t), 1, serial)) {
    fwrite(&recv, sizeof(uint32_t), 1, serial);

    while (recv < top && report(serial, file)) recv++;
  }

}
