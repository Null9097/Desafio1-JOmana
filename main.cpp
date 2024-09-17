//#include <Wire.h>
#include <Adafruit_LiquidCrystal.h>

Adafruit_LiquidCrystal lcd(0);
int analogPin = A0;
int16_t* arr = nullptr;
int capacidad = 512;
int indiceActual = 0;
bool acquiringData = false;

int botonInicio = 7;
int botonDetener = 4;

int16_t maxValue = INT16_MIN;
int16_t minValue = INT16_MAX;
unsigned long lastPeakTime = 0;
unsigned long lastPeakTimePrev = 0;
int peakCount = 0;
float frecuencia = 0;
float amplitud = 0;

void setup() {
    Serial.begin(9600);
    pinMode(botonInicio, INPUT_PULLUP);
    pinMode(botonDetener, INPUT_PULLUP);
    lcd.begin(16, 2);
    arr = new int16_t[capacidad];
}

void loop() {
    if (digitalRead(botonInicio) == HIGH) {
        iniciarAdquisicion();
    }

    if (digitalRead(botonDetener) == HIGH) {
        detenerAdquisicion();
    }

    if (acquiringData) {
        adquirirDatos();
        printMemoryUsage();
    }
}


void iniciarAdquisicion() {
    delete[] arr;
    arr = new int16_t[capacidad];
    indiceActual = 0;
    maxValue = INT16_MIN; 
    minValue = INT16_MAX; 
    peakCount = 0; 
    acquiringData = true;

    lcd.clear();
    lcd.print("Adq. datos");
}

void detenerAdquisicion() {
    acquiringData = false;
    
    calcularAmplitudYFrecuencia();
	  identificarFormaDeOnda();
  
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("Amp: ");
    lcd.print(amplitud, 2);
    lcd.print(" V");

    lcd.setCursor(0, 1);
    lcd.print("Freq: ");
    lcd.print(frecuencia, 2);
    lcd.print(" Hz");
}

void adquirirDatos() {
    int16_t valor = analogRead(analogPin);
    
    arr[indiceActual] = valor;
    Serial.println(arr[indiceActual]);

    if (valor > maxValue) {
        maxValue = valor;
    }
    
    if (valor < minValue) {
        minValue = valor;
    }

    static int16_t prevValue = 0;  
    if (valor > prevValue && (valor - minValue) > 5) {  
        lastPeakTimePrev = lastPeakTime;
        lastPeakTime = millis(); 
        peakCount++;

        if (peakCount > 1) {
            unsigned long intervalo = lastPeakTime - lastPeakTimePrev;
            if (intervalo > 0) {
                float freqActual = 1000.0 / intervalo; 
                frecuencia += freqActual;
            }
        }
    }
    
    prevValue = valor;  

    indiceActual++;
    
    if (indiceActual >= capacidad) {
        indiceActual = 0;
    }
}

void calcularAmplitudYFrecuencia() {
   amplitud = (maxValue - minValue) * (5.0 / 1023.0);
   
   Serial.print("Amplitud: ");
   Serial.print(amplitud);
   Serial.println(" V");

   if (peakCount > 1) {
       frecuencia /= (peakCount - 1); // Dividir por el número de intervalos entre picos
       Serial.print("Frecuencia: ");
       Serial.print(frecuencia, 4); // Mostrar con mayor precisión
       Serial.println(" Hz");
   } else {
       Serial.println("No se detectó una frecuencia válida");
   }

   maxValue = INT16_MIN; 
   minValue = INT16_MAX; 
   frecuencia = 0; 
   peakCount = 0;
}

void printMemoryUsage() {
   Serial.print("Memoria libre: ");
   Serial.print(freeMemory());
   Serial.println(" bytes");
}

int freeMemory() {
   extern int __heap_start;
   extern int *__brkval;
   int v;
   int* p=(int*)&v;
   return (int)p-(int)__brkval;
}

void identificarFormaDeOnda() {
  bool esCuadrada = identificarOndaCuadrada();
  bool esTriangular = identificarOndaTriangular();

  lcd.clear();
  if (esCuadrada) {
    lcd.print("Onda Cuadrada");
  } else if (esTriangular) {
    lcd.print("Onda Triangular");
  } else {
    lcd.print("Señal Desconocida");
  }

  Serial.print("Es cuadrada: ");
  Serial.println(esCuadrada ? "Si" : "No");
  Serial.print("Es triangular: ");
  Serial.println(esTriangular ? "Si" : "No");
}

bool identificarOndaCuadrada() {
  int16_t nivel1 = arr[0];
  int16_t nivel2 = 0;
  bool cambioDetectado = false;
  
  for (int i = 1; i < indiceActual; i++) {
    if (!cambioDetectado && abs(arr[i] - nivel1) > 100) {
      nivel2 = arr[i];
      cambioDetectado = true;
    }
    else if (cambioDetectado && abs(arr[i] - nivel1) > 100 && abs(arr[i] - nivel2) > 100) {
      return false;
    }
  }

  return true;
}

bool identificarOndaTriangular() {
  bool subiendo = arr[1] > arr[0];
  
  for (int i = 1; i < indiceActual - 1; i++) {
    int16_t pendienteActual = arr[i+1] - arr[i];
    int16_t pendienteAnterior = arr[i] - arr[i-1];

    if (subiendo) {
      if (pendienteActual < 0) {
        subiendo = false;
      }
    } else {
      if (pendienteActual > 0) {
        subiendo = true;
      }
    }

    if ((subiendo && pendienteActual <= 0) || (!subiendo && pendienteActual >= 0)) {
      return false;
    }
  }

  return true;
}

void cleanup() { 
     delete[] arr; 
}