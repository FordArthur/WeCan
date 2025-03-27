/** Solaris:
 * Descripción: Receptor/transmisor de la base
 * Autor: Marcos Ávila Navas
 * Version 2.0.0
 *         | | +-- Avances
 *         | +---- Cambios al diseño
 *         +------ Cambios al protocolo
 */

#define DEBUG // Cambiar entre #define y #undef

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

/** set_interface_attribs : int -> int
 * Ajusta el puerto para tener la interfaz apropiada para la lectura de datos
 */
static
int set_interface_attribs(int fd) {
  struct termios options;

  if (tcgetattr(fd, &options) != 0) {
    perror("tcgetattr");
    return -1;
  }

  cfsetispeed(&options, B9600);
  cfsetospeed(&options, B9600);

  options.c_cflag = CS8 | CLOCAL | CREAD;
  options.c_iflag = IGNPAR;
  options.c_oflag = 0;
  options.c_lflag = 0;

  if (tcsetattr(fd, TCSANOW, &options) != 0) {
    perror("tcsetattr");
    return -1;
  }

  return 0;
}

static
int report(int src, int dst) {
  Chunk chunk;
  read(src, &chunk, sizeof(Chunk));

  printf("%d,%f,%f,%f\n", chunk.time, chunk.temp, chunk.pres, chunk.monox);
  return dprintf(dst, "%d,%f,%f,%f\n", chunk.time, chunk.temp, chunk.pres, chunk.monox);;
}

int main(int argc, char *argv[]) {
  if (argc < 3 && 4 > argc) {
    fprintf(stderr, "Usage: %s [port] [file] [chunk_size?]\n", argv[0]);
    return 1;
  }

  int serial  = open(argv[1], O_RDWR | O_NOCTTY | O_NDELAY);
  if (serial < 0) {
    fprintf(stderr, "error %d opening %s: %s\n", errno, argv[1], strerror(errno));
    return 1;
  }

  int file = open(argv[2], O_RDWR | O_CREAT | O_TRUNC);
  if (!file) {
    fprintf(stderr, "error %d opening %s: %s\n", errno, argv[2], strerror(errno));
    return 1;
  }

  if (set_interface_attribs(serial) < 0)
    return 1;

  long read_buf_s = argc == 3? 16 : atoi(argv[3])
     , read_s;

  uint16_t recv = 0
         , top;
  char read_buf[read_buf_s];

  printf(
    "\033c"
    "=== Programa de lectura WeCan ===\n\n"
    HEADER
  );

  write(file, HEADER, sizeof(HEADER));

  while (1) if (read(serial, &top, sizeof(top))) {
    write(serial, &recv, sizeof(recv));
    while (recv < top && report(serial, file)) ++recv;
  }
  return 0;
}
