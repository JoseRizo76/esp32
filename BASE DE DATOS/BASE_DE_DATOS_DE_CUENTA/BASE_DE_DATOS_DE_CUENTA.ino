 //DECLARACION DE LIBRERIA
#include <SPI.h>
#include <MFRC522.h>
#include <LiquidCrystal_I2C.h>
#include <Keypad.h>
#include <EEPROM.h> 
#include <Preferences.h>

Preferences preferences;


//CONFIGURACION DE EL RFID
#define RST_PIN 5
#define SS_PIN 16
MFRC522 mfrc522(SS_PIN, RST_PIN);

//CONFIGURACION DE LA PANTALLA LCD
LiquidCrystal_I2C lcd (0x27, 16, 2);

//CONFIGURACION DE EL TECLADO MATRICIAL
const byte ROWS = 4;
const byte COLS = 3;
char keys[ROWS][COLS] = {
  {'1', '2', '3'},
  {'4', '5', '6'},
  {'7', '8', '9'},
  {'*', '0', '#'}
};
byte rowPins[ROWS] = {12, 26, 14, 17};
byte colPins[COLS] = {13, 4, 2};
Keypad keypad = Keypad( makeKeymap(keys), rowPins, colPins, ROWS, COLS );

//DECLARACION DE VARIABLES DE MANERA GLOBAL
  //VARIABLES DE CLIENTE
  unsigned int contador ;
  byte estado = 0 ;
  boolean fin = false;
 
  // VARIABLES DE ALMACENAMIENTO DE TECLA PRESIONADA
  byte indice = 0;
  char dato [10];

  //VARIABLES DE OPCIONES DE MENU
  byte opc = 0; // MENU PRINCIPAL

  //VARIABLES DE TIEMPO 
  unsigned long tiempo = 0;
  int segundos = 0;

  boolean guardarEnPreferences = false;

  // VARIABLES DE TIPO BANDERA
  char band = 'f';
  char band2 = 'v';
  char band3 = 'f';
  char band4 = 'f';
  char band5 = 'f';

  //VARIABLES PARA LA VERIFICACION DEL DUEÑO
  char clave [7] = "000000";
  boolean Cverificacion = false;
  char clavei [7];
  char clavePrint[7] = "111111";
  boolean CverificacionPrint = false; 

//ESTRUCTURA
struct cliente {
  int id = 0;
  char password [5] = "";
  int dinero = 100;
  char Tarjeta [30] = "";
  char primerDigito ;
} cliente [10];

void setup() {
  //INICIALIZA EL COMUNICADOR SERIAL A 115200 BAUDIOS
  Serial.begin(115200);
  
  preferences.begin("my-app", false);
  contador = preferences.getUInt("contador", 0);

  // Cerrar las preferencias
  preferences.end();

  //VOIDS
  RFID ();
  lcdInit();

  //DECLARACION DE PINES DE SALIDA
  pinMode (15, OUTPUT);

  //DECLARACION DE PINES DE ENTRADA

  if (contador == 0){
    cr();
    lcd.print("REQUID ACCOUNT      ");
    delay(500);
  }
}
  
void loop() {

  
  tiempo = millis();
  segundos = (tiempo / 1000 );
  
  //Si al inicio del programa no hay ninguna cuenta pedira qeu se ingrese una cuenta ademas de la verificacion de la contraseña
  if (contador ==0 && opc == 0 && band3 == 'f' || Cverificacion == false && opc == 2 && band3 == 'f'){
    mensaje_verificacion();
  }

  //Verifica la opcion que el cliente desea realizar
  opcion_menu_principal();
  
  //Verificacion de la clave maestra
  verificacion_clave_maestra ();

  // Registro de cuentas 
    if (opc == 2 && contador<=9 && Cverificacion == true|| contador == 0 && Cverificacion == true){
    nuevaCuenta();
    }
    else if (opc == 5){  //si los usuarios estan completos no dejara que se pueda agregar otro mas 
      lcd.clear();
      cr();
      Serial.print("USERS COMPLETED");
      lcd.print("USERS COMPLETED");
      estado = 1;
      delay(1000);
      opc = 0;
    }

    if (CverificacionPrint == true ){
      for (byte i=0; i<contador; i++){
        Serial.print("Ciente ");
        Serial.println(i+1);
        Serial.print("USUARIO : ");
        Serial.println(cliente[i].id);
        Serial.print("CONTRASEÑA : ");
        Serial.println(cliente[i].password);
        Serial.print("DINERO : ");
        Serial.println(cliente[i].dinero);
        Serial.print("TARJETA : ");
        Serial.println(cliente[i].Tarjeta);
        Serial.print("NFC : ");
        Serial.println(cliente[i].primerDigito);
        Serial.println("");
        Serial.println("");
        
      }
      CverificacionPrint = false; 
      preferences.begin("my-app", false);
      preferences.clear();
      preferences.end();
    }



  if (guardarEnPreferences) {
  preferences.begin("my-app", false);
  preferences.putUInt("contador", contador);
  preferences.end();
  guardarEnPreferences = false;
  }
}







