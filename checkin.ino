/*
  Matehackers (matehackers.org)
  Automateaberto-arduino
  Testado sobre arduino uno e duemilanove.
*/

#include <SPI.h>
#include <Ethernet.h>
#include <EthernetUdp.h>
#include <Time.h>

const int CHAVE = 7;
const int LED_WARNING = 8; //LED avisando que o dispositivo nao esta executando
int VALOR_PINO = 0;

//Variaveis para conexao com o mateaberto.herokuapp.com
byte mac[] = {  0xDE, 0xAD, 0xBE, 0xEF, 0xFE, 0xED };
char server[] = "mateaberto.herokuapp.com";
const int BUFFER_SIZE = 564;
char messageBuffer[BUFFER_SIZE];
int contentLength = 369;
int contador = 0;
EthernetClient client;

//Variaveis para deteccao de presenca

int segundosObservacao = 100; //segundos destinados a observacao de movimento
int minimoDeteccoes = 3; //valor minimo de deteccoes de presenca
//variaveis para conexao ntp
unsigned int localPort = 8888;      // porta local para receber a resposta do servidor NTP
IPAddress timeServer(200, 160, 7, 186); // time.nist.gov NTP server
const int NTP_PACKET_SIZE= 48; // timestamp do NTP encontra-se nos primeiros 48 bytes da mensagem
byte packetBuffer[ NTP_PACKET_SIZE]; //buffer de entrada e saida dos pacotes
int Timezone = -3;
EthernetUDP Udp; //envio de dados udp

int ano;
int mes;
int dia;
int horas;
int minutos;
int segundos;
int horaSaida;
int minutoSaida;


void post(){  
  
  obtemHora();
  //Serial.println("connectando...");
  
  if (client.connect(server, 80)) {
    //Serial.println("conectado");
    snprintf(messageBuffer,564,"POST /checkins HTTP/1.1\r\n"
                     "Host: mateaberto.herokuapp.com\r\n"
                     "Content-Type: application/x-www-form-urlencoded\r\n"
                     "Content-Length: %d\r\n\r\n"
                     "checkin%%5Bname%%5D=automateaberto"
                     "&checkin%%5Barrive%%281i%%29%%5D=%d" //ano
                     "&checkin%%5Barrive%%282i%%29%%5D=%d"    //mes
                     "&checkin%%5Barrive%%283i%%29%%5D=%d"    //dia
                     "&checkin%%5Barrive%%284i%%29%%5D=%d"   //hora
                     "&checkin%%5Barrive%%285i%%29%%5D=%d"   //minuto
                     "&checkin%%5Bleave%%281i%%29%%5D=%d"  //abaixo segue o mesmo formato
                     "&checkin%%5Bleave%%282i%%29%%5D=%d"
                     "&checkin%%5Bleave%%283i%%29%%5D=%d"
                     "&checkin%%5Bleave%%284i%%29%%5D=%d"
                     "&checkin%%5Bleave%%285i%%29%%5D=%d"    
                     "&commit=Create+Checkin\r\n",contentLength,ano,mes,dia,horas,minutos,ano,mes,dia,horaSaida,minutoSaida);//'/,ano,mes,dia,hora,minuto);
    Serial.write((uint8_t*)messageBuffer,BUFFER_SIZE); //post
    Serial.println();
    client.write((uint8_t*)messageBuffer,BUFFER_SIZE);
    client.println();
  } 
  else {
    //Serial.println("falha na conexao");
  }
  client.stop();
}

// envia pacote NTP para o endereco (address) passado.
unsigned long sendNTPpacket(IPAddress& address)
{
  // inicializa buffer
  memset(packetBuffer, 0, NTP_PACKET_SIZE); 
  //Inicializa campos do pacote NTP
  packetBuffer[0] = 0b11100011;   // LI, Version, Mode
  packetBuffer[1] = 0;     // Stratum, or type of clock
  packetBuffer[2] = 6;     // Polling Interval
  packetBuffer[3] = 0xEC;  // Peer Clock Precision
  // 8 bytes em zero 
  packetBuffer[12]  = 49; 
  packetBuffer[13]  = 0x4E;
  packetBuffer[14]  = 49;
  packetBuffer[15]  = 52;

  // Com o pacote completo, envia requisitando o timestamp
  Udp.beginPacket(address, 123);
  Udp.write(packetBuffer,NTP_PACKET_SIZE);
  Udp.endPacket(); 
}


