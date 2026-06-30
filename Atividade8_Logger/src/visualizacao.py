import serial
import matplotlib.pyplot as plt
from collections import deque

# Configuração da porta serial
PORTA = 'COM6'
BAUD = 115200
MAX_PONTOS = 200  # quantos pontos mostrar na tela

# Abre a serial
ser = serial.Serial(PORTA, BAUD, timeout=1)

# Buffers circulares para os dados
tempos = deque(maxlen=MAX_PONTOS)
valores = deque(maxlen=MAX_PONTOS)

# Configura o gráfico
plt.ion()
fig, ax = plt.subplots()
linha, = ax.plot([], [], 'b-')
ax.set_xlabel('Amostra')
ax.set_ylabel('Valor ADC (raw)')
ax.set_title('Aquisicao de Dados em Tempo Real')
ax.grid(True)

contador = 0
print("Lendo dados... feche a janela do grafico para parar.")

try:
    while True:
        linha_serial = ser.readline().decode('utf-8', errors='ignore').strip()

        # Ignora linhas de cabeçalho (começam com #)
        if linha_serial.startswith('#') or not linha_serial:
            continue

        # Tenta separar timestamp,valor
        try:
            partes = linha_serial.split(',')
            valor = int(partes[1])
        except (IndexError, ValueError):
            continue

        valores.append(valor)
        tempos.append(contador)
        contador += 1

        # Atualiza o gráfico
        linha.set_data(list(tempos), list(valores))
        ax.relim()
        ax.autoscale_view()
        plt.pause(0.001)

except KeyboardInterrupt:
    print("Encerrado pelo usuario.")
finally:
    ser.close()