/** Solaris:
 * Descripción: Receptor/transmisor de la base
 * Autor: Marcos Ávila Navas
 * Version 1.0.2
 *         | | +-- Avances
 *         | +---- Cambios al diseño
 *         +------ Cambios al protocolo
 */

#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <fcntl.h> 
#include <string.h>
#include <termios.h>
#include <unistd.h>
#include <pthread.h>
#include <signal.h>
#include <stdint.h>

/** set_interface_attribs : int, int, int -> int
 * Ajusta el puerto para tener la interfaz apropiada para la lectura de datos
 */
static
int set_interface_attribs (int fd, int speed, int parity) {
  /* Los detalles no son importantes, pero básicamente configuramos una interfaz
   * no-canónica, con VMIN = 0 y VTIME = 5
   */
  struct termios tty;
  if (tcgetattr (fd, &tty) != 0) {
    fprintf(stderr, "error %d from tcgetattr while setting up interface: %s\n", errno, strerror(errno));
    return -1;
  }

  cfsetospeed(&tty, speed);
  cfsetispeed(&tty, speed);

  tty.c_cflag = (tty.c_cflag & ~CSIZE) | CS8;
  tty.c_iflag &= ~IGNBRK;
  tty.c_lflag = 0;
  tty.c_oflag = 0;
  tty.c_cc[VMIN] = 0;
  tty.c_cc[VTIME] = 5;
  tty.c_iflag &= ~(IXON | IXOFF | IXANY);
  tty.c_cflag |= (CLOCAL | CREAD);
  tty.c_cflag &= ~(PARENB | PARODD);
  tty.c_cflag |= parity;
  tty.c_cflag &= ~CSTOPB;
  tty.c_cflag &= ~CRTSCTS;

  if (tcsetattr (fd, TCSANOW, &tty) != 0) {
    fprintf(stderr, "error %d from tcsetattr while setting up interface: %s\n", errno, strerror(errno));
    return -1;
  }
  return 0;
}

typedef struct Reader {
  uint64_t chunk_size;
  const char *serial, *file;
} Reader;

pthread_mutex_t serial_lock = PTHREAD_MUTEX_INITIALIZER;

static
void* reader(void* arg) {
  const Reader* conf = arg;
  ssize_t read_buf_s = conf->chunk_size
        , read_s;
  char read_buf[read_buf_s];

  int serial_fd = open(conf->serial, O_RDONLY | O_NOCTTY | O_SYNC);
  if (serial_fd < 0) {
    fprintf(stderr, "(Reader) error %d opening serial: %s\n", errno, strerror(errno));
    return NULL;
  }

  int file_fd = open(conf->file, O_WRONLY | O_CREAT | O_APPEND);
  if (serial_fd < 0) {
    fprintf(stderr, "(Reader) error %d opening file: %s\n", errno, strerror(errno));
    return NULL;
  }

  while (1) {
    pthread_mutex_lock(&serial_lock);
    if ((read_s = read(serial_fd, read_buf, read_buf_s)) < 0) {
      fprintf(stderr, "(Reader) error %d reading from serial: %s\n", errno, strerror(errno));
    }
    pthread_mutex_unlock(&serial_lock);

    if (write(file_fd, read_buf, read_s) < 0) {
      fprintf(stderr, "(Reader) error %d writing to file: %s\n", errno, strerror(errno));
    }
  }
}

int main(int argc, char *argv[]) {
  if (argc < 3 || 4 < argc) {
    fprintf(stderr, "Usage: %s [port] [file] [chunk_size?]\n", argv[0]);
    return 1;
  }

  pthread_t thread;
  Reader conf = (Reader) {
    .chunk_size = argc == 3? 16 : atoi(argv[3]),
    .serial= argv[1],
    .file = argv[2]
  };

  if (pthread_create(&thread, NULL, reader, &conf) < 0) {
    fprintf(stderr, "error %d creating reader thread: %s\n", errno, strerror(errno));
    return 1;
  }

  int serial_fd  = open(argv[1], O_WRONLY | O_NOCTTY | O_SYNC);
  if (serial_fd < 0) {
    fprintf(stderr, "error %d opening %s: %s\n", errno, argv[1], strerror(errno));
    return 1;
  }
  if (set_interface_attribs(serial_fd, B9600, 0) < 0) return 1;

  int file_fd = open(argv[2], O_RDONLY | O_CREAT, 0644);
  if (serial_fd < 0) {
    fprintf(stderr, "error %d opening %s: %s\n", errno, argv[2], strerror(errno));
    return 1;
  }

  struct termios stdin_settings;
  if (tcgetattr(STDIN_FILENO, &stdin_settings) != 0) {
    fprintf(stderr, "error %d from tcsetattr while setting up stdin: %s\n", errno, strerror(errno));
    return 1;
  }
  stdin_settings.c_lflag &= ~(ICANON | ECHO);
  if (tcsetattr(STDIN_FILENO, TCSANOW, &stdin_settings) != 0) {
    fprintf(stderr, "error %d from tcsetattr while setting up stdin: %s\n", errno, strerror(errno));
    return 1;
  }

  int64_t read_s;
  char read_buf[1024]
     , in;

  while (1) {
    if (read(STDIN_FILENO, &in, 1) > 0) switch (in) {
      case 'q':
        pthread_kill(thread, SIGKILL);
        close(serial_fd);
        close(file_fd);
        return 0;
      case 'c':
        printf("\033[H\033[J");
        fflush(stdout);
        break;
      case 's':
        printf("> ");
        for (read_s = 0; (in = getchar()) != '\n' && read_s < 1023; ++read_s) {
          putchar(in);
          read_buf[read_s] = in;
        }
        putchar('\n');
        pthread_mutex_lock(&serial_lock);
        if (write(serial_fd, read_buf, read_s) < 0) {
          fprintf(stderr, "error %d writting to serial: %s", errno, strerror(errno));
        }
        pthread_mutex_unlock(&serial_lock);
        break;
      case 'l':
        lseek(file_fd, 0, SEEK_SET);
        while ((read_s = read(file_fd, read_buf, 1024)) > 0) {
          write(STDOUT_FILENO, read_buf, read_s);
        } putchar('\n');
        fflush(stdout);
        if (read_s < 0) {
          fprintf(stderr, "error %d reading capture: %s\n", errno, strerror(errno));
        }
        break;
      case 'h':
      default:
        printf(
          "===============================================================================================================\n"
          "|| WeCan programa base: Controles                                                                            ||\n"
          "||                                                                                                           ||\n"
          "|| q: cierra este programa    s: manda un mensaje por la antena    l: enseña los contenidos de la captura    ||\n"
          "|| c: limpia la pantalla      h: muestra este mensaje                                                        ||\n"
          "===============================================================================================================\n"
          "\n"
        );
        break;
    }
  }
}
