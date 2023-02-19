/**   Arduino: Nano
  *   created by: Relry Pereira dos Santos
  */
// #define DEBUG_MODE true
#include <ArduUtil.h>
#include <KtaneModule.h>

#define PINS_ADDRESS 2

#define F_CODE_NAME F("module-wires")
#define F_VERSION F("v0.2.0-a1")

KtaneModule module = KtaneModule({PIN_ADDRESS_0, PIN_ADDRESS_1});

volatile bool newMessage = false;

Status lastStatusModule;
unsigned long previousMillis = 0;
int interval = 1000;

// blue, green, red, yellow, black, white
uint8_t pinsWires[] = { 6, 7, 8, 9, 10, 11 };
uint8_t pinsLeds[] = { A0, A1, A2, A3, 4, 5 };
volatile byte wiresInfo = 0x0;
volatile byte ledsInfo = 0x0;

typedef struct ORDER_WIRES {
  byte wire;
  struct ORDER_WIRES *nextOrderWire;
} OrderWires;

OrderWires *orderWires = NULL, *currentOrderWires;
unsigned long lastTimePrint = 0;

void setup() {
  Serial.begin(38400);

  randomSeed(analogRead(A7));

  for (int i = 0; i < sizeof(pinsWires); i++) {
    pinMode(pinsWires[i], INPUT);
  }

  for (int i = 0; i < sizeof(pinsLeds); i++) {
    pinMode(pinsLeds[i], OUTPUT);
    digitalWrite(pinsLeds[i], HIGH);
  }

  delay(1000);

  // module.setFuncModuleEnable();
  module.setFuncResetGame(resetGame);
  module.setFuncValidaResetModule(validaModuloReady);
  module.setFuncConfig(configWrite);
  module.setFuncStartGame(startGame);
  // module.setFuncStopGame();

  module.init(F_CODE_NAME, F_VERSION);
}

void loop() {
  // Verifica se houve mensagem nova recebida pelo barramento I2C
  if (newMessage) {
    switch (module.getStatus()) {
      case IN_GAME:
        Serial.println(F("Em jogo"));
        break;
      default:
        Serial.println(F("Status diferente."));
    }

    newMessage = false;
  }

  // verifica se houve mudança no status do modulo
  if (lastStatusModule != module.getStatus()) {
    Serial.println((String)F("STATUS anterior: ") + Status_name[lastStatusModule]);
    Serial.println((String)F("STATUS atual...: ") + Status_name[module.getStatus()]);

    switch (module.getStatus()) {
      case RESETING:
      case READY:
        break;
      case IN_GAME:
        Serial.println(F("Iniciando o jogo e os leds."));
        for (byte bit = 0; bit < 6; bit++) {
          bool valorBit = bitRead(ledsInfo, bit);
          digitalWrite(pinsLeds[bit], valorBit);
        }
        Serial.print(F("Valor: ")); Serial.println(ledsInfo, BIN);
        break;
      case DEFUSED:
        Serial.println(F("Modulo DEFUSADO!"));
        break;
    }

    lastStatusModule = module.getStatus();
  }

  switch (module.getStatus()) {
    case IN_GAME:
      executeInGame();
      break;
      // default:
      //   Serial.println("module.getStatus(): " + Status_name[module.getStatus()]);
  }

  unsigned long currentTimePrint = millis();
  if (currentTimePrint - lastTimePrint > 2000) {
    Serial.println((String)F("Memory free: ") + freeMemory());
    lastTimePrint = currentTimePrint;
  }
}

void executeInGame() {
  unsigned long currentMillis = millis();

  if (currentMillis - previousMillis >= interval) {
    int i = 0;
    for (uint8_t pin : pinsWires) {
      int value = digitalRead(pin);
      // Serial.println((String)F("Fio do pino: ") + pin + (String)F(", com valor: ") + value);
      bool dif = ((wiresInfo >> i) & 0b0001) ^ value;
      if (dif) {
        wiresInfo ^= (0b0001 << i);
        if (validaRemovedWire(i)) {
          Serial.println(F("Fio removido corretamente"));  
        } else {
          module.addFault((String)F("Fio errado: ") + (String)(i + 1));
        }
      }
      i++;
    }

    if (orderWires == NULL) {
      module.defused();
    }
    // Serial.print(F("wiresInfo: 0b"));Serial.println(wiresInfo, BIN);
    // printSerialTracos();

    Serial.println((String)F("Memory free: ") + freeMemory());
    
    previousMillis = currentMillis;
  }
}

