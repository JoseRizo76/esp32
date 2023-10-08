// DECLARACION DE LIBRERIA
#include <SPI.h>
#include <MFRC522.h>
#include <LiquidCrystal_I2C.h>
#include <Keypad.h>
#include <Preferences.h>

// INSTANCIA PARA GUARDAR LOS DATOS EN LA NVS DE LA ESP32
Preferences preferences;

// CONFIGURACION DE EL RFID
#define RST_PIN 5
#define SS_PIN 16
MFRC522 mfrc522(SS_PIN, RST_PIN);

// CONFIGURACION DE LA PANTALLA LCD
LiquidCrystal_I2C lcd(0x27, 16, 2);

// CONFIGURACION DE EL TECLADO MATRICIAL
const byte ROWS = 4;
const byte COLS = 3;
char keys[ROWS][COLS] = {
    {'1', '2', '3'},
    {'4', '5', '6'},
    {'7', '8', '9'},
    {'*', '0', '#'}};
byte rowPins[ROWS] = {12, 26, 14, 17};
byte colPins[COLS] = {13, 4, 2};
Keypad keypad = Keypad(makeKeymap(keys), rowPins, colPins, ROWS, COLS);

// DECLARACION DE VARIABLES DE MANERA GLOBAL

// VARIABLES DE CLIENTE
unsigned int contador;
byte estado = 0;
boolean fin = false;
int user = 0;
byte i = 0;
byte numCliente = 0;
char VT [30] = "";

// VARIABLES DE ALMACENAMIENTO DE TECLA PRESIONADA
byte indice = 0;
char dato[10];
byte indiceD = 0;
char dineroSave[5];
int dineroint = 0;
char idVF[5];
char passVF [5];

//VARIABLES PAGO
boolean cuentaExistente = false;

//VARIABLES DE RETIRO DE PRODUCTO Y VERIFICACION DE LA CUENTA
boolean retiroProducto = false;

// VARIABLES DE OPCIONES DE MENU
byte opc = 0; // MENU PRINCIPAL
byte subopc = 0;
boolean estado1 = false;
boolean estadoVC = false;
 

// VARIABLES DE TIEMPO
unsigned long tiempo = 0;
int segundos = 0;
int tiempo_actual = 0;
int tiempo_anterior = 0;

/// VARIABLE QUE PERMITE GUARDAR DATOS EN LA NVS DE LA ESP32
boolean guardarEnPreferences = false;

// VARIABLES DE TIPO BANDERA
char band = 'f';
char band2 = 'v';
char band3 = 'f';
char band4 = 'f';
char band5 = 'f';
char band6 = 'f';
char band7 = 'f';
char band8 = 'f';
char bandVC = 'g';
char band9 = 'g';
char bandx = 'g';

// VARIABLES PARA LA VERIFICACION DEL DUEÑO
char clave[7] = "000000";
boolean Cverificacion = false;
char clavei[7];
char clavePrint[7] = "111111";
boolean CverificacionPrint = false;

// ESTRUCTURA QUE ALMACENA LOS DATOS DEL CLIENTE
struct cliente
{
  int id = 0;
  char password[5] = "";
  int dinero = 100;
  char Tarjeta[30] = "";
  char primerDigito;
} cliente[10]; // MAXIMO DE 10 CLIENTES POR EL MOMENTO

void setup()
{
  // INICIALIZA EL COMUNICADOR SERIAL A 115200 BAUDIOS
  Serial.begin(115200);

  // CARGA LOS DATOS ALMACENADOS DESDE NVS DEL ESP32
  preferences.begin("my-app", false);
  contador = preferences.getUInt("contador", 0);
  for (byte i = 0; i < contador; i++)
  {
    String key = "cliente" + String(i);
    preferences.getBytes(key.c_str(), &cliente[i], sizeof(cliente[i]));
  }
  preferences.end();

  // VOIDS
  RFID();
  lcdInit();

  // DECLARACION DE PINES DE SALIDA
  pinMode(15, OUTPUT);

  // DECLARACION DE PINES DE ENTRADA

  // MENSAJE SI NO HAY NINGUNA CUENTA EN LA MEMORIA
  if (contador == 0)
  {
    cr();
    lcd.print("REQUID ACCOUNT      ");
    delay(500);
  }
}

