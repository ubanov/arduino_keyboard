/*
Emular un teclado con el arduino. Enviar teclas indicadas
Se ha configurado un "antirebotes" por software
 */

#include <Keyboard.h>
//#include <avr/wdt.h>
//#include <LowPower.h>

#define MAXBOTONES 32	// para mame deberian ser 32

#define FIRSTBOTON 22	      // para mame deberian ser 22

#define ANTIREBOTETIME 1      // tiempo en milisegundos para el antirebote



// Pin 13 tine un LED conectado. Le ponemos un nombre
int led = LED_BUILTIN;

// Pin 8 led exterior
int ledexterno=8;

// para gestion del encendido y apagado del led
unsigned long millisled=0;
unsigned long millisledexterno=0;


// arrays para la gestion de los botones
char botones[MAXBOTONES];       // leer estado de botones sobre este arary
char estadobotones[MAXBOTONES]; // para automata de enviar teclas (inicializar a 0 este array)
unsigned long timersbotones[MAXBOTONES];

//char teclas[MAXBOTONES]={'a','s','d','f','g'};
// las teclas http://arduino.cc/en/Reference/KeyboardModifiers
//char teclas[MAXBOTONES]={KEY_UP_ARROW,KEY_DOWN_ARROW,KEY_LEFT_ARROW,KEY_RIGHT_ARROW,' '};

#if 0
// las teclas para lo de dani: “+” , “-“ , “barra espaciadora” , “intro” , “R” , “S” , “C” y “B” 
int teclas[MAXBOTONES]={        // las teclas que vamos a enviar cuando se pulse cada uno de los botones
  ']',    // en teclado ingles el + esta aqui
  '/',    // en teclado ingles el - esta aqui
  'r','s','c','b',' ',KEY_RETURN};

#else
int teclas[33]={    // las teclas que vamos a enviar cuando se pulse cada uno de los botones

  // player 1
  KEY_UP_ARROW,     // 22: arriba (C1 naranja)
  KEY_LEFT_CTRL,    // 23: boton 1 (C2 verde)
  KEY_LEFT_ARROW,   // 24: izquierda (C1 verde)
  KEY_LEFT_ALT,     // 25: boton 2 (C2 naranja)
  KEY_DOWN_ARROW,   // 26: abajo (C1 azul)
  ' ',              // 27: boton 3 (C2 azul)
  KEY_RIGHT_ARROW,  // 28: derecha (C1 marron)
  KEY_LEFT_SHIFT,   // 29: boton 4 (C2 marron)

  // player 2
  'r',              // 30: arriba (C1 naranja)
  'a',              // 31: boton 1 (C2 verde)
  'd',              // 32: izquierda (C1 verde)
  's',              // 33: boton 2 (C2 naranja)
  'f',              // 34: abajo (C1 azul)
  'q',              // 35: boton 3 (C2 azul)
  'g',              // 36: derecha (C1 marron)
  'w',              // 37: boton 4 (C2 marron)

  // varios: botones de control y botones 5 y 6 de los jugadores
  KEY_TAB,          // 38: comando de control 1 - TAB (C1 verde)
  'z',              // 39: boton 5 player 1 (C2 naranja)
  KEY_ESC,          // 40: comando de control 2 - ESC (C1 azul)
  'x',              // 41: boton 6 player 1 (C2 verde)
  'p',              // 42: comando de control 2 - Intro (C1 marron)
  'c',              // 43: boton 5 player 2 (C2 azul) - no estandar
  ' ',              // 44: vacio (C1 no conectado)
  'v',              // 45: boton 6 player 2 (C2 marron) - no estandar
    
  // otros: coin, start y flipper
  'b',              // 46: tecla de flipper izquierdo (C1 verde) - no estandar
  'n',              // 47: tecla de flipper derecho (C2 verde) - no estandar
  '1',					    // 48: start player 1 (C1 azul)
  '2',              // 49: start player 2 (C2 azul)
  '5',					    // 50: coin player 1 (C1 marron)
  '6',					    // 51: coin player 2 (C2 marron)
  '-',				      // 52: tecla especial 1 no conectada (bajar volumen) podria ser el ‘-‘ (teclado ingles)
  '=',				      // 53: tecla especial 2 no conectada (subir volumen) podria ser el ‘=‘ (teclado ingles)

  // fin de lista
  0				          // fin de lista
};
#endif


// the setup routine runs once when you press reset:
void setup() {         
  int a;
  
  // initialize the digital pin as an output.
  pinMode(led, OUTPUT);
  //pinMode(ledexterno, OUTPUT);
  
  // inicializar en entrada el pin de programacion
  pinMode(2, INPUT_PULLUP);
  // comprobar si esta en modo de programaci´n o en modo normal
  if(!digitalRead(2) && 0) {  // si esta conectado a masa... (esto pone en manuales de arduino keyboard, pero no creo que es necesario)
    
    while(1) {
      digitalWrite(led, HIGH);   // turn the LED on (HIGH is the voltage level)
      delay(300);               // wait for a second
      digitalWrite(led, LOW);    // turn the LED off by making the voltage LOW
      delay(300);               // wait for a second
    }
  }

  // encender led
  digitalWrite(led, HIGH);   // turn the LED on (HIGH is the voltage level)
  millisled=millis();
  
  // entradas de botones, inicializarlas
  for(a=FIRSTBOTON;a<FIRSTBOTON+MAXBOTONES;a++)
    pinMode(a,INPUT_PULLUP);
  
  // inicializar variable estadobotones
  for(a=0;a<MAXBOTONES;a++)
    estadobotones[a]=0;
    
  // inicializar teclado
  delay(2000);      // esperamos 2 segundos para que todo este bien
  Keyboard.begin();
  Keyboard.press(KEY_RIGHT_SHIFT);   // y pulsar el shift
  delay(500);
  Keyboard.release(KEY_RIGHT_SHIFT);
  // enable watchdog
  //wdt_enable(WDTO_250MS);
}