bool validaRemovedWire(byte numWire) {
  currentOrderWires = orderWires;
  
  if (currentOrderWires != NULL) {
    if (currentOrderWires->wire == numWire) {    
      orderWires = orderWires->nextOrderWire;
      free(currentOrderWires);
      return true;
    }
  }

  return false;
}

void resetGame() {
  currentOrderWires = orderWires;
  wiresInfo = 0x0;
  ledsInfo = 0x0;

  for (byte pinLed : pinsLeds) {
    digitalWrite(pinLed, HIGH);
  }

  for (byte bit = 0; bit < 6; bit++) {
    bool bitValue = random(2) % 2;
    bitWrite(ledsInfo, bit, bitValue);
  }
  Serial.print("Valor: "); Serial.println(ledsInfo, BIN);

  while (currentOrderWires != NULL) {
    orderWires = orderWires->nextOrderWire;
    Serial.println((String)F("Limpando orderWire: ") + currentOrderWires->wire);
    free(currentOrderWires);
    currentOrderWires = orderWires;
  }

  Serial.println(F("Criando nova ordem de desarme dos fios."));
  byte numWires = random(2,7); // 2 to 6
  Serial.println((String)F("numWires: ") + (String)numWires);
  for (byte i = 0; i < numWires; i++) {
    byte wireRandom = random(6);
    if (orderWires == NULL) {
      orderWires = currentOrderWires = (OrderWires*) malloc(sizeof(OrderWires));
    } else {
      currentOrderWires->nextOrderWire = (OrderWires*) malloc(sizeof(OrderWires));
      currentOrderWires = currentOrderWires->nextOrderWire;
    }
    currentOrderWires->wire = wireRandom;
    currentOrderWires->nextOrderWire = NULL;
  }
  
  currentOrderWires = orderWires;
  byte i = 0;
  while(currentOrderWires != NULL) {
    Serial.println((String)F("ordem [") + i + (String)F("] numero do fio: ") + currentOrderWires->wire);
    currentOrderWires = currentOrderWires->nextOrderWire;
    i++;
  }

  delay(300);
  for (byte pinLed : pinsLeds) {
    digitalWrite(pinLed, LOW);
  }

  Serial.println(F("Reset realizado com sucesso!"));
}

bool validaModuloReady() {
  Serial.println(F("Validando se modulo esta pronto"));
  delay(500);
  if (validaFiosConectados()) {
    Serial.println(F("Modulo pronto para iniciar."));
    return true;
  }
  Serial.println(F("Modulo NAO esta pronto para iniciar."));
  return false;
}

bool validaFiosConectados() {
  Serial.println(F("Validando fios conectados"));
  int i = wiresInfo = 0;
  for (uint8_t pin : pinsWires) {
    int value = digitalRead(pin);
    Serial.println((String)F("Fio do pino: ") + pin + (String)F(", com valor: ") + value);
    wiresInfo |= (value << i++);
  }

  Serial.print(F("Valor final dos pinos: 0b"));
  Serial.println(wiresInfo, BIN);
  return (wiresInfo == 0);
}

void startGame() {
  Serial.println(F("startGame chamado."));
  if (!newMessage) {
    newMessage = true;
    Serial.println(F("Nova mensagem"));
  } else {
    Serial.println(F("WARNING!!! Mensagem ignorada"));
  }
}

void configWrite(uint8_t preset) {
  Serial.println(F("Config modulo"));
  if (Wire.available()) {
    Serial.println((String)F("preset: ") + ((String)(char)preset));
    if ((char)preset == 'c') {
      Serial.println(F("Configurando modulo."));
      int numBytes = Wire.available();
      char arrayChars[numBytes + 1];
      for (int i = 0; i < numBytes; i++) {
        arrayChars[i] = Wire.read();
      }
      arrayChars[numBytes] = '\0';
      String message = String(arrayChars);
      // String message = Wire.readStringUntil(';');
      Serial.println((String)F("Recebido do master: '") + message + (String)F("'"));
    }
  }
  Serial.println(F("Resetando modulo, pos config."));
  resetGame();
}

void printSerialTracos() {
  Serial.println(F("-------------------------"));
}
