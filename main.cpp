#include <Wire.h>
#include <Adafruit_LiquidCrystal.h>

Adafruit_LiquidCrystal lcd(0);
int analogPin = A0;
int* arr = nullptr;
int capacidad = 2;
int cantElementos = 0;
bool acquiringData = false;


int botonInicio = 7;
int botonDetener = 4;

void setup() {
  Serial.begin(9600);
  pinMode(botonInicio, INPUT_PULLUP);
  pinMode(botonDetener, INPUT_PULLUP);
  lcd.begin(16, 2);
  arr = new int[capacidad];
}

void loop() {
  if (digitalRead(botonInicio) == HIGH) {
    acquiringData = true;
    lcd.clear();
    lcd.print("Adq. datos");
  }

  if (digitalRead(botonDetener) == HIGH) {
    acquiringData = false;
    lcd.clear();
    lcd.print("Adq. detenida");
    printArray();
  }

  if (acquiringData) {
    if (cantElementos == capacidad) {
      redArr(arr, capacidad);
    }
    arr[cantElementos] = analogRead(analogPin);
    Serial.println(arr[cantElementos]);
    cantElementos++;
    delay(20);
    //lcd.setCursor(0, 1);
    //lcd.print("Valor: ");
    //lcd.print(arr[cantElementos - 1]);
  }
}


void redArr(int*& arr, int& capacidad) {
    int nuevaCap = capacidad * 2;
    int* nuevoArr = new int[nuevaCap];
    for (int i = 0; i < capacidad; i++) {
        nuevoArr[i] = arr[i];
    }
    delete[] arr;
    arr = nuevoArr;
    capacidad = nuevaCap;
}
void printArray() {
  Serial.println("Contenido del arreglo:");
  for (int i = 0; i < cantElementos; i++) {
    Serial.print("Indice ");
    Serial.print(i);
    Serial.print(": ");
    Serial.println(arr[i]);
  }
}

void cleanup() {
  delete[] arr;
}