void obtemHora(){
 
  Udp.begin(localPort);
  
  sendNTPpacket(timeServer); // envia pacote NTP

  //aguarda 1s para a resposta
  delay(1000);
  
  if ( Udp.parsePacket() ) {  
    //Se ha pacote parseado, le esse pacote.
    Udp.read(packetBuffer,NTP_PACKET_SIZE);

    //timestamp eh um inteiro(40bytes) que inicia no byte 40, 
    //le o dado em dois fragmentos de 2 bytes (word) cada.
    unsigned long highWord = word(packetBuffer[40], packetBuffer[41]);
    unsigned long lowWord = word(packetBuffer[42], packetBuffer[43]);  
    
    // Combina ambos fragmentos em um long (4 bytes)
    // Esse valor corresponde a hora no formato NTP (segundos desde 1 de janeiro de 1900)
    unsigned long secsSince1900 = highWord << 16 | lowWord;  
    
    //Converte a hora para o formato UNIX (utilizado na biblioteca Time.h)
    //A hora unix corresponde ao numero de segundos desde 1 de janeiro de 1970.
    //Serial.print("Hora Unix = ");
    // A diferenca eh de 70 anos, ou 2208988800 segundos:
    const unsigned long seventyYears = 2208988800UL;     
    //subtrai os 70 anos.
    unsigned long epoch = secsSince1900 - seventyYears;
    //Opera com o Timezone
    unsigned long epochBr = epoch + Timezone*3600;
    
    unsigned long epochSaida = epochBr + segundosObservacao; //acrescenta o tempo ateh o novo checkin (tempo de observacao)
    
    //Seta hora default da biblioteca com o horario brasileiro.
    setTime(epochBr);
    
    horas = (epochBr  % 86400L) / 3600;
    minutos = (epochBr  % 3600) / 60;
    segundos = epochBr % 60;
    
    ano = year(epochBr);
    mes = month(epochBr);
    dia = day(epochBr);
    
    //exibe a data
    /*
    Serial.println("Data(d/m/a): ");
    if(dia < 10) Serial.print('0');
    Serial.print(dia);    
    Serial.print('/');
    if(mes < 10) Serial.print('0');
    Serial.print(mes);
    Serial.print('/');    
    Serial.print(ano);
    Serial.println();
    
    //exibe a hora    
    Serial.println();
    Serial.println("Hora do checkin: ");    
    Serial.print(horas);    
    Serial.print(':');    
    if(minutos < 10) Serial.print('0');
    Serial.print(minutos);
    Serial.print(':');
    if(segundos < 10) Serial.print('0');
    Serial.print(segundos);
    Serial.println();
   
   */
    horaSaida = hour(epochSaida);
    minutoSaida = minute(epochSaida);
    
    Serial.println();
    //Serial.println("Hora de saida: ");    
    Serial.print(horaSaida);    
    Serial.print(':');    
    if(minutoSaida < 10) Serial.print('0');
    Serial.print(minutoSaida);
    Serial.print(':');
    if(segundos < 10) Serial.print('0');
    Serial.print(segundos);
    Serial.println();
    
    
  }
  
  
  
}

/*
int lePinoRele(int pino){
    //Leitura do rele necessita de um delay.
    //Eh o que essa funcao faz.
    int i;
    
    for(i=0;i<20;i++){      
        if (digitalRead(pino) == HIGH)           
           return HIGH;        
        delay(10);
    }
    //digitalWrite(pino,LOW);
    return LOW;     
}*/


void observa(int segundos) {
  int i;  
  const int DELAY = 1000; //sub-intervalo de observacao em ms

  contador=0;  
  
  //observa, em intervaloes de 1s, durante o tempo fornecido
  for(i=0;i<segundos*(1000/DELAY);i++){
    pinMode(CHAVE,INPUT);
    VALOR_PINO = digitalRead(CHAVE);
     pinMode(CHAVE,OUTPUT);
    digitalWrite(CHAVE,LOW); 
    if (VALOR_PINO == LOW){
      contador++; //presenca/deteccao: incrementa contador de presenca
    }
     
    Serial.println(contador);  
    delay(DELAY);
  }
}

void piscaLedWarning(){
  digitalWrite(LED_WARNING,LOW);
  delay(1000);
  digitalWrite(LED_WARNING,HIGH);
  delay(1000); 
}

void setup(){
  
  int i=0;
  
  pinMode(CHAVE,INPUT);
  pinMode(LED_WARNING,OUTPUT); 
  
  
  Serial.begin(9600);
   while (!Serial) {
    ; // espera porta serial.
  }

  // Inicia conexao ethernet
  if (Ethernet.begin(mac) == 0) {
    //Sem o ethernet, nao faz sentido continuar a aplicacao.
    //Serial.println("Falha no DHCP\nConecte a rede e reinicie");
    while(1)piscaLedWarning(); //pisca ateh reiniciar o dispositivo
    
      
      
  }
  // 1 s para o shield ethernet inicializar
  delay(1000);  
  
  
}


void loop(){
  
  observa(segundosObservacao);
  
  if (contador >= minimoDeteccoes){
    Serial.println("P\n");
    post(); // posta na internet
  }else {
    Serial.println("NP\n");
  } 
  
}
