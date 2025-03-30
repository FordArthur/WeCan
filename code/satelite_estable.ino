/** Solaris:
 * Descripción: Código satélite
 * Autor: Marcos Ávila Navas
 * Version 2.1.0
 *         | | +-- Avances
 *         | +---- Cambios al diseño
 *         +------ Cambios al protocolo
 * 
 * Notas:
 * - La notación "IO ..." informa que tal función opera con input/output,
 *   el argumento dado a "IO" es lo que devuelve (o "()" si no devuelve nada)
 */

#include <Adafruit_BMP280.h>
#include <SoftwareSerial.h>
#include <MQ7.h>

SoftwareSerial antena(10, 11); // RX = 10, TX = 11
MQ7 sensor_CO(A0, 5.0);        // Creamos el objeto sensor de CO
static long timeout;           // Tiempo esperado en get_handshake

#define TIMEOUT 100 // Timeout del handshake en ms
#define MEM 16      // Memoria que reservamos para el buffer circular
#define ERR -1      // -1 será nuestro símbolo error

typedef struct Chunk {
  uint32_t time;
  float temp;
  float pres;
  float altur;
  float monox;
} Chunk;

Adafruit_BMP280 bmp;

/** read_sensors : IO Chunk
 * Lee los sensores y los empaqueta y códifica usando códigos de Hamming en un Chunk
 */
static inline
Chunk read_sensors(void) {
  const float temp = bmp.readTemperature();
  const float pres = bmp.readPressure();
  const float monox /* = sensor_CO.readPpm() */;
  return{write, temp, pres, monox};
}

/** send_chunk : IO ()
 * Manda un `Chunk`
 */
static inline
void send_chunk(const Chunk chunk) {
  antena.write((const char*) &chunk, sizeof(chunk));
}

void setup(void) {
  pinMode(9, OUTPUT);
  antena.begin(9600);

  while (!bmp.begin()) {
    tone(9, 320, 100);
    delay(200);
  }

  bmp.setSampling(
    Adafruit_BMP280::MODE_NORMAL,
    Adafruit_BMP280::SAMPLING_X2,
    Adafruit_BMP280::SAMPLING_X16,
    Adafruit_BMP280::FILTER_X16,
    Adafruit_BMP280::STANDBY_MS_500
  );

  /*
  sensor_CO.calibrate();
  delay(5000);
  */
}

void loop(void) {
  send_chunk(read_sensors());
}
