#%% Importando bibliotecas!
import numpy as np
import time
import csv
import os
#%% Função para gerar dados meteorológicos
def generate_weather_data(csv_filename, interval=2):
    """
    Gera valores contínuos de temperatura, umidade e pressão em função do tempo,
    simulando condições meteorológicas típicas do Pará, e salva os dados em um arquivo CSV.

    :param csv_filename: Nome do arquivo CSV para salvar os dados.
    :param interval: Intervalo entre cada medição em segundos.
    """
    # Verifica se o arquivo CSV já existe e, se não, cria-o com os cabeçalhos
    if not os.path.exists(csv_filename):
        with open(csv_filename, mode='w', newline='') as file:
            writer = csv.writer(file)
            writer.writerow(["Time", "Temperature (°C)", "Humidity (%)", "Pressure (hPa)"])

    while True:
        elapsed_time = time.time()
        
        # Simulação da temperatura no Pará (média mais alta, entre 24°C e 34°C)
        temp = 28 + 4 * np.sin(elapsed_time * 0.1)
        
        # Simulação da umidade relativa (normalmente entre 70% e 90% no Pará)
        humidity = 80 + 5 * np.sin(elapsed_time * 0.05) + np.random.normal(0, 2)
        
        # Simulação da pressão atmosférica (em hPa, normalmente entre 1007 e 1015 hPa no Pará)
        pressure = 1010 + 2 * np.sin(elapsed_time * 0.02) + np.random.normal(0, 0.3)
        
        current_time = time.strftime("%H:%M:%S", time.localtime())
        
        # Escreve os dados no arquivo CSV
        with open(csv_filename, mode='a', newline='') as file:
            writer = csv.writer(file)
            writer.writerow([current_time, f"{temp:.2f}", f"{humidity:.2f}", f"{pressure:.2f}"])
        
        print(f"Time: {current_time} | Temperature: {temp:.2f}°C | Humidity: {humidity:.2f}% | Pressure: {pressure:.2f} hPa")
        
        time.sleep(interval)


# Exemplo de uso
generate_weather_data(csv_filename="weather_data_para.csv", interval=2)