////////////////////////////////////////////////////////////// <<<<< VOID >>>>> ////////////////////////////////////////////////////////////

///////// INICIALIZACION DE LA LCD /////////
void lcdInit() {
  lcd.init();
  lcd.clear();
  lcd.backlight();
  lcd.setCursor(0, 0);
  lcd.print("READING...");
  delay (500);
}

///////// VOID  INICIA LA LA LECTURA DEL RFID //////////
void RFID () {
  SPI.begin();
  mfrc522.PCD_Init();
}

// VOID PARA LA POSICION DE LA PANTALLA LCD
  //VOID PARA LA PRIMERA LINEA DE LA LCD
  void cr() {
    lcd.setCursor(0, 0);
  }
  //VOID PARA SEGUNDA LINEA DE LA LCD
  void cl() {
    lcd.setCursor(0, 1);
  }

//// CREACION DE NUEVA CUENTA DE USUARIO DE MANERA SENCILLA SIN NFC O TARJETA////
void nuevaCuenta (){ 
  //PEDIRA EL ID DE LA CUENTA 
  if (contador == 0 && band == 'f' || opc == 2 && band == 'f') {
    cr(); 
    lcd.print("INGRESE ID 4DIG  "); 
    cl();
    lcd.print("ID: ");
    band = 'v';
    Serial.print("Cliente ");
    Serial.println(contador+1);
  }
  //GUARDA EN LA ESTRUCTURA EL ID DE LA CUENTA 
  if (contador == 0 && band == 'v' || opc == 2 && band == 'v'){
   char key = keypad.getKey();
   //BUCLE HASTA QUE EL ID ESTE EN EL RANGO DE 1000 - 9999
   do {
    if (key){
        dato [indice] = key;
        lcd.print(key);
        indice++;
        cliente[contador].id = atoi(dato);
        dato[indice] = '\0';

    }
    //CUANDO SE INGRESE EL 4 DIGITO SE GURADARA AUNTOMATICAMENTE EN EL ID
    if (indice == 4 && cliente[contador].id >= 1000){
        
        indice=0;
        band = 'p'; 
        lcd.clear();
      }
    // EL ID DEBE DE ESTAR EN EL RANGO DE 1000 A 9999
    else if (indice == 4 && cliente[contador].id < 1000){
      indice = 0;
      cr();
      lcd.print("ERROR ID < 1000");
      delay(500);
      band = 'f';
      lcd.clear();
    }
   }while (cliente[contador].id >= 1000 && band == 'f');
}
  //PEDIRA LA CONTRASEÑA DEL CLIENTE CON UN TOTAL DE 4 CARACTERES
  if (contador == 0 && band == 'p' || opc == 2 && band == 'p' ) {
    cr ();
    lcd.print("PASSWORD      ");
    delay(500);
    cr();
    lcd.print("INGRESE PASSWORD ");  
    cl();
    lcd.print("PASS: ");
    band = 'a';
    indice = 0 ;
  }

  //GUARDARA LA CONTRASEÑA DENTRO DE PASSWORD DEL CLIENTE
  if (contador == 0 && band == 'a' || opc == 2 && band == 'a'){
    char key = keypad.getKey();
    if (key){
        dato [indice] = key;
        lcd.print(key);
        indice++;   
    }
    //UNA VES INGRESADO LOS CUATRO DIGITOS IMPRIME EN EL MONITOR SERIAL LOS DATOS DEL CLIENTE
    if (indice == 4){
        strncpy(cliente[contador].password, dato, sizeof(cliente[contador].password) - 1);
        cliente[contador].password[sizeof(cliente[contador].password) - 1] = '\0';
        indice = 0;
        lcd.clear(); 
        band = 't';
   }
  }

   // VERIFICACION SI TIENE TARJETA
   if (contador == 0 && band == 't' || opc == 2 && band == 't'){
      char key = keypad.getKey();
      char banda = 'f';
      //char bandaa = 'f';
      if (banda =='f'){
        cr();
        lcd.print("TARJETA RFID?");
        cl();
        lcd.print("SI-> *  NO-> #");
        band3 = 'v';
      }
   }

   if (contador == 0 && band == 't' && band3 == 'v' || opc == 2 && band == 't' && band3 == 'v') {
    char key = keypad.getKey();                                                  
    if (key){
         switch (key){
          case '*': band ='q'; lcd.clear(); break;
          case '#': Serial.println("TARJETA NO IDENTIFICADA"); band ='n'; strcpy (cliente[contador].Tarjeta, "NO REGISTRADO"); lcd.clear(); break;
          default: Serial.println("ERROR"); break;
         }
    }

    if (contador == 0 && band == 'q' && band3 == 'v' || opc == 2 && band == 'q' && band3 == 'v') {
      tarjeta();
    }

    
   }
   // VERIFICACION SI TIENE NFC
   if (contador == 0 && band == 'n' || opc == 2 && band == 'n'){
      char key = keypad.getKey();
      char banda = 'f';
      char bandaa = 'f';
      if (banda =='f'){
        cr();
        lcd.print("NFC?");
        cl();
        lcd.print("SI-> *  NO-> #");
        band3 = 'v';
      }
   }

   if (contador == 0 && band == 'n' && band3 == 'v' || opc == 2 && band == 'n' && band3 == 'v') {
    char key = keypad.getKey();                                                  
    if (key){
         switch (key){
          case '*':  band = 'e' ;break;
          case '#': Serial.println("NFC NO IDENTIFICADO"); cliente[contador].primerDigito = 'N'; band ='z'; fin = true; break;
          default: Serial.println("ERROR"); break;
         }
    }
   }

    if (contador == 0 && band == 'e' && band3 == 'v' || opc == 2 && band == 'e' && band3 == 'v') {
      nfcEscaner();
    }
   
   if (band =='z' && fin == true ){
        Serial.print("ID: ");
        Serial.println(cliente[contador].id);
        Serial.print("PASSWORD: "); 
        Serial.println(cliente[contador].password);
        Serial.print("DINERO: ");
        Serial.println(cliente[contador].dinero);
        Serial.print("TARJETA: ");
        Serial.println(cliente[contador].Tarjeta);
        Serial.print("PRIMER DIGITO DE LA TARJETA: ");
        Serial.println(cliente[contador].primerDigito);
        Serial.println(" ");
        indice=0;
        band = 'f';
        contador++;
        lcd.clear(); 
        band2 = 'v';
        fin = false;
        band3 = 'f';
        opc = 0; 
        guardarEnPreferences = true;
  }
  

if(!strcmp(cliente[(contador-1)].password, "2020")){
  digitalWrite(15,HIGH);
 }
}

