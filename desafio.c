#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <stdbool.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <linux/i2c-dev.h>
#include <gpiod.h>

// ---- CONFIGURAÇÃO DO LCD I2C (Driver Nativo Linux) ----
#define I2C_ADDR 0x27 // Mude para 0x3F se o seu display não ligar
int i2c_fd;

void i2c_write(uint8_t data) {
    write(i2c_fd, &data, 1);
}

void lcd_pulse_enable(uint8_t data) {
    i2c_write(data | 0x04); // EN = 1
    usleep(1);
    i2c_write(data & ~0x04); // EN = 0
    usleep(50);
}

void lcd_send_nibble(uint8_t nibble, uint8_t rs) {
    uint8_t data = (nibble << 4) | (rs ? 1 : 0) | 0x08; // 0x08 = Luz de fundo (Backlight) ligada
    i2c_write(data);
    lcd_pulse_enable(data);
}

void lcd_send_byte(uint8_t byte, uint8_t rs) {
    lcd_send_nibble(byte >> 4, rs);
    lcd_send_nibble(byte & 0x0F, rs);
}

void lcd_clear() {
    lcd_send_byte(0x01, 0);
    usleep(2000);
}

void lcd_position(int row, int col) {
    int row_offsets[] = {0x00, 0x40};
    lcd_send_byte(0x80 | (col + row_offsets[row]), 0);
}

void lcd_print(const char* str) {
    while (*str) lcd_send_byte(*str++, 1);
}

void lcd_init(int addr) {
    i2c_fd = open("/dev/i2c-1", O_RDWR);
    if (i2c_fd < 0) {
        printf("Erro ao abrir barramento I2C!\n");
        exit(1);
    }
    ioctl(i2c_fd, I2C_SLAVE, addr);
    usleep(50000);
    lcd_send_nibble(0x03, 0); usleep(5000);
    lcd_send_nibble(0x03, 0); usleep(150);
    lcd_send_nibble(0x03, 0);
    lcd_send_nibble(0x02, 0); // Configura para modo 4 bits
    lcd_send_byte(0x28, 0);   // 2 linhas, matriz 5x8
    lcd_send_byte(0x0C, 0);   // Liga display, desliga cursor
    lcd_clear();
}

// ---- CONFIGURAÇÃO DO TECLADO MATRICIAL (GPIOD) ----
// ATENÇÃO: Na gpiod usamos a numeração BCM do processador.
// Conecte as Linhas aos pinos BCM 5, 6, 13, 19
// Conecte as Colunas aos pinos BCM 12, 16, 20, 21
int rowPins[4] = {5, 6, 13, 19};
int colPins[4] = {12, 16, 20, 21};

struct gpiod_chip *chip;
struct gpiod_line *rowLines[4];
struct gpiod_line *colLines[4];

char keys[4][4] = {
  {'1', '2', '3', '+'},
  {'4', '5', '6', '-'},
  {'7', '8', '9', '*'},
  {'C', '0', '=', '/'}
};

void setupKeyboard() {
    chip = gpiod_chip_open_by_name("gpiochip0"); // Use "gpiochip4" no Raspberry Pi 4 se falhar
    if (!chip) {
        printf("Erro ao acessar GPIO!\n");
        exit(1);
    }

    for (int i = 0; i < 4; i++) {
        // Configura Linhas como saída, nível 0
        rowLines[i] = gpiod_chip_get_line(chip, rowPins[i]);
        gpiod_line_request_output(rowLines[i], "keypad_row", 0);

        // Configura Colunas como entrada com Pull-Down interno
        colLines[i] = gpiod_chip_get_line(chip, colPins[i]);
        struct gpiod_line_request_config config = {
            .consumer = "keypad_col",
            .request_type = GPIOD_LINE_REQUEST_DIRECTION_INPUT,
            .flags = GPIOD_LINE_REQUEST_FLAG_BIAS_PULL_DOWN
        };
        gpiod_line_request(colLines[i], &config, 0);
    }
}

char getKey() {
    for (int r = 0; r < 4; r++) {
        gpiod_line_set_value(rowLines[r], 1);
        usleep(5000); // Debounce de 5ms
        for (int c = 0; c < 4; c++) {
            if (gpiod_line_get_value(colLines[c]) == 1) {
                while (gpiod_line_get_value(colLines[c]) == 1) usleep(1000); // Aguarda soltar
                gpiod_line_set_value(rowLines[r], 0);
                return keys[r][c];
            }
        }
        gpiod_line_set_value(rowLines[r], 0);
    }
    return '\0';
}

// ---- FUNÇÕES ARITMÉTICAS ----
int multiply(int a, int b) {
    int result = 0, sign = 1;
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

// ---- FUNÇÃO PRINCIPAL (MÁQUINA DE ESTADOS) ----
int main() {
    setupKeyboard();
    lcd_init(I2C_ADDR);

    lcd_position(0, 0);
    lcd_print("Calc 4 Bits Rasp");
    usleep(2000000); // Mostra o título por 2 segundos
    lcd_clear();
    lcd_print("A: ");

    char bufferA[10] = "";
    char bufferB[10] = "";
    char op = '\0';
    bool preenchendoA = true;
    char textBuf[32]; // Buffer para formatar textos do LCD

    while (1) {
        char key = getKey();
        if (key != '\0') {
            
            if (key == 'C') {
                bufferA[0] = '\0'; bufferB[0] = '\0';
                op = '\0'; preenchendoA = true;
                lcd_clear();
                lcd_print("A: ");
                continue;
            }

            if (key == '+' || key == '-' || key == '*' || key == '/') {
                if (preenchendoA && strlen(bufferA) > 0) {
                    op = key;
                    preenchendoA = false;
                    lcd_clear();
                    lcd_position(0, 0);
                    lcd_print("B: ");
                }
                continue;
            }

            if (key == '=') {
                if (strlen(bufferA) > 0 && strlen(bufferB) > 0 && op != '\0') {
                    int valA = atoi(bufferA) & 0x0F;
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
                    long long tempo = (t_end.tv_nsec - t_start.tv_nsec) / 1000LL;

                    lcd_clear();
                    if (div_zero) {
                        lcd_position(0, 0);
                        lcd_print("Erro: Div por 0");
                    } else {
                        int bits4 = resultado & 0x0F;
                        
                        lcd_position(0, 0);
                        snprintf(textBuf, sizeof(textBuf), "Res: %d T:%lldus", resultado, tempo);
                        lcd_print(textBuf);
                        
                        lcd_position(1, 0);
                        snprintf(textBuf, sizeof(textBuf), "Bin: %c%c%c%c", 
                                  (bits4 & 8)?'1':'0', (bits4 & 4)?'1':'0', 
                                  (bits4 & 2)?'1':'0', (bits4 & 1)?'1':'0');
                        lcd_print(textBuf);
                    }
                }
                continue;
            }

            if (preenchendoA) {
                if (strlen(bufferA) < 4) {
                    strncat(bufferA, &key, 1);
                    lcd_position(0, 3);
                    lcd_print(bufferA);
                }
            } else {
                if (strlen(bufferB) < 4) {
                    strncat(bufferB, &key, 1);
                    lcd_position(0, 3);
                    lcd_print(bufferB);
                }
            }
        }
        usleep(50000); // 50ms (Evita 100% de uso de CPU)
    }
    return 0;
}
