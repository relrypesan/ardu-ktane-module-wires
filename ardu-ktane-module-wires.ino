/**   Arduino: Nano
  *   created by: Relry Pereira dos Santos
  */
// #define DEBUG_MODE true
#include <KtaneModule.h>

#define F_CODE_NAME F("module-wires")
#define F_VERSION F("v0.1.0-a1")

KtaneModule module;

volatile bool newMessage = false;

Status lastStatusModule;
unsigned long previousMillis = 0;
int interval = 1000;

uint8_t pinsWires[] = { 6, 7, 8, 9, 10, 11 };
volatile byte wiresInfo = 0x0;

void setup() {
  Serial.begin(38400);

  for (int i = 0; i < sizeof(pinsWires); i++) {
    pinMode(pinsWires[i], INPUT_PULLUP);
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
#ifdef DEBUG_MODE
  delay(1000);

  printSerialTracos();
  for (int i = 0; i < sizeof(pinsWires); i++) {
    Serial.println("pinsWires[" + (String)i + "]: " + digitalRead(pinsWires[i]));
  }
  printSerialTracos();
#endif

  // Verifica se houve mensagem nova recebida pelo barramento I2C
  if (newMessage) {
    switch (module.getRegModule()->status) {
      case IN_GAME:
        Serial.println(F("Em jogo"));
        break;
      default:
        Serial.println(F("Status diferente."));
    }

    newMessage = false;
  }

  // module.addFault("teste");

  // verifica se houve mudança no status do modulo
  if (lastStatusModule != module.getRegModule()->status) {
    Serial.println((String)F("STATUS anterior: ") + Status_name[lastStatusModule]);
    Serial.println((String)F("STATUS atual...: ") + Status_name[module.getRegModule()->status]);

    switch (module.getRegModule()->status) {
      case IN_GAME:
        Serial.println(F("Iniciando o jogo e o contador."));
        break;
      case RESETING:
        break;
    }

    lastStatusModule = module.getRegModule()->status;
  }

  switch (module.getRegModule()->status) {
    case IN_GAME:
      executeInGame();
      break;
      // default:
      //   Serial.println("module.getRegModule()->status: " + Status_name[module.getRegModule()->status]);
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
        Serial.println(F("Valores diferentes"));
        wiresInfo ^= (0b0001 << i);
        module.addFault((String)F("Fio retirado errado: ") + (String)(i + 1));
      }
      i++;
    }
    // Serial.print(F("wiresInfo: 0b"));Serial.println(wiresInfo, BIN);
    // printSerialTracos();
    
    previousMillis = currentMillis;
  }
}

void resetGame() {
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
