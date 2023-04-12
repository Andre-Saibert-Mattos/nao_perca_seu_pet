//Biblioteca GPS
#include <TinyGPS++.h>
//Bibliotecas LoRa
#include <SPI.h>
#include <LoRa.h>

//define os pinos do modulo LoRa
#define SCK 5
#define MISO 19
#define MOSI 27
#define SS 18
#define RST 14
#define DIO0 26

//=====================Variaveis configuraveis=====================
//frequencia lora
#define BAND 915E6

int temp_Env_Lora=8000; //tempo em millis, envio de pacotes via LoRa
int temp_Cap_GPS=1000; //tempo em millis, captura de dados GPS

//===========================================================================================================
int tempo=0;  //variavel auxiliar para calculo de tempo

String DadosGPS;  //pacote de dados GPS enviado via LoRa

TinyGPSPlus gps;  //declara a variavel atrelada ao gps

//================Apresenta os dados lidos pelo GPS================
static void smartDelay(unsigned long ms){
  unsigned long start = millis();
  do{
    while (Serial1.available())
      gps.encode(Serial1.read());
  }while (millis() - start < ms);
}

//monta o pacote para envio LoRa
void Dados_GPS(){
  DadosGPS=":";
  DadosGPS+=String(gps.location.lat(), 6);
  DadosGPS+=';';
  DadosGPS+=String(gps.location.lng(), 6);
  DadosGPS+=',';
  
  smartDelay(temp_Cap_GPS);                                      

  if (millis() > 5000 && gps.charsProcessed() < 10)
    Serial.println(F("Nenhum dado de GPS recebido: verifique a fiação"));
}

//===========================================================================================================
void LoRaEnviaDados(){
  if(millis()-tempo>temp_Env_Lora){
    tempo=millis();
    //Enviando pacote LoRa para o receptor
    LoRa.beginPacket();
    LoRa.print(DadosGPS);
    LoRa.endPacket();
  }
}

//===========================================================================================================
void LoRaRecebeDados(){
  //recebe o pacote LoRa
  int packetSize = LoRa.parsePacket();
  if (packetSize) {
    String LoRaDados; //pacotes de dados recebido via LoRa
    
    //lendo o pacote
    while (LoRa.available())
      LoRaDados = LoRa.readString();

    char LoRaDados_char[LoRaDados.length()+1];
    LoRaDados.toCharArray(LoRaDados_char, LoRaDados.length()+1);

    for(int i=0; i<LoRaDados.length()+1; i++){
      if((LoRaDados_char[i]=='T') || (LoRaDados_char[i]=='F')){
        //recebeu um pacote
        Serial.println("Pacote Recebido: ");
        //escreve o pacote no serial
        Serial.println(LoRaDados_char[i]);
    
        switch(LoRaDados_char[i]){
          case 'T':
            temp_Env_Lora=2000;
            break;
          case 'F':
            temp_Env_Lora=8000;
            break;
        }
      }
    }
  }
}

//===========================================================================================================
void setup(){
  Serial.begin(115200); //inicia o monitor serial para apresentação dos dados
  Serial1.begin(9600, SERIAL_8N1, 12, 15);  //inicia um segundo serial para ser usado pelo gps

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
void loop(){
  Dados_GPS();
  LoRaEnviaDados();
  LoRaRecebeDados();
}
