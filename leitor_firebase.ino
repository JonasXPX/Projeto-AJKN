#include <Adafruit_Fingerprint.h>

#include <ESP8266HTTPClient.h>
#include <ESP8266WiFi.h>

#include <Wire.h>
#include <LiquidCrystal_I2C.h>


SoftwareSerial            serial(14, 12); //D5 D6
Adafruit_Fingerprint finger = Adafruit_Fingerprint(&serial);

const String address  =  "192.168.43.78";
LiquidCrystal_I2C        lcd(0x27, 16, 2);

String msgDeErro;
String bemVindoMsg;

void setup() {
  Serial.begin(9600);
  finger.begin(57600);
  lcd.init();
  lcd.backlight();
  while (1) {
    if (finger.verifyPassword()) { 
      setMensagem(0,0, "Leitor biometrico encontrado");
      break;
    }
    else {
      Serial.println("Leitor Biometrico nao encontrado");
    }
  }
  WiFi.begin("AJKN", "hackme");

  setMensagem(0,0, "Conectando...");
  
  Serial.println("Conectando à rede");
  while (WiFi.status() != WL_CONNECTED) {
    delay(100);
    Serial.print(".");
  }

  String ip = WiFi.localIP().toString();
  setMensagem(0,0, "Conectado! IP: " + ip);
  
  Serial.print("Conectado! Seu IP é:");
  Serial.println(WiFi.localIP());

  bemVindoMsg = getMensagemFromServer("bemvindo");
 // cadastrar = getMensagemFromServer("cadastrar");
}

void loop() {
  int saida = aguardarIdParaRegistro();
  if (saida == -1) {

  }
  Serial.print("> " + String(saida));
  if (saida >= 1) {
    int error;
    int out = registrar(saida);
    Serial.print("Registro concluido: ");
    Serial.println((error = getErrorByCode(out)));
    if(msgDeErro != ""){
      setMensagem(0,0, msgDeErro);
      delay(2000);
    }
    msgDeErro = "";
    if (error == 0) {
      enviarRegistro(saida);
      setMensagem(0, 0, "CADASTRO CONCLUIDO");
      Serial.println("Registro enviado.");
      delay(1000);
    }
  }

  int fingerID = getFinger();
  if (fingerID != -1) {
    sendReadFinger(fingerID);
  }
  setMensagem(0, 0, bemVindoMsg);
  delay(500);
}


int getFinger() {
  uint8_t p = finger.getImage();
  if (p != FINGERPRINT_OK)  return -1;

  p = finger.image2Tz();
  if (p != FINGERPRINT_OK)  return -1;

  p = finger.fingerFastSearch();
  if (p != FINGERPRINT_OK)  return -1;

  return finger.fingerID;
}

/**
 * Caso leia uma digital,
 * Enviar o ID capturado para 
 * o servidor.
 * Para fazer logs, e registrar 
 * a entrada do aluno
 */
int sendReadFinger(int id) {
  HTTPClient http;
  http.begin("http://" + String(address) + "/api/senddata/" + String(id));
  http.addHeader("Content-Type", "application/json");
  int httpCode = http.POST("{\"id\": \"" + String(id) + "\"}");
  http.end();
  return httpCode;
}

/**
 * Enviar o cadastro do ID da digital 
 * Para o servidor
  */
String enviarRegistro(int id) {
  HTTPClient http;
  http.begin("http://" + String(address) + "/api/registrar/" + String(id));
  http.addHeader("Content-Type", "application/json");
  http.POST("");
  return http.getString();
}

/**
  * Fica escutando o servidor,
  * Caso o adminstrador envie uma requisição para
  * cadastrar uma nova digital.
*/
int aguardarIdParaRegistro() {
  String saida = "-1";
  HTTPClient http;
  http.begin("http://" + String(address) + "/api/registrar");
  int code = http.GET();
  if (code < 0) {
    return -1;
  }

  String retorno = http.getString();
  http.end();
  return retorno.toInt();
}

