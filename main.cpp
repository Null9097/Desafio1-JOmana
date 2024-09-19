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

// Variables de frecuencia y amplitud
int16_t maxValue = INT16_MIN;
int16_t minValue = INT16_MAX;
float frecuencia = 0;
float amplitud = 0;
int crucePorCero = 0; // Contador para cruces por el punto medio
unsigned long tiempoInicio = 0; // Tiempo de inicio de la adquisición
unsigned long tiempoTotal = 0;  // Tiempo total de la adquisición

// Variables para identificación de onda
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

// Función para iniciar adquisición de datos
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

// Función para detener adquisición de datos
void detenerAdquisicion() {
    acquiringData = false;
    tiempoTotal = millis() - tiempoInicio; // Calcular tiempo total de adquisición
    
    calcularAmplitud();
    calcularFrecuencia(); // Basado en cruces por el punto medio
    identificarTipoOnda(); // Lógica para identificar la onda

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

    // Mostrar datos en el Monitor Serial
    Serial.println("Datos recopilados:");
    Serial.println();
    Serial.print("Amplitud: ");
    Serial.println(amplitud, 2);
    Serial.print("Frecuencia: ");
    Serial.println(frecuencia, 2);
    Serial.print("Tipo de onda: ");
    Serial.println(tipoOnda);
}

// Función para adquirir datos del pin analógico
void adquirirDatos() {
    int16_t valor = analogRead(analogPin);
    arr[indiceActual] = valor;
    Serial.println(valor);

    // Actualizar valores máximo y mínimo
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

// Función para calcular la amplitud
void calcularAmplitud() {
    amplitud = (maxValue - minValue) * (5.0 / 1023.0);
}

// Función para calcular la frecuencia
void calcularFrecuencia() {
    if (crucePorCero > 1 && tiempoTotal > 0) {
        float tiempoSegundos = tiempoTotal / 1000.0; // Convertir a segundos
        frecuencia = (crucePorCero / 2.0) / tiempoSegundos; // Dividir cruces por 2 para obtener ciclos completos
    } else {
        frecuencia = 0;
    }
}

// Función para identificar el tipo de onda
void identificarTipoOnda() {
    if (indiceActual < 20) { // Asegúrate de tener suficientes muestras
        lcd.clear();
        lcd.print("Datos insuficientes");
        return;
    }

    tipoOnda = "Desconocida"; // Inicializar como desconocida

    if (identificarOndaCuadrada()) {
        tipoOnda = "Cuadrada";
    } else if (identificarOndaSenoidal()) {
        tipoOnda = "Senoidal";
    } else if (identificarOndaTriangular()) {
        tipoOnda = "Triangular";
    }
}

/**
 * Identifica si la onda es cuadrada.
 */
bool identificarOndaCuadrada() {
    int16_t nivel1 = arr[0];
    int16_t nivel2 = 0;
    bool cambioDetectado = false;
    
    for (int i = 1; i < indiceActual; i++) {
        if (!cambioDetectado && abs(arr[i] - nivel1) > 100) { // Detectar un salto significativo
            nivel2 = arr[i];
            cambioDetectado = true;
        }
        // Si encontramos un tercer nivel distinto de nivel1 o nivel2, no es cuadrada
        else if (cambioDetectado && abs(arr[i] - nivel1) > 100 && abs(arr[i] - nivel2) > 100) {
            return false;
        }
    }

    return true; // Si solo tiene dos niveles, es cuadrada
}

/**
 * Identifica si la onda es senoidal.
 */
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
            
            // Verificar si la pendiente cambia gradualmente
            if (abs(pendienteAntesDelPico - pendienteDespuesDelPico) > 20) { // Ajustar umbral si es necesario
                esSenoidal = false; // Si cambia abruptamente, probablemente no es senoidal
                break;
            }
        }
    }
    
    return esSenoidal;
}

/**
 * Identifica si la onda es triangular.
 */
bool identificarOndaTriangular() {
    int16_t pendienteAntesDelPico = 0;
    int16_t pendienteDespuesDelPico = 0;
    bool esTriangular = true;

    // Recorremos el arreglo buscando los picos
    for (int i = 1; i < indiceActual - 1; i++) {
        // Detectamos un pico positivo
        if (arr[i] > arr[i - 1] && arr[i] > arr[i + 1]) {
            pendienteAntesDelPico = arr[i] - arr[i - 1];
            pendienteDespuesDelPico = arr[i + 1] - arr[i];
            
            // Para una onda triangular, la pendiente solo cambia de signo pero se mantiene constante
            if (abs(pendienteAntesDelPico) != abs(pendienteDespuesDelPico)) {
                esTriangular = false;
                break;
            }
        }
    }
    
    return esTriangular;
}

/**
 * Libera la memoria utilizada por el arreglo dinámico.
 */
void liberarMemoria() {
    delete[] arr;
    arr = nullptr;
}
