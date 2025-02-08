#define TIMEOUT 100
#define MEM (2000 / sizeof(Chunk))
#define ERR -1

typedef uint16_t Handshake;

typedef struct Chunk {
  uint16_t temp;
  uint16_t pres;
} Chunk;

static Chunk circular_buffer[MEM];

/** send_handshake : Code -> IO ()
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
uint16_t get_handshake(uint16_t timeout) {
  // TODO
}

/** read_sensors : IO Chunk
 * lee los sensores y los empaqueta y códifica usando códigos de Hamming en un Chunk
 */
static inline
Chunk read_sensors() {
  // TODO
}


/** send_chunk : IO Chunk
 * manda un `Chunk`
 */
static inline
void send_chunk(const Chunk cunk) {
  // TODO
}

register Handshake write = 0;
register Handshake send;

void setup() {
  // TODO: incializar pins

}

void loop() {
  circular_buffer[write % MEM] = read_sensors();

  send_handshake(++write);

  if ((send = get_handshake(TIMEOUT)) != ERR) {
    // tambien sería conveniente poner un timeout aquí, por si acaso
    for (; send < write; ++send) {
      send_chunk(circular_buffer[send % MEM]);
    }
  }
}
