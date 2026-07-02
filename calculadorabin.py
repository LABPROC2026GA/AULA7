from RPLCD.i2c import CharLCD
from pad4pi import rpi_gpio
import time

# ---------------- LCD ----------------

lcd = CharLCD(
    i2c_expander='PCF8574',
    address=0x27,
    port=1,
    cols=16,
    rows=2
)

# ---------------- TECLADO ----------------

KEYPAD = [
    ["1","2","3","A"],
    ["4","5","6","B"],
    ["7","8","9","C"],
    ["*","0","#","D"]
]

ROW_PINS = [5,6,13,19]
COL_PINS = [12,16,20,21]

factory = rpi_gpio.KeypadFactory()

keypad = factory.create_keypad(
    keypad=KEYPAD,
    row_pins=ROW_PINS,
    col_pins=COL_PINS
)

ultima_tecla = None

def tecla_pressionada(key):
    global ultima_tecla
    ultima_tecla = key

keypad.registerKeyPressHandler(tecla_pressionada)

# ---------------- FUNÇÕES ----------------

def esperar_tecla():

    global ultima_tecla

    while ultima_tecla is None:
        time.sleep(0.05)

    t = ultima_tecla
    ultima_tecla = None

    return t


def ler_numero(msg):

    lcd.clear()
    lcd.write_string(msg)

    numero = ""

    while True:

        tecla = esperar_tecla()

        if tecla == "#":
            break

        if tecla.isdigit():
            numero += tecla

            lcd.cursor_pos = (1,0)
            lcd.write_string(numero)

    if numero == "":
        return 0

    return int(numero)


def fatorial(n):

    if n < 0:
        return None

    resultado = 1

    for i in range(1,n+1):
        resultado *= i

    return resultado


def multiplicacao_binaria(a,b):

    resultado = 0

    while b > 0:

        if b & 1:
            resultado += a

        a <<= 1
        b >>= 1

    return resultado


def mostrar(texto):

    lcd.clear()
    lcd.write_string(str(texto))

    time.sleep(3)

# ---------------- MENU ----------------

while True:

    lcd.clear()

    lcd.write_string("A+ B- C* D/")

    lcd.cursor_pos = (1,0)

    lcd.write_string("* Fat")

    op = esperar_tecla()

    inicio = time.time()

    if op == "A":

        a = ler_numero("Primeiro")

        b = ler_numero("Segundo")

        mostrar(a+b)

    elif op == "B":

        a = ler_numero("Primeiro")

        b = ler_numero("Segundo")

        mostrar(a-b)

    elif op == "C":

        a = ler_numero("Primeiro")

        b = ler_numero("Segundo")

        mostrar(multiplicacao_binaria(a,b))

    elif op == "D":

        a = ler_numero("Primeiro")

        b = ler_numero("Segundo")

        if b == 0:
            mostrar("Erro")
        else:
            mostrar(round(a/b,2))

    elif op == "*":

        n = ler_numero("Numero")

        mostrar(fatorial(n))

    fim = time.time()

    lcd.clear()
    lcd.write_string("Tempo:")

    lcd.cursor_pos = (1,0)
    lcd.write_string(f"{fim-inicio:.6f}s")

    time.sleep(2)