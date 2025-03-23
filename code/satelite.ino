/** Solaris:
 * Descripción: Código satélite
 * Autor: Marcos Ávila Navas
 * Version 1.0.1
 *         | | +-- Avances
 *         | +---- Cambios al diseño
 *         +------ Cambios al protocolo
 * 
 * Notas:
 * - La notación "IO ..." informa que tal función opera con input/output,
 *   el argumento dado a "IO" es lo que devuelve (o "()" si no devuelve nada)
 */

/* La siguiente secuencia permite elegir entre un programa en el que la antena
 * utilice el puerto serial predeterminado y uno configurado desde software
 */

#include <Adafruit_BMP280.h>

#undef SSerial // Cambiese entre #undef y #define

#ifdef SSerial

#include <Software_Serial.h>

SoftwareSerial _antena(10, 11); // RX = 10, TX = 11

/* Para ser agnosticos en cuanto a cual se eligió, definimos e utilizamos "TheSerial"
 * desde este punto para referirnos al serial del programa
 */
#define TheSerial _antena

#else

#define TheSerial Serial // Misma idea

#endif

#define TIMEOUT 100 // Timeout del handshake en ms
#define MEM 110     // Memoria que reservamos para el buffer circular
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

/** send_handshake : Handshake -> IO ()
 * Manda el correspondiente handshake
 */
static inline
void send_handshake(const Handshake code) {
  // Mandamos el primer byte ...
  TheSerial.write(code & 0xff);
  // ... y luego el segundo
  TheSerial.write(code >> 8);
}

/** get_handshake : uint16_t -> IO Handshake
 * Espera `timeout` ms por un handshake
 * si se rechaza el handshake devuelve ERR
 */
static inline
Handshake get_handshake(long timeout) {
  Handshake handshake;
  for (; timeout; --timeout) 
    if ((handshake = TheSerial.read()) != ERR)
      return handshake;
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
  return encode({time, temp, pres, monox});
}

/** send_float : IO ()
 * Manda un float
 */
static inline
void send_float(const float x) {
  TheSerial.write(*(const int*)(&x) & 0xff);
  TheSerial.write((*(const int*)(&x) & 0xff00) >> 8);
  TheSerial.write((*(const int*)(&x) & 0xff0000) >> 16);
  TheSerial.write((*(const int*)(&x) & 0xff000000) >> 24);
}

/** send_chunk : IO ()
 * Manda un `Chunk`
 */
static inline
void send_chunk(const Chunk chunk) {
  TheSerial.write(chunk.time & 0xff);
  TheSerial.write(chunk.time >> 8);
  send_float(chunk.temp);
  send_float(chunk.pres);
  send_float(chunk.monox);
}

Handshake write = 0;
Handshake send;

void setup() {
  TheSerial.begin(9600);
  bmp.setSampling(
    Adafruit_BMP280::MODE_NORMAL,
    Adafruit_BMP280::SAMPLING_X2,
    Adafruit_BMP280::SAMPLING_X16,
    Adafruit_BMP280::FILTER_X16,
    Adafruit_BMP280::STANDBY_MS_500
  );
}

void loop() {
  ++time;
  circular_buffer[write % MEM] = read_sensors();

  send_handshake(++write);

  if ((send = get_handshake(TIMEOUT)) != ERR) {
    for (; send < write; ++send) {
      send_chunk(circular_buffer[send % MEM]);
    }
  }
  // Nótese como se hace de manera asíncrona
}
