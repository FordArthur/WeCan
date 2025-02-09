/** Solaris:
 * Descripción: Protocolo ISRP-FEC, satélite
 * Autor: Marcos Ávila Navas
 * Version 0.0.1 
 *         | | +-- Avances
 *         | +---- Cambios al diseño
 *         +------ Cambios al protocolo
 * 
 * Notas:
 * - La notación "IO ..." informa que tal función opera con input/output,
 *   el argumento dado a "IO" es lo que devuelve (o "()" si no devuelve nada)
 */

#define HANDSHAKE_TIMEOUT 250
#define CHUNK_TIMEOUT 500
#define ERR -1

typedef uint16_t Handshake;

typedef struct Chunk {
  uint16_t time;
  uint16_t temp;
  uint16_t pres;
  uint16_t monox;
} Chunk;

/** send_handshake : Handshake -> IO ()
 * manda el correspondiente handshake
 */
static inline
void send_handshake(const Handshake code) {
  // TODO
}

/** get_handshake : uint16_t -> IO Handshake
 * espera `timeout` ms por un handshake
 * si se rechaza el handshake devuelve ERR
 */
static inline
Handshake get_handshake(uint16_t timeout) {
  // TODO
}

/** get_chunk : uint16_t -> IO Chunk
 * recive y descodifica un Chunk, con timeout
 * si se detecta que la transmisión no es un Chunk devuelve ERR
 */
static inline
Chunk get_chunk(uint16_t timeout) {
  // TODO
}

void setup() {
  // TODO: inicializar pins
  Serial.begin(9600);
}

register Handshake get = 0;
register Handshake recieve;

void loop() {
  send_handshake(get);
  if ((recieve = get_handshake(HANDSHAKE_TIMEOUT)) != ERR) {
    for (; get < recieve; ++get) {
      Chunk ch = get_chunk(CHUNK_TIMEOUT);
      if (*(uint64_t*) &ch == ANY)
        break;
      Serial.print("elapsado: ");
      Serial.println(ch.time);

      Serial.print("temp: ");
      Serial.println(ch.temp);

      Serial.print("pres: ");
      Serial.println(ch.pres);

      Serial.print("concetración CO: ");
      Serial.println(ch.monox);
    }
  }
}
