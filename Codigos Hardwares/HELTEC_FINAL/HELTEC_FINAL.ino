//Bibliotecas
#include <SPI.h>
#include <LoRa.h>
#include <BluetoothSerial.h>
#include <Wire.h>
#include <TinyGPS++.h>

//definições memoria EEPROM
#define disk1 0x50  //endereço da memoria, varia de 0x50 a 0x57, fefinido pelos switch do modulo
#define address 0   //posição inicial da memoria
#define maxPac 26   //maior pacote a ser armazenado na memoria, quantidade de caracteres
#define minPac 19   //menor pacote a ser armazenado na memoria, quantidade de caracteres

/* Verifique se as configurações Bluetooth estão ativadas no SDK */
/* Caso contrário, você terá que recompilar o SDK */
#if !defined(CONFIG_BT_ENABLED) || !defined(CONFIG_BLUEDROID_ENABLED)
#error Bluetooth is not enabled! Please run `make menuconfig` to and enable it
#endif

//define os pinos do modulo LoRa
#define SCK 5   
#define MISO 19 
#define MOSI 27 
#define SS 18   
#define RST 14  
#define DIO0 26 

//define banda LoRa
#define BAND 915E6

//define o tamanho maximo do pacote bluetooth, quantidade de caracteres
#define tamPacoteBt 24

//define configurações do buzzer
#define pinoBuzzer 25
#define canalBuzzer 0

//variaveis para acionamento do buzzer
bool acionaBuzzer=false;
int timerBuzzer=0;

// pacotes de dados
String LoRaDados;
String aux_LoRaEnviaDados="";

char BtDadosEnvia[tamPacoteBt];
String BtDadosRecebe;

//inicia a variavel que tratara o bluetooth
BluetoothSerial SerialBT;

//===========================================================================================================
void writeEEPROM(int deviceaddress, unsigned int eeaddress, char* dados, byte tam) {
  //escreve o bacote de dados na memoria
  Wire.beginTransmission(deviceaddress);
  Wire.write((int)(eeaddress >> 8));   // Byte mais significativo
  Wire.write((int)(eeaddress & 0xFF)); // Byte menos significativo
  byte i;
  for(i=0; i<tam; i++)
    Wire.write(dados[i]);
  Wire.endTransmission();
  delay(5);

  //confere se existe bytes na sequencia gravados, se sim apagaos
  int aux=0;
  if(i<maxPac){
    for(int j=0; j<(maxPac-i); j++){
      Wire.beginTransmission(deviceaddress);
      Wire.write((int)(i+j >> 8));
      Wire.write((int)(i+j & 0xFF));
      Wire.endTransmission();
      Wire.requestFrom(deviceaddress,1);
      if (Wire.available())
        aux++;
    }
  }
  if(aux!=0){
    Wire.beginTransmission(deviceaddress);
    Wire.write((int)(i >> 8));   // Byte mais significativo
    Wire.write((int)(i & 0xFF)); // Byte menos significativo
    for(int k=0; k<aux; k++)
      Wire.write(0xFF);
    Wire.endTransmission();
    delay(5);
  }
}

//le byte a byte da memoria EEPROM e retorno
byte readByteEEPROM(int deviceaddress, unsigned int eeaddress ) {
  byte rdata = 0x00;
  Wire.beginTransmission(deviceaddress);
  Wire.write((int)(eeaddress >> 8));
  Wire.write((int)(eeaddress & 0xFF));
  Wire.endTransmission();
  Wire.requestFrom(deviceaddress,1);
  
  if (Wire.available())
    rdata = Wire.read();
  return rdata;
}

//monta uma String com a informação salva na memoria
String lerEEPROM(){
  //le a memoria EEPROM
  char rByte;
  String rMemoria="";
  int contEstrapola=0;
  for(int i=0; rByte!=','; i++){
    if(contEstrapola==32) return("E");
    rByte=(char)readByteEEPROM(disk1, i);
    rMemoria+=rByte;
    contEstrapola++;
  }
  return(rMemoria);
}

//===========================================================================================================
void splitString(String pacote, String* armazem){
  int cont=0;
  //retira os caracteres de inicio e final de pacote
  pacote = pacote.substring(pacote.indexOf(':')+1, pacote.indexOf(','));
  while (pacote.length() > 0){
    int index = pacote.indexOf(';');
    if (index == -1){
      armazem[cont++] = pacote;
      break;
    }else{
      armazem[cont++] = pacote.substring(0, index);
      pacote = pacote.substring(index+1);
    }
  }
}

//===========================================================================================================
double distancia(){
  double dist=0.0;
  //pega na memoria EEPROM a localização do servidor e atribui para uma array
  String rEEPROM[3];
  String rsEEPROM = lerEEPROM();
  if(rsEEPROM!="E"){
    splitString(rsEEPROM, rEEPROM);
  
    //pega a ultima localização do pet recebida via LoRa e atribui para uma array
    String rLoRa[2];
    splitString(LoRaDados, rLoRa);
  
    //função que mede distancia entre duas coordenadas
    dist = TinyGPSPlus::distanceBetween(
      rLoRa[0].toDouble(), rLoRa[1].toDouble(),
      rEEPROM[0].toDouble(), rEEPROM[1].toDouble());
  }
  return(dist);
}