void loop()
{

  // VARIABLES QUE ALMACENAN EL TIEMPO DESDE QUE SE ENCENDIO EL ESP32
  tiempo = millis();
  segundos = (tiempo / 1000);

  // SI DETECTA QUE NO HAY NINGUNA CUANTA EN LA ESTRUCUTA CLIENTE PEDIRA QUE SE INGRESE UNA CUENTA OBLIGATORIAMENTE
  if (contador == 0 && opc == 0 && band3 == 'f' || Cverificacion == false && opc == 2 && band3 == 'f')
  {
    mensaje_verificacion();
  }

  // VOID DE MENU DE OPCIONES QUE EL CLIENTE PUEDE REALIZAR
  opcion_menu_principal();

  // VERIFICA SI EL INGRESO ES DEL DUEÑO PARA HACER MODIFICACIONES EN ELLA
  verificacion_clave_maestra();

  // DESPUES DE VERIFICAR LA CONTRASEÑA PARA AGREGAR UNA CUENTA PASA A LA CREACION DE LA CUENTA
  if (opc == 2 && contador <= 9 && Cverificacion == true && subopc == 1 || contador == 0 && Cverificacion == true)
  {
    nuevaCuenta();
  }

  if (opc == 5)
  { // SI HAY MAS DE 10 USUARUIOS NO PERMITIRA EL INGRESO DE MAS USUARIOS
    lcd.clear();
    cr();
    Serial.print("USERS COMPLETED");
    lcd.print("USERS COMPLETED");
    estado = 1;
    delay(1000);
    lcd.clear();
    opc = 0;
    band3 ='v';
  }

  // SI LA CONTRASEÑA CORRECTA IMPRIME EN EL MONITOR SERIAL TODAS LAS CUENTAS ALMACENADAS
  if (CverificacionPrint == true){
    for (byte i = 0; i < contador; i++){
      Serial.print("Ciente ");
      Serial.println(i + 1);
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
  }

  // CUANDO SE HAYA MODIFICADO LA ESTRUCUTA SE ALMACENARA LOS CAMBIOS EN LA MEMORIA NVS DE LA ESP 32 REMPLZANDO ASI LOS CAMBIOS
  if (guardarEnPreferences){
    preferences.begin("my-app", false);
    preferences.putUInt("contador", contador);
    //BUCLE PARA ALMACENAR TODOS LOS DATOS DE LA ESTRUCTURA DENTRO DE LA NVS 
    for (byte i = 0; i < contador; i++){
      String key = "cliente" + String(i);
      preferences.putBytes(key.c_str(), &cliente[i], sizeof(cliente[i]));
    }
    preferences.end();
    guardarEnPreferences = false;
  }

 //MENU QUE PARA RECARGAR O AGREGAR CUENTA 
  if (opc == 2 && contador > 0 && Cverificacion == true && subopc == 0){
    char kye2 = keypad.getKey();
    //IMPRIME UNA SOLA  VEZ EN LA LCD 
    if (band7 == 'f'){
      lcd.clear();
      cr();
      lcd.print("RECARGAR -> *");
      cl(),
      lcd.print("NEW ACOUNT -> #");
      band7 = 'v';
    }

    if (kye2){
      switch (kye2){
      case '*':subopc = 2; lcd.clear(); band7 = 'f'; break;
      case '#':
        if (contador<=9){subopc = 1; lcd.clear(); band7 = 'f';}
        else{opc = 5; Cverificacion = false; indice = 0; band2 = 'v'; band3 = 'f'; band6 = 'f';band7 ='f';} 
        break;
      default: lcd.print("ERROR DE OPCION"); delay(500); lcd.clear(); band7 = 'f';break;
      }
    }
  }

  //PEDIRA EL ID DEL CLIENTE Y LO ALMACENARA DENTRO DE UNA VARIABLE PARA DESPUES COMPARARLA CON LOS DATOS DE LA ESTRUCTURA
  if (opc == 2 && subopc == 2){
    pedirID();
  }

  //SI SE INGRESA EL ID SE BUSCARA DENTRO DE TODA LA ESTRUCTURA
  if (user>=1000 && user < 9999){
    if (band7 == 'f'){
      lcd.clear();
      cr();
      lcd.print("ESPERE...");
      delay(500);
      band7 = 'v';
    }
    //BUCLE PARA COMPARAR CON TODOS LOS ID DE LOS CLIENTES 
    for(int i=0; i<contador; i++){
      if (user == cliente[i].id){
        numCliente = i;
        bandVC = 'v';
        if (opc == 1){
          estadoVC = true;
        }
        else if (opc == 2 ){
          estado1 = true;
        }
        break;
      }
      else{
        bandVC = 'f';
        estado1 = false;
      }
    }



    //VERIFICA SI LA CUENTA SE ENCONTRO Y REGISTRA EN DONDE SE ENCONTRO
    if (bandVC == 'v' && estado1 == true ){
     // lcd.clear();
      Serial.print("LA CUENTA SE ENCONTRO Y ES LA CUENTA NUMERO :");
      Serial.println(numCliente+1);
      user =0;
      cuentaExistente = true;
      lcd.clear();
      cr();
      lcd.print("DINERO A GUARDAR");
      cl();
      lcd.print("$ ");
      delay(1000);
      estado1 = true;
    }

    //VERIFICA SI LA CUENTA NO SE ENCONTRO E IMPRIME QUE LA CUENTA NO SE ENCONTRO
    else if (bandVC == 'f' && estado1 == false){
      lcd.clear();
      cr();
      lcd.print("ACOUNT INVALIDE");
      delay(500);
      Serial.println("LA CUENTA NO SE ENCONTRO EN LOS REGISTRO");
      delay(500);
      band_default();
      cuentaExistente = false;
    }
  }

//SI LA CUENTA EXISTE ENTRA Y PREGUNTA CUANTO DINERO QUIERE DEPOSITAR
  if (cuentaExistente == true && estado1 == true ){
    //Serial.println(numCliente);
    char kye2 = keypad.getKey();
    if (kye2){
      dineroSave[indiceD] = kye2;
      lcd.print(kye2);
      indiceD ++;
      if (kye2 =='#'){
        dineroint = atoi(dineroSave);
        cliente[numCliente].dinero = (cliente[numCliente].dinero + dineroint);
        cr();
        lcd.print("DEPOSITO EXITOSO");
        cl();
        lcd.print("DINERO: ");
        lcd.print(cliente[numCliente].dinero);
        delay(1500);
        Serial.print("NUEVO SALDO : ");
        Serial.println(cliente[numCliente].dinero);
        band_default();
      }
    }
  }


//RETIRO DE PRODUCTO DEPENDIENDO DE LA FORMA DE INGRESO DE ID Y CONTRASEÑA
  if (opc == 1 && estadoVC == false && retiroProducto == false){
    pedirID();
    band3 = 'f';
    if (mfrc522.PICC_IsNewCardPresent() && mfrc522.PICC_ReadCardSerial()){
      String idTarjeta = "";
      lcd.clear();
      cr();
      lcd.print("PROCESANDO RFID..");
      for (byte i = 0; i < mfrc522.uid.size; i++){
        idTarjeta += String(mfrc522.uid.uidByte[i], HEX);
      }
      idTarjeta.toUpperCase();
      strcpy(VT, idTarjeta.c_str());
      mfrc522.PICC_HaltA();
      mfrc522.PCD_StopCrypto1();
      bandx = 'v';
      for(int i=0; i<contador; i++){
        if (!strcmp(VT, cliente[i].Tarjeta)){
          numCliente = i;
          band9 = 'v';
          bandx = 'f';
          break;
        }
        else{
          band9 = 'f';
          bandx = 'g';
         }
    }
      }
      band5 = 'f';
    }

      
    if (band9 == 'f' && bandx == 'g' && opc == 1 && retiroProducto == false ){
      Serial.println("no encontrado");
      delay(500);
      band9 = 'g';
      band_default();
    }
    else if (band9 == 'v' && bandx == 'f' && opc == 1 && retiroProducto == false){
      Serial.print("encontrado, Cliente # ");
      Serial.println(numCliente +1);
      delay(500);
      retiroProducto = true;
    }

  if (opc == 1 && estadoVC == true && retiroProducto == false){
    
    if(band3 == 'f' ){
      Serial.print("si entro");
      Serial.print(numCliente +1 );
      lcd.clear();
      cr();
      lcd.print("DIGITE PASSWORD");
      cl();
      lcd.print("-> ");
      band3 = 'v';
      indice = 0 ;
    }
    char key = keypad.getKey();
    if (key){
      passVF[indice] = key;
      lcd.print(key);
      indice++;
    }
    if (indice == 4){
      if (!strcmp(passVF,cliente[numCliente].password)){
        lcd.clear();
        cr();
        lcd.print("EXITO");
        delay(500);
        band3 = 'f';
        retiroProducto = true;
        indice = 0;
      }
      else{
        lcd.clear();
        cr();
        lcd.print("ERROR");
        delay(500);
        band_default(); //EN CASO DE QUE LA CONTRASEÑA NO SEA CORRECTA VOLVERA AL MENU PRINCIPAL
      }
    }

  }

  if (opc == 1 && retiroProducto == true ){
    if (band3 == 'f'){
    lcd.clear();
    cr();
    lcd.print("CLIENTE -> ");
    lcd.print(numCliente + 1);
    band3 = 'v';
    }

    digitalWrite (15, HIGH);
    delay(1000);
    digitalWrite (15, LOW);
    delay (500);
  }


}





////////////////////////////////////////////////////////////// <<<<< VOID >>>>> ////////////////////////////////////////////////////////////

///////// INICIALIZACION DE LA LCD /////////
void lcdInit(){
  lcd.init();
  lcd.clear();
  lcd.backlight();
  lcd.setCursor(0, 0);
  lcd.print("READING...");
  delay(500);
}

///////// VOID  INICIA LA LECTURA DEL RFID //////////
void RFID(){
  SPI.begin();
  mfrc522.PCD_Init();
}

// VOID PARA LA POSICION DE LA PANTALLA LCD
// VOID PARA LA PRIMERA LINEA DE LA LCD
void cr(){
  lcd.setCursor(0, 0);
}

// VOID PARA SEGUNDA LINEA DE LA LCD
void cl(){
  lcd.setCursor(0, 1);
}

//// CREACION DE NUEVA CUENTA DE USUARIO DE MANERA SENCILLA SIN NFC O TARJETA////
void nuevaCuenta(){
  // PEDIRA EL ID DE LA CUENTA
  if (contador == 0 && band == 'f' || opc == 2 && band == 'f'){
    cr();
    lcd.print("INGRESE ID 4DIG  ");
    cl();
    lcd.print("ID: ");
    band = 'v';
    Serial.print("Cliente ");
    Serial.println(contador + 1);
  }

  // GUARDA EN LA ESTRUCTURA EL ID DE LA CUENTA
  if (contador == 0 && band == 'v' || opc == 2 && band == 'v'){
    char key = keypad.getKey();
    // BUCLE HASTA QUE EL ID ESTE EN EL RANGO DE 1000 - 9999
    do{
      if (key){
        dato[indice] = key;
        lcd.print(key);
        indice++;
        cliente[contador].id = atoi(dato);
        dato[indice] = '\0';
      }

      // CUANDO SE INGRESE EL 4 DIGITO SE GURADARA AUNTOMATICAMENTE EN EL ID
      if (indice == 4 && cliente[contador].id >= 1000){
        indice = 0;
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
    } while (cliente[contador].id >= 1000 && band == 'f');
  }
  
  // PEDIRA LA CONTRASEÑA DEL CLIENTE CON UN TOTAL DE 4 CARACTERES
  if (contador == 0 && band == 'p' || opc == 2 && band == 'p'){
    cr();
    lcd.print("PASSWORD      ");
    delay(500);
    cr();
    lcd.print("INGRESE PASSWORD ");
    cl();
    lcd.print("PASS: ");
    band = 'a';
    indice = 0;
  }

  // GUARDARA LA CONTRASEÑA DENTRO DE PASSWORD DEL CLIENTE
  if (contador == 0 && band == 'a' || opc == 2 && band == 'a'){
    char key = keypad.getKey();
    if (key){
      dato[indice] = key;
      lcd.print(key);
      indice++;
    }

    // UNA VES INGRESADO LOS CUATRO DIGITOS IMPRIME EN EL MONITOR SERIAL LOS DATOS DEL CLIENTE
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
    // char bandaa = 'f';
    if (banda == 'f'){
      cr();
      lcd.print("TARJETA RFID?");
      cl();
      lcd.print("SI-> *  NO-> #");
      band3 = 'v';
    }
  }

  // VERIFICA QUE SI PRESIONO * PARA LECTURA DEL LA RFID O # PARA CANCELAR LA LECTURA DEL RFID
  if (contador == 0 && band == 't' && band3 == 'v' || opc == 2 && band == 't' && band3 == 'v'){
    char key = keypad.getKey();
    if (key){
      switch (key){
      case '*': band = 'q'; lcd.clear(); break;
      case '#': Serial.println("TARJETA NO IDENTIFICADA"); band = 'n'; strcpy(cliente[contador].Tarjeta, "NO REGISTRADO");
        lcd.clear();break;
      default: lcd.clear(); cr(); lcd.print("ERROR"); delay(500); lcd.clear(); break;
      }
    }
    // SI PRESIONO  * PEDIRA QUE ESCANEE LA TARJETA
    if (contador == 0 && band == 'q' && band3 == 'v' || opc == 2 && band == 'q' && band3 == 'v'){
      tarjeta();
    }
  }

  // VERIFICACION SI TIENE NFC
  if (contador == 0 && band == 'n' || opc == 2 && band == 'n'){
    char key = keypad.getKey();
    char banda = 'f';
    char bandaa = 'f';
    if (banda == 'f'){
      cr();
      lcd.print("NFC?");
      cl();
      lcd.print("SI-> *  NO-> #");
      band3 = 'v';
    }
  }

  // SI PRESIONA * PEDIRA QUE ESCANEE EL NFC
  if (contador == 0 && band == 'n' && band3 == 'v' || opc == 2 && band == 'n' && band3 == 'v'){
    char key = keypad.getKey();
    if (key){
      switch (key){
      case '*': band = 'e'; break;
      case '#': Serial.println("NFC NO IDENTIFICADO"); cliente[contador].primerDigito = 'N'; band = 'z'; fin = true; break;
      default: lcd.clear(); cr(); lcd.print("ERROR"); delay(500); lcd.clear(); break;
      }
    }
  }

  if (contador == 0 && band == 'e' && band3 == 'v' || opc == 2 && band == 'e' && band3 == 'v'){
    nfcEscaner();
  }

  // UNA VEZ TERMINADO TODO EL PORCESO DE CREACION DE CUENTA AGREGA EL CLIENTE
  if (band == 'z' && fin == true){
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
    contador ++;
    band_default();
  }
} // FIN DEL INGRESO DE CUENTA



//////// VERIFICACION DE LA CLAVE PARA PODER RECARGAR O INGRESAR NUEVA CUENTA
void verificacion_clave_maestra(){
  if (Cverificacion == false && opc == 2 && band3 == 'v'){
    char key = keypad.getKey();
    if (band2 == 'v'){
      cr();
      lcd.print("PASSWORD MASTER");
      cl();
      lcd.print("KEY: ");
      band2 = 'f';
      indice = 0;
    }

    //  MUESTRA EN LA PANTALLA LA CONTRASEÑA PERO DE MANERA DE XXXXXXX
    if (key){
      clavei[indice] = key;
      lcd.print("X");
      indice++;
    }

    // CUANDO LA CONTRASEÑA TENGA UN RANGO DE 6 DIGITOS LA COMPARA CON LA CONTRASEÑA MAESTRA
    if (indice == 6 && Cverificacion == false){
      lcd.clear();
      // COMPARA PARA LA GENERACION DE UNA NUEVA CUENTA
      if (!strcmp(clavei, clave)){
        Cverificacion = true;
        indice = 0;
      }

      // COMPARA PARA PODER IMPRIMIR LOS DATOS DE LAS CUENTAS DE LOS CLIENTES
      else if (!strcmp(clavei, clavePrint)){
        CverificacionPrint = true;
        indice = 0;
        opc = 0;
        band2 = 'v';
        band3 = 'f';
      }

      // SI LA CONTRASEÑA NO COINCIDE ENTONCES DICE QUE LA CLAVE ES INCORRECTA Y VUELVE A SU MENU PRINCIPAL
      else{
        lcd.clear();
        cr();
        lcd.print("CLAVE INCORRECTA");
        delay(500);
        opc = 0;
        lcd.clear();
        indice = 0;
        band2 = 'v';
        band3 = 'f';
      }
    }

    // En caso que en ese tiempo no se ingrese ningun caracter
    if (contador > 0){
      retardo();
      if (segundos == tiempo_anterior){
        lcd.clear();
        cr();
        lcd.print("TIEMPO AGOTADO");
        delay(500);
        opc = 0;
        lcd.clear();
        indice = 0;
        band2 = 'v';
        band3 = 'f';
        band6 = 'f';
      }
    }
  }
}

// MENU PRINCIPAL DE LAS ACCIONES QUE SE PUEDEN HACER
void opcion_menu_principal(){
  if (opc == 0 && contador > 0){
    char key = keypad.getKey();
    cr();
    lcd.print("   PRESIONE");
    cl();
    lcd.print("KEY-> * AVAN-> #");
    estado = 0;
    // VERIFICACION DE LA ACCION QUE QUIERE HACER EL CLIENTE
    if (key){
      switch (key){
      // OPCIONES QUE REQUIEREN QUE INGRESE CONTRASEÑA PARA INGRESAR UNA CUENTA
      case '*': opc = 2; Cverificacion = false; band3 = 'f'; band6 = 'f'; lcd.clear(); break;
      // EN CASO DE QUE QUIERA PROCEDER A RETIRAR UN  PRODUCTO
      case '#': opc = 1; subopc = 1; lcd.clear(); band6 = 'f'; break;
        // OPCION  TEMPORAL PARA BOORAR LOS DATOS DE LA EPROM
      case '1': lcd.clear(); cr(); lcd.print("CLEAR DATOS"); delay(500); lcd.clear(); preferences.begin("my-app", false); preferences.clear(); preferences.end(); break;
      // EN CASO DE QUE PRESIONE UN TECLA QUE NO ESTE EN EL MENU
      default: lcd.clear(); cr(); lcd.print("ERROR DE OPCION"); delay(500); lcd.clear(); break;
      }
    }
  }
}

// MENSAJE DE VERIFICACION
void mensaje_verificacion(){
  opc = 2;
  Cverificacion = false;
  indice = 0;
  lcd.clear();
  cr();
  lcd.print("VERIFICACION");
  delay(300);
  band3 = 'v';
}

// LECTURA DE TARJETA VOID
void tarjeta(){
  while (band == 'q'){
    if (band5 == 'f'){
      lcd.clear();
      lcd.setCursor(0, 0);
      lcd.print("ESCANEE LA TARJETA");
      lcd.setCursor(0, 1);
      lcd.print("WAINTING...");
      band5 = 'g';
    }

    // SI SE PRESENTA UNA TARJETA EN EL LECTOR LA LEE Y ALMACENA LA ID DENTRO DE TARJEA DE CLIENTE
    if (mfrc522.PICC_IsNewCardPresent() && mfrc522.PICC_ReadCardSerial()){
      String idTarjeta = "";
      lcd.clear();
      for (byte i = 0; i < mfrc522.uid.size; i++){
        idTarjeta += String(mfrc522.uid.uidByte[i], HEX);
      }
      idTarjeta.toUpperCase();
      strcpy(cliente[contador].Tarjeta, idTarjeta.c_str()); // COPIA A LA ESTREUCTURA DEL CLIENTE
      mfrc522.PICC_HaltA();
      mfrc522.PCD_StopCrypto1();
      // TERMINA LA LECTURA DEL RFID
      band = 'n'; // PASA ALA VERFICACION DEL NFC
      lcd.clear();
      cr();
      lcd.print("TARJETA SAVE");
      delay(500);
      lcd.clear();
      band5 = 'f';
    }
  }
}

// VOID DEL ESCANEO NFC ///
void nfcEscaner(){
  while (band == 'e'){
    if (band5 == 'f'){
      lcd.clear();
      lcd.setCursor(0, 0);
      lcd.print("ESCANEE NFC");
      lcd.setCursor(0, 1);
      lcd.print("WAINTING...");
      band5 = 'g';
    }

    // SI SE PRESENTA UN NFC EN EL LECTOR LO LEE Y LO ALMACENA DENTRO DE NFC DEL CLIENTE
    if (mfrc522.PICC_IsNewCardPresent() && mfrc522.PICC_ReadCardSerial())
    {
      String idTarjeta = "";
      lcd.clear();
      for (byte i = 0; i < mfrc522.uid.size; i++)
      {
        idTarjeta += String(mfrc522.uid.uidByte[i], HEX);
      }
      idTarjeta.toUpperCase();
      cliente[contador].primerDigito = idTarjeta[0];
      mfrc522.PICC_HaltA();
      mfrc522.PCD_StopCrypto1();
      // DETIENE LA LECTURA DEL NFC
      lcd.clear();
      lcd.print("NFC SAVE");
      delay(500);
      lcd.clear();
      band5 = 'f';
      band = 'z';
      fin = true;
    }
  }
}

//VOID PARA RETARDO
void retardo(){
  if (band6 == 'f'){
    tiempo_anterior = segundos + 10;
    band6 = 'v';
  }
}

void verificarCliente(){
}


void pedirID (){
  char key = keypad.getKey();
  if (band7 == 'f'){
      cr();
      lcd.print("INGRESE SU USERS");
      cl();
      lcd.print("->  ");
      band7 = 'v';
      indice = 0;
    }
    
  if (key){
    idVF[indice] = key;
    lcd.print(key);
    indice++;
    if (indice == 4){
    user = atoi(idVF);
    }
    //dato[indice] = '\0';
  }

    if (indice == 4 && user >= 1000){
      indice = 0;
      lcd.clear();
      band7 = 'f';
      subopc = 5;
  }

    // EL ID DEBE DE ESTAR EN EL RANGO DE 1000 A 9999
    else if (indice == 4 && user < 1000){
      indice = 0;
      cr();
      lcd.print("ERROR ID < 1000");
      delay(500);
      band7 = 'f';
      lcd.clear();
      subopc = 2; 
    }
}

//MANTIENE POR DEFECTO TODAS LAS VARIABLES DE TIPO BAND Y OTRAS PARA EL INICIO AL MENU PRINCIPAL
void band_default(){
  band9 = 'g'; bandx = 'g'; Cverificacion = false;
  opc = 0; user =0; bandVC = 'g'; band2 = 'v'; band3 = 'f'; band4 = 'f'; band5 = 'f'; band6 = 'f'; band7 = 'f'; band8 = 'f'; bandVC = 'g';
  indice = 0; band = 'f'; lcd.clear(); band2 = 'v'; fin = false; band3 = 'f'; opc = 0; guardarEnPreferences = true; subopc = 0; band7 = 'f';
  dineroint = 0; cuentaExistente = false; estado1 = false; indiceD = 0; estado1 = false;  estadoVC = false;
  cuentaExistente = false; retiroProducto = false;  
 
}