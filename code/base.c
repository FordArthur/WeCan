/** WeCan:
 * Descripción: Receptor/transmisor de la base
 * Autor: Marcos Ávila Navas
 * Version 2.0.3
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
#include <time.h>
#include <bits/floatn-common.h>

#define HEADER "Tiempo (s),Temperatura (ºC),Presión (Pa),Altitud (m),Nivel de CO (ppm)\n"

typedef struct Chunk {
  uint32_t time;
  float temp;
  float pres;
  float altur;
  float monox;
} Chunk;

/** set_interface_attribs : int -> int
 * Ajusta el puerto para tener la interfaz apropiada para la lectura de datos
 */
static
int set_interface_attribs(int fd) {
  // Obtenemeos la configuración del fd
  struct termios options;

  // Si hay algún error, devolvemos -1
  if (tcgetattr(fd, &options) != 0) {
    perror("tcgetattr while setting interface");
    return -1;
  }

  // velocidad I/O de fd = B9600
  cfsetispeed(&options, B9600);
  cfsetospeed(&options, B9600);

  // No ignoramos BRKs
  options.c_iflag &= ~IGNBRK;

  /*** Para procesamiento crudo ***/
  // Ninguna local mode flag
  // (~ICANON, ~ECHO, ~ISIG, ~IEXTEN)
  options.c_lflag = 0;
  options.c_iflag &= ~(IXON | IXOFF | IXANY);
  options.c_oflag = 0;

  // configuración de serial (paridad, por ejemplo)
  options.c_cflag |= (CLOCAL | CREAD);
  options.c_cflag &= ~(PARENB | PARODD);
  options.c_cflag &= ~CRTSCTS;

  /*** Para la aplicación ***/
  // Read debe leer al menos sizeof(Chunk) carácteres antes de devolver
  // (por esto requerimos que sizeof(Chunk) == sizeof(Handshake))
  options.c_cc[VMIN] = sizeof(Chunk);
  // Esperamos el tiempo máximo (2s) para que complete read
  options.c_cc[VTIME] = 20;

  // Aplicamos la configuración
  if (tcsetattr(fd, TCSANOW, &options) != 0) {
    perror("tcsetattr while setting interface");
    return -1;
  }

  return 0;
}

static inline
ssize_t send_handshake(int serial, uint32_t code) {
  char hcode[sizeof(Chunk)];
  mempcpy(hcode, &code, sizeof(code));
  return write(serial, hcode, sizeof(hcode));
}

static inline
uint32_t get_handshake(int serial) {
  char hcode[sizeof(Chunk)];
  ssize_t x;
  if ((x = read(serial, hcode, sizeof(hcode))) < 0 && ((float*) hcode)[1] != __builtin_nanf32(""))
    return 0;
  return ((uint32_t*)hcode)[0];
}

/** delay : time_t -> void
 * delay espera `secs` segundos
 */
static
void delay(time_t secs) {
  for (time_t future = time(NULL) + secs; time(NULL) < future;);
}


/** report : int, int -> int
 * report(src, dst) lee un Chunk de src, lo escribe en dst y en stdout, y devuelve el número
 * de carácteres impresos a dst
 */
static
int report(int src, int dst) {
  // Leemos un Chunk de src
  Chunk chunk;
  if (read(src, &chunk, sizeof(Chunk)) < 0 || chunk.temp == __builtin_nanf32(""))
    return 0;

  // Y lo imprimimos en stdout y dst
  printf("%d,%f,%f,%f\n", chunk.time, chunk.temp, chunk.pres, chunk.monox);
  return dprintf(dst, "%d,%f,%f,%f\n", chunk.time, chunk.temp, chunk.pres, chunk.monox);;
}

int main(int argc, char *argv[]) {
  if (argc < 3 && 4 > argc) {
    fprintf(stderr, "Usage: %s [port] [file] [chunk_size?]\n", argv[0]);
    return 1;
  }

  // Abrimos los archivos
  int serial  = open(argv[1], O_RDWR | O_NOCTTY | O_NDELAY);
  if (serial < 0) {
    fprintf(stderr, "error %d opening %s: %s\n", errno, argv[1], strerror(errno));
    return 1;
  }

  int file = open(argv[2], O_RDWR | O_CREAT | O_TRUNC, 0644);
  if (!file) {
    fprintf(stderr, "error %d opening %s: %s\n", errno, argv[2], strerror(errno));
    return 1;
  }

  // Configuramos la interfaz del serial
  if (set_interface_attribs(serial) < 0)
    return 1;

  // recv será cuantos datos hemos leídos
  // top será cuantos datos ha procesado el satélite
  uint32_t recv = 0
         , top;

  printf(
    "\033c"
    "=== Programa de lectura WeCan ===\n\n"
    HEADER
  );

  write(file, HEADER, sizeof(HEADER));

  while (1) if ((top = get_handshake(serial))) {
    send_handshake(serial, recv);

    while (recv < top && report(serial, file)) ++recv;
    delay(1);
  }
  return 0;
}
