// Incluindo bibliotecas necessárias para o funcionamento do programa

#include "display.cpp";
#include "rele.cpp";
#include "sensor_de_luz.cpp";
#include "sensor_de_temp_umi.cpp";
#include "sensor_umi_solo.cpp";
#include <Bounce2.h>

// Criando variáveis para interrupção

int interruptPin = 2;
boolean estadoPararTudo = false;
const int ledSinalizador = 13;

// Definições gerais

#define PIN_BOMBA 10
#define NIVEL_LOGICO_RELE_BOMBA LOW
#define TEMPO_BOMBA_LIGADA 20 // Tempo em segundos

#define PIN_SOLENOIDE1 8 // Solenoide da caixa
#define NIVEL_LOGICO_RELE_SOLENOIDE1 LOW

#define PIN_SOLENOIDE2 9 // Solenoide das plantas
#define NIVEL_LOGICO_RELE_SOLENOIDE2 LOW

#define DHTPIN 5
#define DHTTYPE DHT11 // DHT 11

#define SENSOR_UMI_SOLO1 A1
#define SENSOR_UMI_SOLO2 A2
#define SENSOR_UMI_SOLO3 A3
#define SMS_VCC 7
#define SMS_GND 6

#define SENSOR_LUZ A0
#define NOITE 40

#define SOLO_SECO 40
#define SOLO_UMIDO 60

//#define PIN_BOTAO_LER_SENSORES 12 // Modo interrupção
//#define PIN_BOTAO_BOMBA 9

// Instanciando objetos

Rele bomba(PIN_BOMBA, NIVEL_LOGICO_RELE_BOMBA); // Instanciando o rele da bomba
Rele solenoide_caixa(PIN_SOLENOIDE1, NIVEL_LOGICO_RELE_SOLENOIDE1);
Rele solenoide_plantas(PIN_SOLENOIDE2, NIVEL_LOGICO_RELE_SOLENOIDE2);

DHT dhtParam(DHTPIN, DHTTYPE);
TempUmi dht(&dhtParam); // Passando sensor DHT como parâmetro para o nosso objeto
float temp, umi; // Variáveis para guardar temperatura e umidade

LiquidCrystal_I2C lcdParam(0x3F, 16, 2); // Instanciando Display LCD
Display lcd(&lcdParam); // Passando como parâmetro o objeto lcdParam no objeto lcd

Ldr ldr1(SENSOR_LUZ);

Solo solo1(SENSOR_UMI_SOLO1, SMS_VCC, SMS_GND);
Solo solo2(SENSOR_UMI_SOLO2, SMS_VCC, SMS_GND);
Solo solo3(SENSOR_UMI_SOLO3, SMS_VCC, SMS_GND);

Bounce debouncerInterrupaoGeral = Bounce();

int mediaUmidade = 50;
boolean estadoAnterior = false;

// cabeçalho das funções criadas neste arquivo

void calcularTempo();
void liberarSolucao();
int lerSensores();
void agitarSolucao();
void lerBotoes();
void showDisplay();
void pararTudo();

void setup() {
  lcd.iniciar();
  lcd.imprimir("Iniciando...");
  dht.iniciar();
  pinMode(SMS_VCC, OUTPUT);
  pinMode(SMS_GND, OUTPUT); 
  pinMode(SENSOR_UMI_SOLO1, INPUT);
  pinMode(SENSOR_UMI_SOLO2, INPUT);
  pinMode(SENSOR_UMI_SOLO3, INPUT);
  pinMode(PIN_BOMBA, OUTPUT);
  pinMode(PIN_SOLENOIDE1, OUTPUT);
  pinMode(PIN_SOLENOIDE2, OUTPUT);
  pinMode(interruptPin, INPUT_PULLUP);
  pinMode(ledSinalizador, OUTPUT);
  attachInterrupt(digitalPinToInterrupt(interruptPin), pararTudo, FALLING);
  debouncerInterrupaoGeral.attach(interruptPin);
  debouncerInterrupaoGeral.interval(5);
  Serial.begin(9600);
  delay(1000);
}

