#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <stdbool.h>
#include <wiringPi.h>
#include <lcd.h>      // Módulo de LCD da WiringPi
#include <pcf8574.h>  // Módulo para o expansor I2C do LCD

// ---- CONFIGURAÇÃO DO LCD I2C ----
#define PCF_BASE 100          // Base de pinos virtuais para o PCF8574
#define I2C_ADDR 0x27         // Endereço I2C comum do LCD (mude para 0x3F se necessário)
// Mapeamento interno do adaptador PCF8574 para o LCD 16x2
#define LCD_RS (PCF_BASE + 0)
#define LCD_RW (PCF_BASE + 1)
#define LCD_EN (PCF_BASE + 2)
#define LCD_BL (PCF_BASE + 3) // Backlight
#define LCD_D4 (PCF_BASE + 4)
#define LCD_D5 (PCF_BASE + 5)
#define LCD_D6 (PCF_BASE + 6)
#define LCD_D7 (PCF_BASE + 7)

// ---- CONFIGURAÇÃO DO TECLADO MATRICIAL (GPIO WiringPi) ----
int rowPins[4] = {4, 5, 6, 7};   // Linhas (Saídas) -> Pinos físicos correspondentes
int colPins[4] = {21, 22, 23, 24}; // Colunas (Entradas com Pull-Down)

char keys[4][4] = {
  {'1', '2', '3', '+'},
  {'4', '5', '6', '-'},
  {'7', '8', '9', '*'},
  {'C', '0', '=', '/'} // 'C' limpa, '=' calcula
};

// ---- FUNÇÕES ARITMÉTICAS ORIGINAIS ----
int multiply(int a, int b) {
  int result = 0; int sign = 1;
  if (b < 0) { sign = -1; b = -b; } 
  for (int i = 0; i < b; i++) result += a; 
  return result * sign;
} 

int factorial(int n) {
  if (n <= 1) return 1; 
  int result = 1;
  for (int i = 2; i <= n; i++) result *= i; 
  return result;
}

int divide(int a, int b) {
  if (b == 0) return -999; 
  return a / b; 
}

// ---- LÓGICA DO TECLADO MATRICIAL ----
void setupKeyboard() {
  for (int i = 0; i < 4; i++) {
    pinMode(rowPins[i], OUTPUT);
    digitalWrite(rowPins[i], LOW);
    pinMode(colPins[i], INPUT);
    pullUpDnControl(colPins[i], PUD_DOWN); // Garante nível LOW quando solto
  }
}

char getKey() {
  for (int r = 0; r < 4; r++) {
    digitalWrite(rowPins[r], HIGH); // Ativa a linha atual
    delay(5); // Debounce estabilização
    for (int c = 0; c < 4; c++) {
      if (digitalRead(colPins[c]) == HIGH) {
        while (digitalRead(colPins[c]) == HIGH); // Aguarda soltar a tecla
        digitalWrite(rowPins[r], LOW);
        return keys[r][c];
      }
    }
    digitalWrite(rowPins[r], LOW); // Desativa linha
  }
  return '\0'; // Nenhuma tecla pressionada
}

// ---- FUNÇÃO PRINCIPAL (MÁQUINA DE ESTADOS) ----
int main() {
  if (wiringPiSetup() == -1) return 1;

  setupKeyboard();

  // Inicializa o expansor I2C PCF8574
  pcf8574Setup(PCF_BASE, I2C_ADDR);
  
  // Inicializa o LCD (2 linhas, 16 colunas, modo 4 bits através do PCF8574)
  int lcdHandle = lcdInit(2, 16, 4, LCD_RS, LCD_EN, LCD_D4, LCD_D5, LCD_D6, LCD_D7, 0, 0, 0, 0);
  digitalWrite(LCD_BL, HIGH); // Liga a luz de fundo
  
  lcdClear(lcdHandle);
  lcdPosition(lcdHandle, 0, 0);
  lcdPrintf(lcdHandle, "Calc 4 Bits Rasp");

  char bufferA[10] = "";
  char bufferB[10] = "";
  char op = '\0';
  bool preenchendoA = true;

  while (1) {
    char key = getKey();
    if (key != '\0') {
      
      // Se pressionar 'C', reinicia a calculadora
      if (key == 'C') {
        bufferA[0] = '\0'; bufferB[0] = '\0';
        op = '\0'; preenchendoA = true;
        lcdClear(lcdHandle);
        lcdPrintf(lcdHandle, "A: ");
        continue;
      }

      // Identifica se é operador
      if (key == '+' || key == '-' || key == '*' || key == '/') {
        if (preenchendoA && strlen(bufferA) > 0) {
          op = key;
          preenchendoA = false;
          lcdClear(lcdHandle);
          lcdPosition(lcdHandle, 0, 0);
          lcdPrintf(lcdHandle, "B: ");
        }
        continue;
      }

      // Executa o cálculo ao pressionar '='
      if (key == '=') {
        if (strlen(bufferA) > 0 && strlen(bufferB) > 0 && op != '\0') {
          int valA = atoi(bufferA) & 0x0F; // Força limite de 4 bits (0-15)
          int valB = atoi(bufferB) & 0x0F;
          int resultado = 0;
          bool div_zero = false;

          struct timespec t_start, t_end;
          clock_gettime(CLOCK_MONOTONIC, &t_start);

          if (op == '+') resultado = valA + valB;
          else if (op == '-') resultado = valA - valB;
          else if (op == '*') resultado = multiply(valA, valB);
          else if (op == '/') {
            resultado = divide(valA, valB);
            if (resultado == -999) div_zero = true; 
          }

          clock_gettime(CLOCK_MONOTONIC, &t_end);
          long long tempo = (t_end.tv_nsec - t_start.tv_nsec) / 1000LL; // em µs

          lcdClear(lcdHandle);
          if (div_zero) {
            lcdPosition(lcdHandle, 0, 0);
            lcdPrintf(lcdHandle, "Erro: Div por 0");
          } else {
            // Mostra o resultado em Decimal e Binário (4 bits mais baixos)
            int bits4 = resultado & 0x0F;
            lcdPosition(lcdHandle, 0, 0);
            lcdPrintf(lcdHandle, "Res: %d  T:%lldus", resultado, tempo);
            lcdPosition(lcdHandle, 1, 0);
            lcdPrintf(lcdHandle, "Bin: %c%c%c%c", 
                      (bits4 & 8)?'1':'0', (bits4 & 4)?'1':'0', 
                      (bits4 & 2)?'1':'0', (bits4 & 1)?'1':'0');
          }
        }
        continue;
      }

      // Adiciona os números digitados nos buffers correspondentes
      if (preenchendoA) {
        if (strlen(bufferA) < 4) { // Limita entrada de caracteres
          strncat(bufferA, &key, 1);
          lcdPosition(lcdHandle, 0, 3);
          lcdPrintf(lcdHandle, "%s", bufferA);
        }
      } else {
        if (strlen(bufferB) < 4) {
          strncat(bufferB, &key, 1);
          lcdPosition(lcdHandle, 0, 3);
          lcdPrintf(lcdHandle, "%s", bufferB);
        }
      }
    }
    delay(50); // Evita sobrecarga de CPU no loop
  }
  return 0;
}