int registrar(int saida) {
  setMensagem(0, 0, "COLOQUE SUA DIGITAL NO LEITOR");
  Serial.print("Registrando Digital na ID ");
  Serial.println(saida);
  int p = -1;
  while (p != FINGERPRINT_OK) {
    p = finger.getImage();
    switch (p) {
      case FINGERPRINT_OK:
        break;
    }
  }
  p = finger.image2Tz(1);
  switch (p) {
    case FINGERPRINT_OK:
      break;
    default:
      Serial.println("Erro image2Tz(1)");
      return p;
  }
  Serial.println("Retire o dedo");
  setMensagem(0,0, "RETIRE O DEDO");
  delay(3000);
  p = 0;
  while (p != FINGERPRINT_NOFINGER) {
    p = finger.getImage();
  }
  p = -1;
  Serial.println("Coloque o mesmo dedo novamente");
  setMensagem(0,0, "INSIRA O DEDO");
  while (p != FINGERPRINT_OK) {
    p = finger.getImage();
    switch (p) {
      case FINGERPRINT_OK:
        break;
      case FINGERPRINT_NOFINGER:
        break;
      default:
        Serial.println("Error getImage(2)");
        break;
    }
  }

  p = finger.image2Tz(2);

  switch (p) {
    case FINGERPRINT_OK:
      break;
    default:
      Serial.println("Error image2Tz(2)");
      return p;
  }
  p = finger.createModel();
  if (p != FINGERPRINT_OK) {
    Serial.println("Error on createModel()");
    return p;
  }

  p = finger.storeModel(saida);
  if (p != FINGERPRINT_OK) {
    Serial.println("Error on storeModel()");
    return p;
  }
}

int getErrorByCode(int code) {
  int error = 0;
  switch (code) {
    case FINGERPRINT_IMAGEMESS:
      Serial.println("Imagem muito confusa");
      msgDeErro = "Imagem muito confusa";
      error = 1;
      break;
    case FINGERPRINT_PACKETRECIEVEERR:
      Serial.println("Erro ao se comunicar");
      msgDeErro = "Erro ao se comunicar";
      error = 1;
      break;
    case FINGERPRINT_FEATUREFAIL | FINGERPRINT_INVALIDIMAGE:
      Serial.println("Nao foi possível encontrar características da impressao digital");
      msgDeErro = "Digital sem nao encontrada.";
      error = 1;
      break;
    case FINGERPRINT_ENROLLMISMATCH:
      Serial.println("Digital nao corresponde");
      msgDeErro = "Digital nao corresponde";
      error = 1;
      break;
    case FINGERPRINT_BADLOCATION:
      Serial.println("Impossível Armazenar dados");
      msgDeErro = "Impossível Armazenar dados";
      error = 1;
      break;
    case FINGERPRINT_FLASHERR:
      Serial.println("Erro ao salvar na memoria");
      msgDeErro = "Erro ao salvar na memoria";
      error = 1;
      break;
    case FINGERPRINT_IMAGEFAIL:
      Serial.println("Erro ao capturar imagem");
      msgDeErro = "Erro ao capturar imagem";
      error = 1;
      break;
  }
  return error;
}


String getMensagemFromServer(String param) {
  HTTPClient http;
  http.begin("http://" + String(address) + "/api/getmensagem/" + String(param));
  http.GET();
  String msg = http.getString();
  http.end();
  return msg;
}

void setMensagem(int pos1, int pos2, String msg) {
     limparLcd();
     String msg1, msg2;
     if(msg.length() >= 16) {
      msg1 = msg.substring(0, 15);
      msg2 = msg.substring(15, msg.length());      
     } else {
        msg1 = msg;
     }
     
     lcd.setCursor(pos1,pos2);
     lcd.print(msg1);
     
     if(msg.length() >= 16) {
      lcd.setCursor(0, 1);
      lcd.print(msg2);
     }
}

void limparLcd(){
   lcd.setCursor(0,0);
     lcd.print("                ");
     delay(25);
     lcd.setCursor(0,1);
     lcd.print("                ");
     delay(25);
}
