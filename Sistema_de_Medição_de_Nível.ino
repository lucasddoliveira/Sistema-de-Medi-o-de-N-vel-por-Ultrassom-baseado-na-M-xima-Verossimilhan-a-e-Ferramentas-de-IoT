/****************************************
 * Mapeamento dos Pinos utilizados
 ****************************************/
/* * Notepad dos pinos INPUT_PULLUP
 * 14 - DAT Temperature/Humidity
 * 16 - Echo 1
 * 17 - Trig 1
 * 18 - Echo 2
 * 19 - Echo 3
 * 21 - SDA LCD   //I2C address: 63 (0x3F)
 * 22 - SCL LCD
 * 23 - Trig 2
 * 
 * * Notepad dos pinos sem pullup
 * 13 - 
 * 25 - 
 * 26 - 
 * 27 - 
 * 32 - Trig 3
 * 33 - 
 * 
 * * Notepad dos pinos input only
 * 35 - Pushbutton
 */

/****************************************
 * Include Libraries
 ****************************************/
#include <WiFi.h>
//#include <PubSubClient.h>
#include <Ultrasonic.h>
#include "DHT.h"
#include <LiquidCrystal_I2C.h>
#include <ModbusIP_ESP8266.h>

/****************************************
 * WiFi and MQTT credentials
 ****************************************/
//#define WIFISSID "DECACTJ212" // Put your WifiSSID here
//#define PASSWORD "civilamb04" // Put your wifi password here
#define WIFISSID2 "TP-LINK_9291E8"
#define PASSWORD2 ""

//------ Definições do ModBus IP --------//
const int SENSOR_NIVEL = 10;
const int SENSOR_TEMP = 20;

ModbusIP mb;

/****************************************
 * Define UbiDots Constants
 ****************************************/
/*
#define UD_ECHO_1 "echo-1" // Assign the variable label
#define UD_ECHO_2 "echo-2" // Assign the variable label
#define UD_ECHO_3 "echo-3" // Assign the variable label
#define UD_TANK "tank" // Assign the variable label
#define UD_DHT_T "dht-t" // Assign the variable label
#define UD_DHT_H "dht-h" // Assign the variable label
#define UD_DHT_HIC "dht-hic" // Assign the variable label
#define UD_DEVICE "esp32" // Assign the device label

/****************************************
 * Definição das Variáveis do Arduino
 ****************************************/
#define ECHO_1     16 // pinos dos sensores ultrassônicos
#define TRIGGER_1  17
#define ECHO_2     18
#define TRIGGER_2  23
#define ECHO_3     19
#define TRIGGER_3  32
#define DHTPIN 14     // pino do sensor de temperatura e humidade
#define DHTTYPE DHT11   // Type of the temperature/humidity sensor used (DHT 11)
#define PUSHBUTTON 35 // pino do botão

long tof = 0;  // Variável utilizada para a distância final (MLE)
long historico[30]; // Variável utilizada para memorizar o histórico
float distancia = 0;  // Distância final (MLE)
float gama = 0; //
float gama12 = 0;
float gama13 = 0;
float gama23 = 0;
float uf = 0; // Desvio padrão calculado durante a aplicação do MLE
float u[] = {0, 0, 0};  // Desvio padrão individual dos sensores
float tank = 0; // Nível do tanque calculado em relação a distância final
float vol = 0;
#define pi 3.1415926

/*
char mqttBroker[]  = "industrial.api.ubidots.com"; // servidor utilizado na parte IoT
//char mqttBroker[]  = "industrial.ubidots.com"; // servidor utilizado na parte IoT
char payload[100];  // variável utilizada para enviar mensagens ao servidor
char topic[150];  // tópico da mensagem a ser enviada
char topicSubscribe[100]; // tópico da mensagem a ser recebida/lida do servidor
char str_sensor[10];  // espaço reservado para a mensagem a ser enviada
*/ 
int loop_counter = 0; // contador de ciclos
// Cada ciclo leva aprox. 300ms
int w = 1000; // a cada 'w' ciclos, checa a conexão wifi
// 1000x300ms = 5 min
int k = 100000; // a cada 'k' ciclos, recalibra os sensores
// 100000x300ms = 8h20min

int j = 0; // pushbutton helping variable 
int buttonState = 0; // variable for reading the pushbutton status
int menu = 0; // posição atual do display
/****************************************
 * Funções de Inicialização
 ****************************************/
