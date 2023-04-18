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
#define BAND 915E6  //frequencia lora
#define MAC_PET "[24:62:AB:DD:D0:68]" //define MAC do PET para comparar com o endereço recebido via LoRa
#define MAC_Servidor "[CC:50:E3:81:9D:24]" //define MAC do Servidor para usar no protocolo de comunicação LoRa

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
    DadosGPS = MAC_Servidor+DadosGPS;   //adiciona o endereço MAC do Servidor no inicio do pacote com dados do GPS
    Serial.print("Pacote Enviado: "); Serial.println(DadosGPS);
    //Enviando pacote LoRa para o receptor
    LoRa.beginPacket();
    LoRa.print(DadosGPS);
    LoRa.endPacket();
  }
}

bool validaMAC(String* LoRaDados){
  int index1 = LoRaDados->indexOf('[');
  int index2 = LoRaDados->indexOf(']')+1;
  String MAC = LoRaDados->substring(index1, index2);
  if(MAC == MAC_PET){
    *LoRaDados = LoRaDados->substring(index2);
    return true;
  }else return false;
}

void LoRaRecebeDados(){
  //recebe o pacote LoRa
  int packetSize = LoRa.parsePacket();
  if (packetSize) {
    String LoRaDados; //pacotes de dados recebido via LoRa
    
    //lendo o pacote
    while (LoRa.available())
      LoRaDados = LoRa.readString();

    //escreve o pacote no serial
    Serial.print("Pacote Recebido: ");Serial.println(LoRaDados);

    if(validaMAC(&LoRaDados)){
      char LoRaDados_char[LoRaDados.length()+1];
      LoRaDados.toCharArray(LoRaDados_char, LoRaDados.length()+1);
  
      for(int i=0; i<LoRaDados.length()+1; i++){
        if((LoRaDados_char[i]=='T') || (LoRaDados_char[i]=='F')){
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
