/** Solaris:
 * Descripción: Código satélite
 * Autor: Marcos Ávila Navas
 * Version 2.0.2
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

#define TIMEOUT 100 // Timeout del handshake en ms
#define MEM 100     // Memoria que reservamos para el buffer circular
#define ERR -1      // Reservamos -1 como símbolo de error

// Es CRUCIAL Handshake no tenga signo (para que el shift sea lógico y no aritmético)
typedef uint16_t Handshake;

typedef struct Chunk {
  uint16_t time;
  float temp;
  float pres;
  float monox;
} Chunk;

Adafruit_BMP280 bmp;
static long time = 0;
static Chunk circular_buffer[MEM];

Handshake write = 0;
Handshake send;

/** send_handshake : Handshake -> IO ()
 * Manda el correspondiente handshake
 */
static inline
void send_handshake(const Handshake code) {
  antena.write((const char*) &code, sizeof(code));
}

/** get_handshake : uint16_t -> IO Handshake
 * Espera `timeout` ms por un handshake
 * si se rechaza el handshake devuelve ERR
 */
static inline
Handshake get_handshake(long timeout) {
  Handshake handshake;
  for (; timeout; --timeout) 
    if ((handshake = antena.read()) != ERR) {
      handshake |= antena.read() << 8;
      return handshake;
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
Chunk read_sensors() {
  const float temp = bmp.readTemperature();
  const float pres = bmp.readPressure();
  const float monox /* = ... */;
  return encode({write, temp, pres, monox});
}

/** send_chunk : IO ()
 * Manda un `Chunk`
 */
static inline
void send_chunk(const Chunk chunk) {
  antena.write((const char*) &chunk, sizeof(chunk));
}

void setup() {
  antena.begin(9600);
  bmp.setSampling(
    Adafruit_BMP280::MODE_NORMAL,
    Adafruit_BMP280::SAMPLING_X2,
    Adafruit_BMP280::SAMPLING_X16,
    Adafruit_BMP280::FILTER_X16,
    Adafruit_BMP280::STANDBY_MS_500
  );
  pinMode(9, OUTPUT);
  /*
  sensor_CO.calibrate();
  delay(5000);
  */
}

void loop() {
  circular_buffer[write % MEM] = read_sensors();

  send_handshake(++write);

  if ((send = get_handshake(TIMEOUT)) != ERR) {
    for (; send < write; ++send) {
      send_chunk(circular_buffer[send % MEM]);
    }
  }
  delay(1000);

}
