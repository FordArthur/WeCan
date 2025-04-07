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

#define PIN_BUZZER 9

SoftwareSerial antena(10, 11); // RX = 10, TX = 11
MQ7 sensor_CO(A0, 5.0);        // Creamos el objeto sensor de CO
static long time = 0;          // Tiempo

float floor_height;
bool is_recovery = false;

#define ALTURA_DATOS 50.0
#define ALTURA_RECUP 20.0

typedef struct Chunk {
  uint32_t time;
  float temp;
  float pres;
  float altur;
  float monox;
} Chunk;

Adafruit_BMP280 bmp;

/** read_sensors : IO Chunk
 * Lee los sensores y los empaqueta
 */
static inline
Chunk read_sensors(void) {
  const float temp = bmp.readTemperature();
  const float pres = bmp.readPressure();
  const float altur = bmp.readAltitude();
  const float monox = sensor_CO.readPpm();
  return {time++, temp, pres, altur - floor_height, monox};
}

/** send_chunk : IO ()
 * Manda un `Chunk`
 */
static inline
void send_chunk(const Chunk chunk) {
  antena.write((const char*) &chunk, sizeof(chunk));
}

/** tono_recuperacion : IO ()
 * Reproduce una melodía para la recuperación
 */
static inline
void recuperation_tone(void) {
  tone(PIN_BUZZER,587);
  delay(500);
  tone(PIN_BUZZER,659);
  delay(500);
  tone(PIN_BUZZER,523);
  delay(500);
  tone(PIN_BUZZER,261);
  delay(500);
  tone(PIN_BUZZER,392);
  delay(800);
  noTone(PIN_BUZZER);
  delay(500);
}

void setup(void) {
  pinMode(PIN_BUZZER, OUTPUT);
  antena.begin(9600);

  while (!bmp.begin()) {
    tone(PIN_BUZZER, 320, 100);
    delay(200);
  }

  bmp.setSampling(
    Adafruit_BMP280::MODE_NORMAL,
    Adafruit_BMP280::SAMPLING_X2,
    Adafruit_BMP280::SAMPLING_X16,
    Adafruit_BMP280::FILTER_X16,
    Adafruit_BMP280::STANDBY_MS_500
  );

  floor_height = bmp.readAltitude();

  sensor_CO.calibrate();
  delay(5000);
}

void loop(void) {
  send_chunk(read_sensors());

  if (is_recovery && bmp.readAltitude() - floor_height < ALTURA_RECUP)  {
    send_chunk({NAN, NAN, NAN, NAN});
    while (1) recuperation_tone();
  } else if (bmp.readAltitude() - floor_height  > ALTURA_DATOS)
    is_recovery = true;
    
  delay(1000);
}
