#include <Wire.h>
#include <Adafruit_LiquidCrystal.h>

Adafruit_LiquidCrystal lcd(0);
int analogPin = A0;
int16_t* arr = nullptr;
int capacidad = 2;
int cantElementos = 0;
bool acquiringData = false;

int botonInicio = 7;
int botonDetener = 4;

int16_t maxValue = INT16_MIN;
int16_t minValue = INT16_MAX;
unsigned long peakTime1 = 0;
unsigned long peakTime2 = 0;
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
    if (cantElementos == capacidad) {
      redArr(arr, capacidad);
    }

    int16_t valor = analogRead(analogPin);
    arr[cantElementos] = valor;
    Serial.println(arr[cantElementos]);

    if (valor > maxValue) {
      maxValue = valor;
      peakTime1 = millis();
    }

    if (valor < minValue) {
      minValue = valor;
    }

    // Calcular la frecuencia
    if (peakTime1 != 0 && (millis() - peakTime1) > 1000) { // Ajustar el tiempo según la señal
      peakTime2 = millis();
      unsigned long periodo = peakTime2 - peakTime1;
      frecuencia = 1000.0 / periodo; // Frecuencia en Hz
      peakTime1 = peakTime2;
    }

    cantElementos++;
    delay(50);
    printMemoryUsage();
  }
}

void iniciarAdquisicion() {
  delete[] arr;

  capacidad = 2;
  cantElementos = 0;
  arr = new int16_t[capacidad];
  maxValue = INT16_MIN;
  minValue = INT16_MAX;
  acquiringData = true;
  lcd.clear();
  lcd.print("Adq. datos");
}

void detenerAdquisicion() {
  acquiringData = false;
  lcd.clear();
  lcd.print("Adq. detenida");
  printArray();
  calcularAmplitudYFrecuencia();
}

void calcularAmplitudYFrecuencia() {
  amplitud = (maxValue - minValue) * (5.0 / 1023.0);

  Serial.print("Amplitud: ");
  Serial.print(amplitud);
  Serial.println(" V");

  if (frecuencia > 0) {
    Serial.print("Frecuencia: ");
    Serial.print(frecuencia);
    Serial.println(" Hz");
  } else {
    Serial.println("No se detectó una frecuencia válida");
  }

  maxValue = INT16_MIN;
  minValue = INT16_MAX;
  frecuencia = 0;
}

void redArr(int16_t*& arr, int& capacidad) {
  int nuevaCap = capacidad * 2;
  int16_t* nuevoArr = new int16_t[nuevaCap];
  for (int i = 0; i < capacidad; i++) {
    nuevoArr[i] = arr[i];
  }
  delete[] arr;
  arr = nuevoArr;
  capacidad = nuevaCap;
}

void printArray() {
  for (int i = 0; i < cantElementos; i++) {
    Serial.print("Indice ");
    Serial.print(i);
    Serial.print(": ");
    Serial.println(arr[i]);
  }
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
  int* p = (int*)&v;
  return (int)p - (int)__brkval;
}

void cleanup() {
  delete[] arr;
}