//////// VERIFICACION DE LA CLAVE PARA PODER RECARGAR O INGRESAR NUEVA CUENTA
 void verificacion_clave_maestra (){
  if (Cverificacion == false && opc == 2 && band3 == 'v' ){
    char key = keypad.getKey();
    if (band2 =='v'){
      cr(); 
      lcd.print("PASSWORD MASTER");
      cl();
      lcd.print("KEY: ");
      band2 = 'f';
      indice = 0 ;
    }
    //  MUESTRA EN LA PANTALLA LA CONTRASEÑA PERO DE MANERA DE XXXXXXX
    if (key){
        clavei [indice] = key;
        lcd.print("X");
        indice++;
    }
    // CUANDO LA CONTRASEÑA TENGA UN RANGO DE 6 DIGITOS LA COMPARA CON LA CONTRASEÑA MAESTRA
    if (indice == 6 && Cverificacion == false ){
        lcd.clear();
        if (!strcmp(clavei,clave)){
          Cverificacion = true; 
          indice = 0;
        }
        else if (!strcmp(clavei,clavePrint)){
          CverificacionPrint = true; 
          indice = 0;
          opc = 0;
          band2 = 'v';
          band3= 'f';
        }
        
        // SI LA CONTRASEÑA NO COINCIDE ENTONCES DICE QUE LA CLAVE ES INCORRECTA Y VUELVE A SU MENU PRINCIPAL
        else{
          lcd.clear();
          cr();
          lcd.print("CLAVE INCORRECTA");
          delay (500);
          opc = 0;
          lcd.clear();
          indice = 0;
          band2 = 'v';
          band3= 'f';
        }
      }  
    }
  }