// Initialize ultrasonic sensors.
Ultrasonic ultrasonic1(TRIGGER_1, ECHO_1);
Ultrasonic ultrasonic2(TRIGGER_2, ECHO_2);
Ultrasonic ultrasonic3(TRIGGER_3, ECHO_3);
// Initialize DHT sensor.
DHT dht(DHTPIN, DHTTYPE);
// Set the LCD address for a 16 chars and 2 line display
// LCD on protoboard: 0x27 
// LCD on project: 0x3F
LiquidCrystal_I2C lcd(0x3F, 16, 2);
// Initialize the IoT communication
//WiFiClient ubidots;
//PubSubClient client(ubidots);

void connect_to_wifi() {
  if (WiFi.status() != WL_CONNECTED) {
    Serial.println();
    Serial.println("Trying to connect to WiFi 1...");
    WiFi.begin(WIFISSID2, PASSWORD2);
    Serial.println(WiFi.localIP());
    lcd.clear();
    lcd.print("Buscando");
    lcd.setCursor(0,1);
    lcd.print("WiFi...");  
    delay(500);
    lcd.clear();
    lcd.print("Buscando");
    lcd.setCursor(0,1);
    lcd.print("WiFi.");  
    delay(500);
    lcd.setCursor(0,1);
    lcd.print("WiFi..");  
    delay(500);
    lcd.setCursor(0,1);
    lcd.print("WiFi...");  
    delay(500);
    if (WiFi.status() != WL_CONNECTED) {
      lcd.clear();
      lcd.print("Falha na");
      lcd.setCursor(0,1);
      lcd.print("Conexao WiFi");  
      delay(600); 
    }
  }
  if (WiFi.status() == WL_CONNECTED) {
    Serial.println("");
    Serial.println("WiFi Connected");
    Serial.println("IP address: ");
    Serial.println(WiFi.localIP());
    lcd.clear();
    lcd.print("WiFi");
    lcd.setCursor(0,1);
    lcd.print("Conectado");  
    delay(600);
  }
}

void reconnect() {
  Serial.println("Attempting MQTT connection...");
  // Attemp to connect
  /*
  if (client.connect(MQTT_CLIENT_NAME, TOKEN, "")) {
    Serial.println("Connected");
  } else {
    Serial.print("Failed, rc=");
    Serial.print(client.state());
    Serial.println(" try again next iteration");
  }*/
}

/*
 * Get the mean from an array of longs
 */
float getMean(long * val, int arrayCount) {
  long total = 0;
  for (int i = 0; i < arrayCount; i++) {
    total = total + val[i];
  }
  float avg = total/(float)arrayCount;
  return avg;
}

/*
 * Get the standard deviation from an array of longs
 */
float getStdDev(long * val, int arrayCount) {
  float avg = getMean(val, arrayCount);
  long total = 0;
  for (int i = 0; i < arrayCount; i++) {
    total = total + (val[i] - avg) * (val[i] - avg);
  }

  float variance = total/(float)arrayCount;
  float stdDev = sqrt(variance);
  return stdDev;
}

long compararComHistorico (long * historico, long tof) {
 long maxValue = 0;
 for (int i=1; i<30; i++){
   if (maxValue<historico[i]){
     maxValue = historico[i];
   }
 }
 if ((maxValue-tof>120) || (tof>maxValue)) {
  return tof;
 } else {
  return maxValue;
 }
}

float calibrate(int sensor) {
  float mean;
  float std;
  if (sensor == 1) {
    long tof1[100];
    lcd.clear();
    lcd.print("Calibrando");
    lcd.setCursor(0,1);
    lcd.print("Sensores.");  
    Serial.println("");
    Serial.println("Calibrating Sensors...");
    for (byte i = 0; i < 100; i = i + 1) {
      tof1[i] = ultrasonic1.timing();
      delay(15);
    }
    mean = getMean(tof1, 100);
    std = getStdDev(tof1, 100);
    // print the output
    Serial.print("Mean 1: ");
    Serial.println(mean);
    Serial.print("Standard deviation 1: ");
    Serial.println(std);
    return std;
  }
  if (sensor == 2) {
    long tof2[100];
    lcd.clear();
    lcd.print("Calibrando");
    lcd.setCursor(0,1);
    lcd.print("Sensores..");  
    for (byte i = 0; i < 100; i = i + 1) {
      tof2[i] = ultrasonic2.timing();
      delay(15);
    }
    mean = getMean(tof2, 100);
    std = getStdDev(tof2, 100);
    // print the output
    Serial.print("Mean 2: ");
    Serial.println(mean);
    Serial.print("Standard deviation 2: ");
    Serial.println(std);
    return std;
  }
  if (sensor == 3) {
    long tof3[100];
    lcd.clear();
    lcd.print("Calibrando");
    lcd.setCursor(0,1);
    lcd.print("Sensores...");  
    for (byte i = 0; i < 100; i = i + 1) {
      tof3[i] = ultrasonic3.timing();
      delay(15);
    }
    mean = getMean(tof3, 100);
    std = getStdDev(tof3, 100);
    // print the output
    Serial.print("Mean 3: ");
    Serial.println(mean);
    Serial.print("Standard deviation 3: ");
    Serial.println(std);
    return std;
  }
}

