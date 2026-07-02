#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <stdbool.h>
#include <gpiod.h> // Biblioteca moderna para GPIO no Linux

// Mapeamento de pinos GPIO (Numeração BCM oficial do Raspberry Pi)
#define LED_BIT0 17 // Pino físico 11
#define LED_BIT1 18 // Pino físico 12
#define LED_BIT2 27 // Pino físico 13
#define LED_BIT3 22 // Pino físico 15

// Variáveis globais para o controlo dos pinos
struct gpiod_chip *chip;
struct gpiod_line *line0, *line1, *line2, *line3;

// --- Funções Aritméticas ---
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

// --- Controlo de LEDs via libgpiod ---
void setLeds(int valor) {
  int v = valor & 0x0F;
  gpiod_line_set_value(line0, (v >> 0) & 1);
  gpiod_line_set_value(line1, (v >> 1) & 1);
  gpiod_line_set_value(line2, (v >> 2) & 1);
  gpiod_line_set_value(line3, (v >> 3) & 1);
}

void printBinary16(int valor, char* buffer) {
  for (int i = 15; i >= 0; i--) {
    buffer[15 - i] = ((valor >> i) & 1) ? '1' : '0';
  }
  buffer[16] = '\0';
}

int main() {
  // 1. Abre o controlador GPIO do Raspberry Pi 3 (geralmente "gpiochip0")
  // NOTA: Em modelos como o Pi 4, pode ser "gpiochip4". Se der erro, altere aqui.
  chip = gpiod_chip_open_by_name("gpiochip0");
  if (!chip) {
    printf("Erro ao aceder ao controlador GPIO (gpiochip0)!\n");
    return 1;
  }

  // 2. Obtém a referência de cada linha (pino BCM)
  line0 = gpiod_chip_get_line(chip, LED_BIT0);
  line1 = gpiod_chip_get_line(chip, LED_BIT1);
  line2 = gpiod_chip_get_line(chip, LED_BIT2);
  line3 = gpiod_chip_get_line(chip, LED_BIT3);

  // 3. Configura os pinos como saída (Output) com valor inicial 0
  gpiod_line_request_output(line0, "calc_led0", 0);
  gpiod_line_request_output(line1, "calc_led1", 0);
  gpiod_line_request_output(line2, "calc_led2", 0);
  gpiod_line_request_output(line3, "calc_led3", 0);

  char sA[17], sB[17];
  char operacao;

  printf("=== CALCULADORA BINÁRIA 4 BITS / 16 BITS (GPIOD) ===\n");

  while (1) {
    printf("\nDigite o Operando A (16 bits em binário): ");
    scanf("%16s", sA);
    printf("Digite o Operando B (16 bits em binário): ");
    scanf("%16s", sB);
    printf("Escolha a operação (+, -, *, /, !): ");
    scanf(" %c", &operacao);

    int valA = (int)strtol(sA, NULL, 2);
    int valB = (int)strtol(sB, NULL, 2);

    if (valA & 0x8000) valA |= ~0xFFFF;
    if (valB & 0x8000) valB |= ~0xFFFF;

    struct timespec t_start, t_end;
    clock_gettime(CLOCK_MONOTONIC, &t_start);

    int resultado = 0;
    bool div_zero = false;
    char nomeOp[20];

    switch (operacao) {
      case '+': resultado = valA + valB; strcpy(nomeOp, "SOMA"); break;
      case '-': resultado = valA - valB; strcpy(nomeOp, "SUBTRACAO"); break;
      case '*': resultado = multiply(valA, valB); strcpy(nomeOp, "MULTIPLICACAO"); break;
      case '!': resultado = factorial(valA); strcpy(nomeOp, "FATORIAL"); break;
      case '/':
        resultado = divide(valA, valB);
        if (resultado == -999) { div_zero = true; resultado = 0; }
        strcpy(nomeOp, "DIVISAO");
        break;
      default:
        printf("Operação Inválida!\n");
        continue;
    }

    clock_gettime(CLOCK_MONOTONIC, &t_end);

    long long tempo_us = (t_end.tv_sec - t_start.tv_sec) * 1000000LL +
                         (t_end.tv_nsec - t_start.tv_nsec) / 1000LL;

    bool ov = (!div_zero) && (resultado > 32767 || resultado < -32768);

    int bits16 = resultado & 0xFFFF;
    int dec = (bits16 & 0x8000) ? (bits16 - 65536) : bits16;

    char binStr[17];
    printBinary16(bits16, binStr);

    // Atualiza os LEDs com os 4 bits menos significativos
    setLeds(div_zero ? 0 : (bits16 & 0x0F));

    printf("\n--- RESULTADO ---\n");
    printf("Operação Realizada: %s\n", nomeOp);
    printf("Resultado Binário : %s\n", binStr);
    printf("Resultado Decimal : %d\n", dec);
    printf("Tempo de Cálculo  : %lld us\n", tempo_us);
    
    if (div_zero) printf("Status            : ** ERRO: DIVISÃO POR ZERO **\n");
    else if (ov) printf("Status            : ** OVERFLOW DETECTADO **\n");
    else printf("Status            : OK\n");
    
    printf("-----------------\n");
  }

  // Liberta os recursos ao fechar
  gpiod_chip_close(chip);
  return 0;
}