// MENU PRINCIPAL DE LAS ACCIONES QUE SE PUEDEN HACER
  void opcion_menu_principal(){
    if (opc == 0 && contador > 0 )  {
    char key = keypad.getKey();
      cr();
      lcd.print("PRESS-- RECG->#");
      cl();
      lcd.print("AVAN->0 NEW->*");
      estado = 0;
    //VERIFICACION DE LA ACCION QUE QUIERE HACER EL CLIENTE   
    if (key){
      switch (key){
        // EN CASI DE QUE QUIERA RECARGAR UNA NUEVA CUENTA
        case '#': opc = 3 ;
          lcd.clear();
          break;
        // EN CASO DE QUE QUIERA AGREGAR UNA NUEVA CUENTA  
        case '*': 
          if (contador <=9){
            opc = 2;
            Cverificacion = false;
            band3 = 'f';
          }
          else{
            opc = 5 ;
          }
          lcd.clear();
          break;
        // EN CASO DE QUE QUIERA PROCEDER A RETIRAR UN  PRODUCTO
        case '0': 
          opc = 1 ;
          lcd.clear();
          break;
        //EN CASO DE QUE PRESIONE UN TECLA QUE NO ESTE EN EL MENU
        default: lcd.clear();
          cr();
          lcd.print("ERROR DE OPCION");
          delay(500);
          break;
      }
    }
  } 
}

//Mensaje Verificacion
void mensaje_verificacion(){
    opc = 2;
    Cverificacion = false;
    indice = 0;
    lcd.clear();
    cr();
    lcd.print("VERIFICACION");
    delay (300);
    band3 = 'v';
}
////////////////////////////////////////////

void tarjeta (){
  while (band == 'q'){
    if (band5 == 'f') {
      lcd.clear();
      lcd.setCursor(0, 0);
      lcd.print("ESCANEE LA TARJETA");
      lcd.setCursor(0, 1);
      lcd.print("WAINTING...");
      band5 = 'g';
    }

    if (mfrc522.PICC_IsNewCardPresent() && mfrc522.PICC_ReadCardSerial()) {
      String idTarjeta = "";
      lcd.clear();
      for (byte i = 0; i < mfrc522.uid.size; i++) {
        idTarjeta += String(mfrc522.uid.uidByte[i], HEX);
      }
      idTarjeta.toUpperCase(); // Convierte a mayúsculas (opcional)
      // Almacena el ID de la tarjeta en la estructura cliente
      strcpy(cliente[contador].Tarjeta, idTarjeta.c_str());
      mfrc522.PICC_HaltA();
      mfrc522.PCD_StopCrypto1();
      band = 'n';
      lcd.clear();
      band5 = 'f';
    }
  }
}

// VOID DEL ESCANEO NFC ///
void nfcEscaner(){
 while (band == 'e') {
    if (band5 == 'f') {
      lcd.clear();
      lcd.setCursor(0, 0);
      lcd.print("ESCANEE NFC");
      lcd.setCursor(0, 1);
      lcd.print("WAINTING...");
      band5 = 'g';
    }

    if (mfrc522.PICC_IsNewCardPresent() && mfrc522.PICC_ReadCardSerial()) {
      String idTarjeta = "";
      lcd.clear();
      for (byte i = 0; i < mfrc522.uid.size; i++) {
        idTarjeta += String(mfrc522.uid.uidByte[i], HEX);
      }

      idTarjeta.toUpperCase(); // Convierte a mayúsculas (opcional) 

      // Almacena el NFC IDENTIFICATOR
      cliente[contador].primerDigito = idTarjeta[0];

      mfrc522.PICC_HaltA();
      mfrc522.PCD_StopCrypto1();
      lcd.clear();
      band5 = 'f';
      band ='z' ;
      fin = true ;  
    }
  } 
}