// botones sin multiplexor
int LeerBotones(void)
{
  int a;

  for(a=0;a<MAXBOTONES;a++)
    botones[a]=!digitalRead(FIRSTBOTON+a);    // pines de FIRSTBOTON a FIRSTBOTON+MAXBOTONES-1
  return a;   // devolver numero de botones leidos
}    


// esta rutina es la encargada de enviar las pulsaciones. Por ahora 
// solo envia pulsacion cuando se hace y fin de pulsacion...
// tiene un automata de 4 estados para evitar rebote por software.
void TrataTeclaConAntirebote(int n)
{
  if(estadobotones[n]==1) {   // esperando a que se consolide pulsacion tecla    
    if(!botones[n])       // si era rebote volver hacia atras a estado 0
      estadobotones[n]=0;
    else if(millis()-timersbotones[n]>=ANTIREBOTETIME) {
      estadobotones[n]=2;
      // enviar tecla (teclas especiales que necesitaban tratamiento especial para dani)
//      if(teclas[n]=='+') {
        //Keyboard.press(KEY_LEFT_SHIFT);
        //Keyboard.press('=');
//        Keyboard.write(teclas [n]);
//      } else if(teclas[n]=='-') {
//        Keyboard.write(teclas[n]);
//      } else
      Keyboard.press(teclas[n]);	// envio tecla normal
    }
  } if(estadobotones[n]==2) {   // se ha enviado tecla, esperando a que se deje de pulsar
      if(!botones[n]) {
        timersbotones[n]=millis();
        estadobotones[n]=3;
      }
  } else if(estadobotones[n]==3) {  // probando si se ha consolidado la no pulsacion
      if(botones[n])
        estadobotones[n]=2;
      else if(millis()-timersbotones[n]>=ANTIREBOTETIME) {
        estadobotones[n]=0;
      
//      if(teclas[n]=='+') {	// teclas especiales
        //Keyboard.release('=');
        //Keyboard.release(KEY_LEFT_SHIFT);
//        ;
//      } else if(teclas[n]=='-') {
//        ;
//      } else
      Keyboard.release(teclas[n]);
    }
  } else {                // estado por defecto, esperando pulsacion
      if(botones[n]) {
        estadobotones[n]=1;
        timersbotones[n]=millis();
      }
  }
}


void TrataTeclaSinAntirebote(int n)
{
  if(estadobotones[n]==0) {      
    if(botones[n]) {
      Keyboard.press(teclas[n]);
      estadobotones[n]=1;
    }
  } else {
    if(!botones[n]) {
      Keyboard.release(teclas[n]);
      estadobotones[n]=0;
    }
  }
}


// gestion de diodo led. Para indicar que esta funcionando bien
// cada dos segundos hacemos un encendido rapido del led
// en setup tendremos que ejecutar millisled=millis(); para
// inicializar rutina
int GestionLed(unsigned long encendido,unsigned long cycle)
{
  if(millisled==0)      // por si acaso en setup no se ha inicializado
    millisled=millis();
    
  while(millis()-millisled>cycle)
    millisled=millisled+cycle;

  if(millis()-millisled<encendido){
    digitalWrite(led, HIGH);   // turn the LED on (HIGH is the voltage level)
    return 1;
  } else {
    digitalWrite(led, LOW);   // turn the LED on (HIGH is the voltage level)
    return 0;
  }
}


// gestion de diodo led. Para indicar que esta funcionando bien
// cada dos segundos hacemos un encendido rapido del led
// en setup tendremos que ejecutar millisled=millis(); para
// inicializar rutina
int GestionLedExterno(unsigned long encendido,unsigned long cycle,unsigned long trans)
{
  int value;
  unsigned long t;
  
  if(millisledexterno==0)      // por si acaso en setup no se ha inicializado
    millisledexterno=millis();
    
  while(millis()-millisled>cycle)
    millisledexterno=millisledexterno+cycle;

  t=millis()-millisledexterno;
  if(t<trans)
    value=255*(float)t/trans;
  else if(t<trans+encendido)
    value=255;
  else if(t<trans*2+encendido)
    value=255*(float)(trans*2+encendido-t)/trans;
  else
    value=0;

  if(value<0)
    value=0;
  else if(value>255)
    value=255;
    
  analogWrite(ledexterno, 255-value);
  return 0;
}

// the loop routine runs over and over again forever:
void loop() {
  int a;

  //wdt_reset();    // resetear watchdog
  GestionLed(300,2000);
  //GestionLedExterno(300,2000,200);
  LeerBotones();      // leer todos los botones en array teclas[]
  for(a=0;a<MAXBOTONES;a++)    // tratar cada uno de los botones
    if(ANTIREBOTETIME==0)
      TrataTeclaSinAntirebote(a);
    else
      TrataTeclaConAntirebote(a);
}


