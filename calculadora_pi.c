#include <stdio.h>
#include <time.h>

long long fatorial(int n) {
    long long resultado = 1;

    if (n < 0)
        return -1;

    for (int i = 1; i <= n; i++)
        resultado *= i;

    return resultado;
}

unsigned int multiplicacaoBinaria(unsigned int a, unsigned int b) {
    unsigned int resultado = 0;

    while (b > 0) {
        if (b & 1)
            resultado += a;

        a <<= 1;
        b >>= 1;
    }

    return resultado;
}

void soma() {
    int a, b;
    printf("Digite dois numeros: ");
    scanf("%d %d", &a, &b);

    printf("Resultado = %d\n", a + b);
}

void subtracao() {
    int a, b;
    printf("Digite dois numeros: ");
    scanf("%d %d", &a, &b);

    printf("Resultado = %d\n", a - b);
}

void divisao() {
    int a, b;

    printf("Digite dois numeros: ");
    scanf("%d %d", &a, &b);

    if (b == 0) {
        printf("Erro: divisao por zero.\n");
        return;
    }

    printf("Resultado = %.2f\n", (float)a / b);
}

void multiplicacao() {
    unsigned int a, b;

    printf("Digite dois numeros: ");
    scanf("%u %u", &a, &b);

    printf("Resultado = %u\n", multiplicacaoBinaria(a, b));
}

void calcularFatorial() {
    int n;

    printf("Digite um numero: ");
    scanf("%d", &n);

    if (n < 0) {
        printf("Nao existe fatorial de numero negativo.\n");
        return;
    }

    printf("%d! = %lld\n", n, fatorial(n));
}

int main() {

    char operador;
    clock_t inicio, fim;
    double tempo;

    printf("===== CALCULADORA =====\n");
    printf("+ Soma\n");
    printf("- Subtracao\n");
    printf("* Multiplicacao Binaria\n");
    printf("/ Divisao\n");
    printf("! Fatorial\n\n");

    printf("Escolha a operacao: ");
    scanf(" %c", &operador);

    inicio = clock();

    switch (operador) {

        case '+':
            soma();
            break;

        case '-':
            subtracao();
            break;

        case '*':
            multiplicacao();
            break;

        case '/':
            divisao();
            break;

        case '!':
            calcularFatorial();
            break;

        default:
            printf("Operador invalido.\n");
            return 1;
    }

    fim = clock();

    tempo = ((double)(fim - inicio)) / CLOCKS_PER_SEC;

    printf("\nTempo de execucao: %.8f segundos\n", tempo);

    return 0;
}