void loop() {
  // Aqui só implementa a lógica do sistema
  showDisplay();
  delay(100);
  lerSensores();
  //lerBotoes();
  mediaUmidade = (solo1.getValor() + solo2.getValor() + solo3.getValor()) / 3;
  delay(100);
  if (mediaUmidade < SOLO_SECO && ldr1.getValor() > NOITE) { // SOLO_SECO = 40 e NOITE = 40
    agitarSolucao();
    delay(100);
    liberarSolucao();
  }
  else {
    showDisplay();
    delay(10000); // Tempo de espera para voltar ao inicio do loop
  }
  delay(50);
  //estadoAnterior = !estadoAnterior;
}

void pararTudo() {
  estadoPararTudo = !estadoPararTudo;
  for (int i = 0; i < 100; i++) {
    digitalWrite(ledSinalizador, HIGH);
  }
  digitalWrite(ledSinalizador, LOW);
  Serial.println(estadoPararTudo);
}

void showDisplay() {
  lcd.apagarTudo();
  lcd.imprimir("TA:   C UA:   %", 0, 0);
  lcd.imprimir((int)dht.getTemp(), 4, 0);
  lcd.imprimir((int)dht.getUmi(), 12, 0);
  lcd.imprimir("MUS:   % L:   %", 0, 1);
  lcd.imprimir(mediaUmidade, 5, 1);
  lcd.imprimir(ldr1.getValor(), 12, 1);
}

void liberarSolucao() {
    int tempoBombaLigada = 0;
    int tempMediaUmidadeSolo = 50;
    int quantRepeticoes = 0;
    bomba.desligar();
    delay(500);
    while (true) { // mediaUmidade < SOLO_UMIDO
      if (estadoPararTudo) {
        estadoPararTudo = false;
        break;
      }
      lcd.imprimir("liberando sol", 0, 0);
      solenoide_plantas.ligar();
      delay(1000);
      bomba.ligar();
      lcd.imprimir("B: 1 C: 0 P: 1", 0, 1);
      if (tempoBombaLigada >= TEMPO_BOMBA_LIGADA) {
        //lcd.imprimir("TBOMBA >= TTEMPO");
        int count = 0;
        int mediaAnterior = 0;
        if (estadoPararTudo) {
          estadoPararTudo = false;
          break;
        }
        bomba.desligar();
        delay(1000);
        solenoide_plantas.desligar();
        delay(500);
        quantRepeticoes++;
        while (count < 3) {
          lcd.imprimir("Lendo sensores..");
          lcd.imprimir(count, 0, 1);
          solo1.ler();
          solo2.ler();
          solo3.ler();
          mediaUmidade = (solo1.getValor() + solo2.getValor() + solo3.getValor()) / 3;
          mediaAnterior += mediaUmidade;
          ++count;
        }
        tempMediaUmidadeSolo = mediaAnterior / 3;
        if (tempMediaUmidadeSolo < (SOLO_SECO + SOLO_UMIDO) / 2) {
          //lcd.imprimir("TSOLO < MEDIA");
          tempoBombaLigada = 0;
          if (quantRepeticoes >= 3) {
            lcd.imprimir("ERRO LEITURA");
            delay(15000);
            return;
          }
          continue;
        }
        else if (tempMediaUmidadeSolo >= SOLO_UMIDO) {
          lcd.imprimir("Substr. umido!");
          break;
        }
      }
      tempoBombaLigada++;
      delay(1000);
    }
    bomba.desligar();
    delay(1000);
    solenoide_plantas.desligar();
    lcd.imprimir("Irrigado!");
    lcd.imprimir("B: 0 C: 0 P: 0", 0, 1);
    delay(2000);
}

void agitarSolucao() {
  lcd.imprimir("Agitando sol.");
  lcd.imprimir("B: 0 C: 1 P: 0", 0, 1);
  delay(1000);
  solenoide_plantas.desligar();
  solenoide_caixa.ligar();
  delay(1000);
  bomba.ligar();
  lcd.imprimir("B: 1 C: 1 P: 0", 0, 1);
  delay(1000);
  unsigned long tempo = 0;
  while (tempo != 180) {
    tempo++;
    delay(1000);
    if (estadoPararTudo) {
      estadoPararTudo = false;
      break;
    }
  }
  bomba.desligar();
  delay(1000);
  solenoide_caixa.desligar();
  lcd.imprimir("agitado!");
  lcd.imprimir("B: 0 C: 0 P: 0", 0, 1);
  delay(2000);
}

int lerSensores() {
  int error = 0;
  solo1.ler();
  solo2.ler();
  solo3.ler();
  ldr1.ler();
  error = dht.ler();
  return 0;
}

