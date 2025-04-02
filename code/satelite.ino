/** WeCan:
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
static Chunk circular_buffer[MEM];

uint32_t write = 0;
uint32_t send;

/** send_handshake : uint32_t -> IO ()
 * Manda el correspondiente handshake
 */
static inline
void send_handshake(const uint32_t code) {
  const char hcode[sizeof(Chunk)];
  memset(hcode, NAN, sizeof(hcode));
  memcpy(hcode, &code, sizeof(code));
  antena.write(hcode, sizeof(hcode));
}

/** get_handshake : IO Handshake
 * Espera `timeout` ms por un handshake
 * si se rechaza el handshake devuelve ERR
 */
static inline
uint32_t get_handshake(void) {
  char handshake[sizeof(Chunk)];
  timeout = TIMEOUT;
  for (size_t rd = 0; timeout; --timeout) {
    if ((rd += antena.readBytes(rd + (char*) &handshake, sizeof(handshake) - rd)) >= sizeof(Chunk)) {
      return ((uint32_t*) handshake)[0];
    }
    delay(1);
  }
  return ERR;
}

/** encode : Chunk -> Chunk
 * Codifica un Chunk con códigos hamming
 */
static inline
Chunk encode(Chunk chunk) {
  /* TODO: La implementación requiere más pruebas para ver donde sería óptimo recortar precisión en el chunk */
  return chunk;
}

/** read_sensors : IO Chunk
 * Lee los sensores y los empaqueta y códifica usando códigos de Hamming en un Chunk
 */
static inline
Chunk read_sensors(void) {
  const float temp = bmp.readTemperature();
  const float pres = bmp.readPressure();
  const float monox /* = sensor_CO.readPpm() */;
  return encode({write, temp, pres, monox});
}

/** send_chunk : IO ()
 * Manda un `Chunk`
 */
static inline
void send_chunk(const Chunk chunk) {
  antena.write((const char*) &chunk, sizeof(chunk));
}

void setup(void) {

  Serial.begin(9600);

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
  circular_buffer[write % MEM] = read_sensors();
  send_handshake(++write);

  if (get_handshake() != ERR) {
    for (; send < write; ++send) {
      send_chunk(circular_buffer[send % MEM]);
      delay(5);
    }
  }

  delay(1000 - timeout - 5*(send));
}