float convert(long tof, float t){
  float cair = 20.05*sqrt(t+273.15);  // speed of sound for temperature T
  float distance = tof*cair/20000; // distance in centimeters
  return distance;
}
/****************************************
 * Main Functions
 ****************************************/
void setup() {
  Serial.begin(115200);
  pinMode(PUSHBUTTON,   INPUT);
  pinMode(34,   INPUT);
  pinMode(36,   INPUT);
  pinMode(39,   INPUT);
  lcd.init();  
  lcd.backlight();
  //client.setServer(mqttBroker, 1883);
  dht.begin();
  lcd.clear();
  connect_to_wifi();
  reconnect();

  mb.server(); //Servidor ModBus IP
  mb.addIreg(SENSOR_NIVEL);
  mb.addIreg(SENSOR_TEMP);

}

void loop() {
  /*
   * A cada w iteraçõpes, checa se a conexão wifi foi perdida
   * Caso tenha sido, reconecta
   */
   mb.task();
   
  if (loop_counter%w == 0) {
    if (WiFi.status() != WL_CONNECTED) {
      connect_to_wifi();
    }
  }

    
  /*
   * Em seguida checa se é necessário calibrar os sensores
   * Para calcular o desvio padrão de cada um e permitir
   * aplicar o MLE
   */
  if (loop_counter%k == 0) {
    u[0] = calibrate(1);
    u[1] = calibrate(2);
    u[2] = calibrate(3);
    gama = (1/(u[0]*u[0])+1/(u[1]*u[1])+1/(u[2]*u[2]));
    gama12 = (1/(u[0]*u[0])+1/(u[1]*u[1]));
    gama13 = (1/(u[0]*u[0])+1/(u[2]*u[2]));
    gama23 = (1/(u[1]*u[1])+1/(u[2]*u[2]));
    uf = sqrt(1/gama);
    Serial.print("Final standard deviation (uf): ");
    Serial.println(uf);
  }

  /*
   * Mede a temperatura, humidade e sensação térmica
   * E publica nos servidores da Ubidot
   */
  // Reading temperature or humidity takes about 250 milliseconds!
  // Sensor readings may also be up to 2 seconds 'old' (its a very slow sensor)
  float h = dht.readHumidity();
  float t = dht.readTemperature();  // Read temperature as Celsius (the default)
  // Check if any reads failed and write both variables as 0 to indicate an error.
  if (isnan(h) || isnan(t)) {
    Serial.println(F("Failed to read from DHT sensor!"));
    h = 0;
    t = 28.0;
  }
  // Compute heat index in Celsius (isFahreheit = false)
  float hic = dht.computeHeatIndex(t, h, false);

  Serial.print(F("Humidity: "));
  Serial.print(h);
  Serial.print(F("%  Temperature: "));
  Serial.print(t);
  Serial.print(F("°C  Heat index: "));
  Serial.print(hic);
  Serial.println(F("°C "));

  if (WiFi.status() == WL_CONNECTED) {
    /*if (!client.connected()) {
      reconnect();
    }
    sprintf(topic, "%s%s", "/v1.6/devices/", UD_DEVICE);
    sprintf(payload, "%s", ""); // Cleans the payload
    sprintf(payload, "{\"%s\":", UD_DHT_T); // Adds the variable label
    dtostrf(t, 4, 2, str_sensor); // 4 is mininum width, 2 is precision; float value is copied onto str_sensor
    sprintf(payload, "%s {\"value\": %s}}", payload, str_sensor); // Adds the value
    client.publish(topic, payload);
    client.loop();
    delay(10);
  
    sprintf(payload, "%s", ""); // Cleans the payload
    sprintf(payload, "{\"%s\":", UD_DHT_H); // Adds the variable label
    dtostrf(h, 4, 2, str_sensor);
    sprintf(payload, "%s {\"value\": %s}}", payload, str_sensor); // Adds the value
    client.publish(topic, payload);
    client.loop();
    delay(10);
  
    sprintf(payload, "%s", ""); // Cleans the payload
    sprintf(payload, "{\"%s\":", UD_DHT_HIC); // Adds the variable label
    dtostrf(hic, 4, 2, str_sensor);
    sprintf(payload, "%s {\"value\": %s}}", payload, str_sensor); // Adds the value
    client.publish(topic, payload);
    client.loop();
    delay(10);*/
  }
  
  /*
   * Mede a distancia de cada sensor
   * E publica nos servidores da Ubidot
   */
  
  long microsec1 = ultrasonic1.timing(); // Reading the time of flight
  float distance1 = convert(microsec1, t); // Computing distance knowing the temperature
  float nivel1 = 350 - distance1;
  Serial.print("(Sensor 1) us: ");
  Serial.print(microsec1);
  Serial.print(", cm: ");
  Serial.println(distance1);
  
  if (WiFi.status() == WL_CONNECTED) {
    /*if (!client.connected()) {
      reconnect();
    }
    sprintf(topic, "%s%s", "/v1.6/devices/", UD_DEVICE);
    sprintf(payload, "%s", ""); // Cleans the payload
    sprintf(payload, "{\"%s\":", UD_ECHO_1); // Adds the variable label
    dtostrf(nivel1, 4, 2, str_sensor);
    sprintf(payload, "%s {\"value\": %s}}", payload, str_sensor); // Adds the value
    client.publish(topic, payload);
    client.loop();
  */
  }
  delay(20);
  
  long microsec2 = ultrasonic2.timing();
  float distance2 = convert(microsec2, t);
  float nivel2 = 350 - distance2;
  Serial.print("(Sensor 2) us: ");
  Serial.print(microsec2);
  Serial.print(", cm: ");
  Serial.println(distance2);
  
  if (WiFi.status() == WL_CONNECTED) {
    /*if (!client.connected()) {
      reconnect();
    }
    sprintf(topic, "%s%s", "/v1.6/devices/", UD_DEVICE);
    sprintf(payload, "%s", ""); // Cleans the payload
    sprintf(payload, "{\"%s\":", UD_ECHO_2); // Adds the variable label
    dtostrf(nivel2, 4, 2, str_sensor);
    sprintf(payload, "%s {\"value\": %s}}", payload, str_sensor); // Adds the value
    client.publish(topic, payload);
    client.loop();
    */
  }
  delay(20);

  long microsec3 = ultrasonic3.timing();
  float distance3 = convert(microsec3, t);
  float nivel3 = 350 - distance3;
  Serial.print("(Sensor 3) us: ");
  Serial.print(microsec3);
  Serial.print(", cm: ");
  Serial.println(distance3);
  
  if (WiFi.status() == WL_CONNECTED) {
    /*
    if (!client.connected()) {
      reconnect();
    }
    sprintf(payload, "%s", ""); // Cleans the payload
    sprintf(payload, "{\"%s\":", UD_ECHO_3); // Adds the variable label
    dtostrf(nivel3, 4, 2, str_sensor);
    sprintf(payload, "%s {\"value\": %s}}", payload, str_sensor); // Adds the value
    client.publish(topic, payload);
    client.loop();
    */
  }

  /*
   * Após medir as distâncias individuais, identifica-se se houve falha em alguma medição
   * comparando os valores de cada sensor
   */
  if ((microsec1>microsec2) && (microsec1>microsec3)) {
    if ((abs(microsec1-microsec2)>=500) && (abs(microsec1-microsec3)>=500)) {
      tof = microsec1;
    } else if ((abs(microsec1-microsec2)<500) && (abs(microsec1-microsec3)>=500)) {
      tof = ((1/(gama12*u[0]*u[0]))*microsec1 + (1/(gama12*u[1]*u[1]))*microsec2);
    } else if ((abs(microsec1-microsec2)>=500) && (abs(microsec1-microsec3)<500)) {
      tof = ((1/(gama13*u[0]*u[0]))*microsec1 + (1/(gama13*u[2]*u[2]))*microsec3);
    } else {
      tof = ((1/(gama*u[0]*u[0]))*microsec1 + (1/(gama*u[1]*u[1]))*microsec2 + (1/(gama*u[2]*u[2]))*microsec3);
    }
  } else if (microsec2>microsec3) {
    if ((abs(microsec2-microsec1)>=500) && (abs(microsec2-microsec3)>=500)) {
      tof = microsec2;
    } else if ((abs(microsec2-microsec1)<500) && (abs(microsec2-microsec3)>=500)) {
      tof = ((1/(gama12*u[0]*u[0]))*microsec1 + (1/(gama12*u[1]*u[1]))*microsec2);
    } else if ((abs(microsec2-microsec1)>=500) && (abs(microsec2-microsec3)<500)) {
      tof = ((1/(gama23*u[1]*u[1]))*microsec2 + (1/(gama23*u[2]*u[2]))*microsec3);
    } else {
      tof = ((1/(gama*u[0]*u[0]))*microsec1 + (1/(gama*u[1]*u[1]))*microsec2 + (1/(gama*u[2]*u[2]))*microsec3);
    }
  } else {
    if ((abs(microsec3-microsec1)>=500) && (abs(microsec3-microsec2)>=500)) {
      tof = microsec3;
    } else if ((abs(microsec3-microsec1)<500) && (abs(microsec3-microsec2)>=500)) {
      tof = ((1/(gama13*u[0]*u[0]))*microsec1 + (1/(gama13*u[2]*u[2]))*microsec3);
    } else if ((abs(microsec3-microsec1)>=500) && (abs(microsec3-microsec2)<500)) {
      tof = ((1/(gama23*u[1]*u[1]))*microsec2 + (1/(gama23*u[2]*u[2]))*microsec3);
    } else {
      tof = ((1/(gama*u[0]*u[0]))*microsec1 + (1/(gama*u[1]*u[1]))*microsec2 + (1/(gama*u[2]*u[2]))*microsec3);
    }
  }
  
  /*
   * Em seguida realiza-se outra detecção de falhas comparando o valor final com o histórico de medições
   */
  if (loop_counter>=30) {
    tof = compararComHistorico(historico,tof);
  }
  historico[loop_counter%30] = tof;

  /*
   *  Após a detecção de falhas, aplica-se o MLE
   */
  if (loop_counter==0){
    distancia = convert(tof, t); // Primeiro valor de distância
  } else {
    distancia = 0.05*(convert(tof, t))+0.95*distancia; // Filtro digital para atenuar variações bruscas
  }
  Serial.print("(MLE) ToF(us): ");
  Serial.print(tof);
  Serial.print(", cm: ");
  Serial.println(distancia);
  Serial.println("");

  // Cálculo do nível e volume de água no tanque:
  tank = 350 - distancia;
  vol = (tank/100.0)*pi*0.785*0.785*1000.0;
  
  if (WiFi.status() == WL_CONNECTED) {
    Serial.println(WiFi.localIP());
    
    
    /*
    if (!client.connected()) {
      reconnect();
    }
    sprintf(payload, "%s", ""); // Cleans the payload
    sprintf(payload, "{\"%s\":", UD_TANK); // Adds the variable label
    dtostrf(tank, 4, 2, str_sensor);
    sprintf(payload, "%s {\"value\": %s}}", payload, str_sensor); // Adds the value
    client.publish(topic, payload);
    client.loop();
    */
  }

  /*
   * Exibe no display a informação desejada
   */
  if (menu == 0) {
    //lcd.clear();
    lcd.setCursor(0,0);  
    lcd.print("Nivel:        cm");
    lcd.setCursor(6,0);  
    lcd.print(tank);
    lcd.setCursor(0,1);
    lcd.print("Temp:          C");  
    lcd.setCursor(6,1);  
    lcd.print(t);
    lcd.setCursor(14,1);  
    lcd.write(223);
  }
  if (menu == 1) {
    //lcd.clear();
    lcd.setCursor(0,0);  
    lcd.print("Vol:           L");
    lcd.setCursor(6,0);  
    lcd.print(vol);
    lcd.setCursor(0,1);
    lcd.print("Temp:          C");  
    lcd.setCursor(6,1);  
    lcd.print(t);
    lcd.setCursor(14,1);  
    lcd.write(223);
  }
  if (menu == 2) {
    //lcd.clear();
    lcd.setCursor(0,0);  
    lcd.print("Nivel:        cm");
    lcd.setCursor(6,0);  
    lcd.print(tank);
    lcd.setCursor(0,1);  
    lcd.print("Vol:           L");
    lcd.setCursor(6,1);  
    lcd.print(vol);
  }

   int tankInt = int(tank*100); 
   int tempInt = int(t*100);
    
   mb.Ireg(SENSOR_NIVEL, tankInt);
   mb.Ireg(SENSOR_TEMP, tempInt);

  /*
   * Checa se o botão foi pressionado
   */
  for (byte i = 0; i < 10; i = i + 1) {
    buttonState = analogRead(PUSHBUTTON);
    delay(30);
    if (buttonState >4000) {
      j++;
    }
    else {
      j = 0;
    }
    if (j==8) {
      menu = menu + 1;
      if (menu == 3) {
        menu = 0;
      }
      delay((10-i)*30);
      break;
    }
  }
  
  //Serial.println("");
  loop_counter++;
}
