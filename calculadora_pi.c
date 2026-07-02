#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <stdbool.h>
#include <wiringPi.h> // Biblioteca para controle de GPIO no Raspberry Pi

// Mapeamento de pinos GPIO (numeração WiringPi)
// No Raspberry Pi, verifique a tabela de mapeamento correspondente aos pinos físicos
#define LED_BIT0 0  // Pino físico 11 (GPIO17)
#define LED_BIT1 1  // Pino físico 12 (GPIO18)
#define LED_BIT2 2  // Pino físico 13 (GPIO27)
#define LED_BIT3 3  // Pino físico 15 (GPIO22)

// Funções Aritméticas (Mantendo a lógica original do desafio)
int multiply(int a, int b) {
  int result = 0;
  int sign = 1;
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
  if (b == 0) return -999; // Sentinela para divisão por zero 
  return a / b; 
}

// Controle de LEDs via WiringPi
void setLeds(int valor) {
  int v = valor & 0x0F; 
  digitalWrite(LED_BIT0, (v >> 0) & 1); 
  digitalWrite(LED_BIT1, (v >> 1) & 1); 
  digitalWrite(LED_BIT2, (v >> 2) & 1); 
  digitalWrite(LED_BIT3, (v >> 3) & 1); 
}

// Função para exibir a string binária de 16 bits
void printBinary16(int valor, char* buffer) {
  for (int i = 15; i >= 0; i--) { 
    buffer[15 - i] = ((valor >> i) & 1) ? '1' : '0'; 
  }
  buffer[16] = '\0';
}

int main() {
  // Inicializa a biblioteca WiringPi
  if (wiringPiSetup() == -1) {
    printf("Erro ao inicializar a fiação WiringPi!\n");
    return 1;
  }

  // Configura os pinos dos LEDs como saída
  pinMode(LED_BIT0, OUTPUT);
  pinMode(LED_BIT1, OUTPUT);
  pinMode(LED_BIT2, OUTPUT);
  pinMode(LED_BIT3, OUTPUT);

  // Inicializa LEDs em nível baixo
  setLeds(0);

  char sA[17], sB[17];
  char operacao;

  printf("=== CALCULADORA BINÁRIA 4 BITS / 16 BITS (RASPBERRY PI 3) ===\n");

  while (1) {
    printf("\nDigite o Operando A (16 bits em binário, ex: 0000000000000011): ");
    scanf("%16s", sA);
    printf("Digite o Operando B (16 bits em binário, ex: 0000000000000010): ");
    scanf("%16s", sB);
    printf("Escolha a operação (+, -, *, /, !): ");
    scanf(" %c", &operacao);

    // Parser: string binária 16 bits → int
    int valA = (int)strtol(sA, NULL, 2);
    int valB = (int)strtol(sB, NULL, 2);

    // Extensão de sinal para complemento de dois em 16 bits
    if (valA & 0x8000) valA |= ~0xFFFF;
    if (valB & 0x8000) valB |= ~0xFFFF;

    // Medição de tempo de alta precisão no Linux (CLOCK_MONOTONIC)
    struct timespec t_start, t_end;
    clock_gettime(CLOCK_MONOTONIC, &t_start);

    int resultado = 0;
    bool div_zero = false;
    char nomeOp[20];

    switch (operacao) {
      case '+':
        resultado = valA + valB;
        strcpy(nomeOp, "SOMA"); 
        break;
      case '-':
        resultado = valA - valB; 
        strcpy(nomeOp, "SUBTRACAO"); 
        break;
      case '*':
        resultado = multiply(valA, valB); 
        strcpy(nomeOp, "MULTIPLICACAO"); 
        break;
      case '!':
        resultado = factorial(valA); 
        strcpy(nomeOp, "FATORIAL"); 
        break;
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

    // Calcula tempo decorrido em microssegundos (us)
    long long tempo_us = (t_end.tv_sec - t_start.tv_sec) * 1000000LL +
                         (t_end.tv_nsec - t_start.tv_nsec) / 1000LL;

    // Verificação de Overflow (-32768 a +32767)
    bool ov = (!div_zero) && (resultado > 32767 || resultado < -32768); 

    int bits16 = resultado & 0xFFFF; 
    int dec = (bits16 & 0x8000) ? (bits16 - 65536) : bits16; 

    char binStr[17];
    printBinary16(bits16, binStr);

    // Atualiza os LEDs físicos com os 4 bits menos significativos (nibble low)
    setLeds(div_zero ? 0 : (bits16 & 0x0F)); 

    // Exibição dos resultados no terminal (Saída HDMI)
    printf("\n--- RESULTADO ---\n");
    printf("Operação Realizada: %s\n", nomeOp);
    printf("Resultado Binário : %s\n", binStr);
    printf("Resultado Decimal : %d\n", dec);
    printf("Tempo de Cálculo  : %lld us\n", tempo_us);
    
    if (div_zero) {
      printf("Status            : ** ERRO: DIVISÃO POR ZERO **\n");
    } else if (ov) {
      printf("Status            : ** OVERFLOW DETECTADO **\n");
    } else {
      printf("Status            : OK\n");
    }
    printf("-----------------\n");
  }

  return 0;
}
