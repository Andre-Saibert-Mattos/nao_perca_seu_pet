# nao_perca_seu_pet
Aplicação desenvolvida para rastreamento de pets, o projeto consiste em um hardware a ser fixado no pet, na coleira por exemplo, o mesmo obtêm a sua localização por meio de um módulo GPS e a envia via LoRa para um segundo hardware que será o servidor de ancoragem do dispositivo no pet e um celular android, ao receber a localização o servidor a envia para o celular via bluetooth e mede a distância entre ele e o pet, tendo como parâmetro uma zona segura, caso o pet esteja fora desta zona o servidor manda uma solicitação ao dispositivo do pet para enviar a sua localização com um espaço de tempo menor, e produz um bip para alertar o usuário que o pet escapou da zona segura, fazendo assim com que o usuário pegue o celular e abra o aplicativo. No aplicativo existe um mapa onde mostra a localização o próprio aparelho celular e do pet, traçando uma rota entre eles para que o usuário consiga se encaminhar até o seu pet, no aplicativo também existe uma aba de configuração onde é possível definir o local do servidor e qual o raio da zona segura, ao salvar, estes dados são enviados para o servidor e armazenados na memória, para serem posteriormente usados medição de distância entre ele e o pet.

Hardwares:

PET: placa TTGO LoRa32-OLED V1

Servidor: placa Heltec WiFi LoRa 32

App Android desenvolvido na plataforma MIT App Inverntor