//===========================================================================================================
void LoRaEnviaDados(String envio){
  if(envio!=aux_LoRaEnviaDados){
    aux_LoRaEnviaDados=envio;
    Serial.print("Pacote Enviado: "); Serial.println(envio);
    //Enviando pacote LoRa para o receptor
    LoRa.beginPacket();
    LoRa.print(envio);
    LoRa.endPacket();
  }
}

void LoRaRecebeDados(){
  //recebe o pacote LoRa
  int packetSize = LoRa.parsePacket();
  if (packetSize) {
    LoRaDados="";
    
    //lendo o pacote
    while (LoRa.available())
      LoRaDados = LoRa.readString();

    //recebeu um pacote
    Serial.print("Pacote Recebido: ");
    //escreve o pacote no serial
    Serial.println(LoRaDados);
      
    //monta o pacote bluetooth
    LoRaDados.toCharArray(BtDadosEnvia, tamPacoteBt);
    
    //envia o pacote
    for(int i=0; i<tamPacoteBt; i++)
      SerialBT.write(BtDadosEnvia[i]);

    //======checagem zona segura======
    String rsLoRaDados[2];
    splitString(LoRaDados, rsLoRaDados);
    if((rsLoRaDados[0].toDouble()!=0.0) && (rsLoRaDados[1].toDouble()!=0.0)){
      String rEEPROM[3];
      String rsEEPROM = lerEEPROM();
      if(rsEEPROM!="E"){
        splitString(rsEEPROM, rEEPROM);
      
        if(distancia()>(rEEPROM[2].toDouble()+3)){
          acionaBuzzer=true;
          LoRaEnviaDados(":T,");
          Serial.println("Pet fora da zona segura");
        }else{
          acionaBuzzer=false;
          LoRaEnviaDados(":F,");
          Serial.println("Pet dentro da zona segura");
        }
      }
    }else acionaBuzzer=false;
  }
}

//===========================================================================================================
void BtRecebeDados(){
  //recebe dados enviados pelo app via bluetooth
  BtDadosRecebe="";
  while(SerialBT.available())
    BtDadosRecebe+=(char)SerialBT.read();
  if((BtDadosRecebe!="") && (BtDadosRecebe.length()>=minPac) && (BtDadosRecebe.length()<=maxPac)){
    //cria uma array de char com o tamanho da String
    char string[BtDadosRecebe.length()];
    //transcreve a String dentro da array de char
    BtDadosRecebe.toCharArray(string, BtDadosRecebe.length()+1);
    //escreve a array de char na memoria
    writeEEPROM(disk1, address, string, sizeof(string));
  }else if((BtDadosRecebe!="") && (BtDadosRecebe=="1_init")){
    String dadosEEPROM = lerEEPROM();
    if(dadosEEPROM!="E"){
      char BtEnviaEEPROM[dadosEEPROM.length()+1];
  
      //monta o pacote bluetooth
      dadosEEPROM.toCharArray(BtEnviaEEPROM, dadosEEPROM.length()+1);

      //envia o pacote
      for(int i=0; i<dadosEEPROM.length()+1; i++)
        SerialBT.write(BtEnviaEEPROM[i]);
    }
  }
}

//===========================================================================================================
void acionarBuzzer(){
  if(acionaBuzzer){
    if(millis()-timerBuzzer>800){
      timerBuzzer=millis();
      ledcWriteTone(canalBuzzer, 880);
    }
    if(millis()-timerBuzzer>600) ledcWriteTone(canalBuzzer, 0);
  }else ledcWriteTone(canalBuzzer, 0);
}

//===========================================================================================================
void setup() {
  //configura o a saida para acionamento do buzzer
  ledcSetup(canalBuzzer, 2000, 10);
  ledcAttachPin(pinoBuzzer, 0);
  
  //Inicia o Serial Monitor
  Serial.begin(115200);
  Wire.begin(); 
  
  /* Se nenhum nome for dado, o padrão 'Esp32' será aplicado
   * Se você quiser dar seu próprio nome ao dispositivo esp32 bluetooth, então
   * Especifique o nome como um argumento serialbt.begin ("myesp32bluetooth*/
  SerialBT.begin();
  Serial.println("O Bluetooth começou! Pronto para emparelhar ...");
  
  Serial.print("LoRa Receptor Teste: ");

  //SPI LoRa pinos
  SPI.begin(SCK, MISO, MOSI, SS);
  //setup LoRa modulo transceptor
  LoRa.setPins(SS, RST, DIO0);
  
  if (!LoRa.begin(BAND)) {
    Serial.println("Inicializaçaõ LoRa falhou!");
    while (1);
  }
  Serial.println("LoRa Inicializado OK!");
}

//===========================================================================================================
void loop() {
  LoRaRecebeDados();

  BtRecebeDados();

  acionarBuzzer();
}
