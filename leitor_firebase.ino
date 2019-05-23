#include <Adafruit_Fingerprint.h>

#include <ESP8266HTTPClient.h>
#include <ESP8266WiFi.h>
#include <ArduinoJson.h>
                     //D5 D6
SoftwareSerial serial(14, 12);
Adafruit_Fingerprint finger = Adafruit_Fingerprint(&serial);

const String address = "10.35.220.87";

void setup() {
  Serial.begin(9600);
  finger.begin(57600);
  while(1){
    if (finger.verifyPassword()) {
      Serial.println("Leitor Biometrico encontrado");
      break;
    }
    else {
      Serial.println("Leitor Biometrico nao encontrado");
    }
  }
  WiFi.begin("Campus Boulevard", "");

  Serial.println("Conectando à rede");
  while(WiFi.status() != WL_CONNECTED) {
    delay(100);
    Serial.print(".");
  }
  Serial.print("Conectado! Seu IP é: ");
  Serial.println(WiFi.localIP());
}

void loop() {
  int saida = lerRegistro();
  Serial.print("> " + String(saida));
  if(saida >= 1){
    int out = registrar(saida);
    Serial.print("Registro concluido, error code: ");
    int erro = getErrorByCode(out);
    Serial.println(erro);
    if(erro == 0){
      enviarRegistro(saida);
      Serial.println("Registro enviado.");
      delay(1000);
    }
  }
  int fingerID = getFinger();
  if(fingerID != -1) {
    sendData(fingerID);
  }
  delay(500);
}


int getFinger(){
  uint8_t p = finger.getImage();
  if (p != FINGERPRINT_OK)  return -1;

  p = finger.image2Tz();
  if (p != FINGERPRINT_OK)  return -1;

  p = finger.fingerFastSearch();
  if (p != FINGERPRINT_OK)  return -1;

  return finger.fingerID;
}

void sendData(int id){
    HTTPClient http;
    http.begin("http://"+ String(address) + "/api/senddata/" + String(id));
    http.addHeader("Content-Type", "application/json");
    int httpCode = http.POST("{\"id\": \"" + String(id) + "\"}");
    http.end();
    Serial.print("sendData(): ");
    Serial.println(httpCode);
    Serial.println(http.getString());
}


String enviarRegistro(int id){
  HTTPClient http;
  http.begin("http://" + String(address) + "/api/registrar/" + String(id));
  http.addHeader("Content-Type", "application/x-www-form-urlencoded");
  http.POST("");
  return http.getString();
}


int lerRegistro(){
  String saida = "-1";
  HTTPClient http;
  http.begin("http://" + String(address) +"/api/registrar");
  int code = http.GET();
  if(code < 0){
    return -1;
  }
  
  String retorno = http.getString();
  http.end(); 
  return retorno.toInt();
}

int registrar(int saida) {
  Serial.print("Registrando Digital na ID");
  Serial.println(saida);
  int p = -1;
  while(p != FINGERPRINT_OK){
    p = finger.getImage();
    switch(p){
      case FINGERPRINT_OK:
        break;  
    }
  }
  p = finger.image2Tz(1);
  switch(p){
    case FINGERPRINT_OK:
      break;
    default:
      Serial.println("Erro image2Tz(1)");
      return p;
  }
  Serial.println("Retire o dedo");
  delay(3000);
  p = 0;
  while(p != FINGERPRINT_NOFINGER){
    p = finger.getImage();
  }
  p = -1;
  Serial.println("Coloque o mesmo dedo novamente");
  while(p != FINGERPRINT_OK){
    p = finger.getImage();
    switch(p){
      case FINGERPRINT_OK:
        break;
      case FINGERPRINT_NOFINGER:
        break;
      default:
        Serial.println("Erro getImage(2)");
        break;
    }
  }

  p = finger.image2Tz(2);
  
  switch(p){
    case FINGERPRINT_OK:
      break;
    default:
      Serial.println("Erro image2Tz(2)");
      return p;
  } 
  p = finger.createModel();
  if(p != FINGERPRINT_OK){
    Serial.println("Erro on createModel()");
    return p;
  }

  p = finger.storeModel(saida);
  if(p != FINGERPRINT_OK){
    Serial.println("Erro on storeModel()");
    return p;
  }
    
}

int getErrorByCode(int code){
  int error = 0;
  switch(code){
    case FINGERPRINT_IMAGEMESS:
      Serial.println("Imagem muito confusa");
      error = 1;
      break;
    case FINGERPRINT_PACKETRECIEVEERR:
      Serial.println("Erro ao se comunicar");
      error = 1;
      break;
    case FINGERPRINT_FEATUREFAIL | FINGERPRINT_INVALIDIMAGE:
      Serial.println("Nao foi possível encontrar características da impressao digital");
      error = 1;
      break;
    case FINGERPRINT_ENROLLMISMATCH:
      Serial.println("Digital nao corresponde");
      error = 1;
      break;
    case FINGERPRINT_BADLOCATION:
      Serial.println("Impossível Armazenar dados");
      error = 1;
      break;
    case FINGERPRINT_FLASHERR:
      Serial.println("Erro ao salvar na memoria");
      error = 1;
      break;
    case FINGERPRINT_IMAGEFAIL:
      Serial.println("Erro ao capturar imagem");
      error = 1;
      break;
  }
  return error;
}
