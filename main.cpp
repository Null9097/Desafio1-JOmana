#include <Wire.h>
#include <Adafruit_LiquidCrystal.h>

Adafruit_LiquidCrystal lcd(0);
int analogPin = A0;
int16_t* arr = nullptr;  // Arreglo dinámico para valores analógicos
int capacidad = 512;     // Capacidad inicial del arreglo
int indiceActual = 0;    // Índice del arreglo
bool acquiringData = false; // Estado de adquisición de datos

int botonInicio = 7;
int botonDetener = 4;

int16_t maxValue = INT16_MIN;
int16_t minValue = INT16_MAX;
float frecuencia = 0;
float amplitud = 0;
int crucePorCero = 0; // Contador para cruces por el punto medio
unsigned long tiempoInicio = 0; // Tiempo de inicio de la adquisición
unsigned long tiempoTotal = 0;  // Tiempo total de la adquisición

String tipoOnda = "Desconocida";

void setup() {
    Serial.begin(9600);
    pinMode(botonInicio, INPUT_PULLUP);
    pinMode(botonDetener, INPUT_PULLUP);
    lcd.begin(16, 2);
    arr = new int16_t[capacidad]; // Inicialización del arreglo dinámico
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
    }
}

void iniciarAdquisicion() {
    delete[] arr;
    arr = new int16_t[capacidad]; // Reasignar memoria
    indiceActual = 0;
    maxValue = INT16_MIN;
    minValue = INT16_MAX;
    crucePorCero = 0; // Reiniciar contador
    acquiringData = true;
    tiempoInicio = millis(); // Guardar el tiempo de inicio

    lcd.clear();
    lcd.print("Adq. datos");
}

void detenerAdquisicion() {
    acquiringData = false;
    tiempoTotal = millis() - tiempoInicio; // Calcular tiempo total de adquisición
    
    calcularAmplitud();
    calcularFrecuencia();
    identificarTipoOnda();

    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("Amplitud ");
    lcd.setCursor(0, 1);
    lcd.print(amplitud, 2);
    lcd.print(" V");
    delay(2000);

    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("Frecuencia ");
    lcd.setCursor(0, 1);
    lcd.print(frecuencia, 2);
    lcd.print(" Hz");
    delay(2000);
    
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("Onda: ");
    lcd.setCursor(0, 1);
    lcd.print(tipoOnda);

    Serial.println("Datos recopilados:");
    Serial.println();
    Serial.print("Amplitud: ");
    Serial.println(amplitud, 2);
    Serial.print("Frecuencia: ");
    Serial.println(frecuencia, 2);
    Serial.print("Tipo de onda: ");
    Serial.println(tipoOnda);
}

void adquirirDatos() {
    int16_t valor = analogRead(analogPin);
    arr[indiceActual] = valor;
    Serial.println(valor);

    if (valor > maxValue) maxValue = valor;
    if (valor < minValue) minValue = valor;

    // Contar cruces por el punto medio
    if (indiceActual > 0) {
        int16_t medio = (maxValue + minValue) / 2;
        if ((arr[indiceActual - 1] > medio && valor < medio) || (arr[indiceActual - 1] < medio && valor > medio)) {
            crucePorCero++;
        }
      delay(2);
    }

    indiceActual++;
    if (indiceActual >= capacidad) indiceActual = 0; // Sobrescribir valores antiguos

    delay(2); // 2 ms para una frecuencia de muestreo de 500 Hz
}

void calcularAmplitud() {
    amplitud = (maxValue - minValue) * (5.0 / 1023.0);
}

void calcularFrecuencia() {
    if (crucePorCero > 1 && tiempoTotal > 0) {
        float tiempoSegundos = tiempoTotal / 1000.0;
        frecuencia = (crucePorCero / 2.0) / tiempoSegundos;
    } else {
        frecuencia = 0;
    }
}

void identificarTipoOnda() {
    if (indiceActual < 20) {
        lcd.clear();
        lcd.print("Datos insuficientes");
        return;
    }

    tipoOnda = "Desconocida";

    if (identificarOndaCuadrada()) {
        tipoOnda = "Cuadrada";
    } else if (identificarOndaSenoidal()) {
        tipoOnda = "Senoidal";
    } else if (identificarOndaTriangular()) {
        tipoOnda = "Triangular";
    }
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

bool identificarOndaSenoidal() {
    int16_t pendienteAntesDelPico = 0;
    int16_t pendienteDespuesDelPico = 0;
    bool esSenoidal = true;

    // Recorremos el arreglo buscando los picos
    for (int i = 1; i < indiceActual - 1; i++) {
        // Detectamos un pico positivo
        if (arr[i] > arr[i - 1] && arr[i] > arr[i + 1]) {
            pendienteAntesDelPico = arr[i] - arr[i - 1];
            pendienteDespuesDelPico = arr[i + 1] - arr[i];
            
            if (abs(pendienteAntesDelPico - pendienteDespuesDelPico) > 30) {
                esSenoidal = false;
                break;
            }
        }
    }
    
    return esSenoidal;
}

bool identificarOndaTriangular() {
    int16_t pendienteAntesDelPico = 0;
    int16_t pendienteDespuesDelPico = 0;
    bool esTriangular = true;

    // Recorremos el arreglo buscando los picos
    for (int i = 1; i < indiceActual - 1; i++) {
        if (arr[i] > arr[i - 1] && arr[i] > arr[i + 1]) {
            pendienteAntesDelPico = arr[i] - arr[i - 1];
            pendienteDespuesDelPico = arr[i + 1] - arr[i];
            
            if (abs(pendienteAntesDelPico) != abs(pendienteDespuesDelPico)) {
                esTriangular = false;
                break;
            }
        }
    }
    
    return esTriangular;
}

void liberarMemoria() {
    delete[] arr;
    arr = nullptr;
}
