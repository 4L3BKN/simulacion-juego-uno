#include <iostream>
#include <sys/mman.h>    
#include <unistd.h>      
#include <fcntl.h>       
#include <sys/wait.h>    
#include <semaphore.h>   
#include <cstring>       
#include <cstdlib>       
#include <vector>
#include <string>
#include <algorithm> 
#include <ctime> 
#include <sys/stat.h> 
#include <iterator>
#include <random>

using namespace std;

//Struct carta
struct Carta {
    int color;  //Números del 1 al 4: 1 (rojo), 2(Verde), 3(Azul), 4(Amarillo)
    int numero; //Números del 0 al 9 y -1 (skip), -2 (+2), -3 (reverse), -4 (+4), -5 (cambio de color), -8 (+2(listo)) , -9 (+4(listo))

/*
Función mostrarCarta: Muestra la carta en la consola, con el color y el número o tipo de carta, transformando los valores de color y número a su respectivo nombre.
*/
    void mostrarCarta(){
        char colorNombre[10] = "";  
        char colorTipo[30] = "";
 
        //Asigna el nombre del color según el valor de color
        if(this->color == 1){
            strcpy(colorNombre, "Rojo");
        }else if(this->color == 2){
            strcpy(colorNombre, "Verde");
        }else if(this->color == 3){
            strcpy(colorNombre, "Azul");
        }else if(this->color == 4){
            strcpy(colorNombre, "Amarillo");
        } 
        if (this->numero < 0) {
            if(this->numero == -1){
                strcpy(colorTipo, "Skip");
            } else if(this->numero == -2 || this->numero == -8){
                strcpy(colorTipo, "+2");
            } else if(this->numero == -3){
                strcpy(colorTipo, "Reverse");
            } else if(this->numero == -4 || this->numero == -9){
                strcpy(colorTipo, "+4");
            } else if(this->numero == -5){
                strcpy(colorTipo, "Cambio de color");
            }
            //Mostrar la carta con su color y tipo
            cout << colorNombre << " " << colorTipo << endl;
        } else {
            //Mostrar la carta con su color y número
            cout << colorNombre << " " << this->numero << endl;
        }
        
    }
};

//Struct para memoria compartida: Tablero contiene la información del juego tales como los turnos, el mazo(con su tamaño), la carta tope, la dirección del juego y si el juego ha terminado.
struct Tablero {
    int turno;
    bool juegoTerminado;
    Carta cartaTope;
    Carta* mazo;
    int mazoSize;
    int mazoMaxSize;
    int direccion; //1 va hacia la derecha (creciente), -1 va hacia la izquierda (izquierda).
};

/*
Funcion llenarMazo: Llena el mazo con las cartas del juego UNO, creando un vector temporal con las cartas y luego copiando las cartas al arreglo de cartas en la estructura Tablero, 
además de inicializar el tamaño del mazo con sus respectivas 108 cartas.
*/

void llenarMazo(Tablero* tablero) {
    tablero->mazo = new Carta[108];
    
    //Inicializar el tamaño del mazo
    tablero->mazoSize = 0;
    tablero->mazoMaxSize = 108;

    //Vector temporal para almacenar las cartas antes de pasarlas al arreglo
    vector<Carta> mazoTemporal;

    //Llenar el vector temporal con las cartas
    
    for (int i = 1; i <= 4; i++) {
        mazoTemporal.push_back({i, 0}); // Cartas de 0
        for (int j = 0; j < 2; j++) {
            mazoTemporal.push_back({i, -1});  // Cartas de skip
            mazoTemporal.push_back({i, -3});  // Cartas de reverse
            mazoTemporal.push_back({i, -2});  // Cartas de +2
        }
        mazoTemporal.push_back({0, -4});  // Cartas de +4
        mazoTemporal.push_back({0, -5});  // Cambio de color
    }
    
    for (int i = 1; i <= 4; i++) {
        for (int j = 0; j < 18; j++) {
            int num = (j % 9) + 1;
            mazoTemporal.push_back({i, num});   // Cartas de 1 a 9
        }
    }

    
    //Verificar que el tamaño del vector sea correcto
    if (mazoTemporal.size() != 108) {
        cerr << "Error: El tamaño del mazo temporal no es correcto." << mazoTemporal.size() << endl;
        exit(1);
    }

    //Copiar las cartas del vector al arreglo estático en la estructura Tablero
    for (int i = 0; i < tablero->mazoMaxSize; i++) {
        tablero->mazo[i] = mazoTemporal[i];
        tablero->mazoSize++; 
    }
}


//Función para mezclar el mazo: Se utiliza la función shuffle de la librería algorithm para mezclar las cartas del mazo.
void mezclarMazo(Tablero* tablero) {
    srand(static_cast<unsigned>(time(0)));
    shuffle((tablero->mazo), (tablero->mazo) + tablero->mazoSize, default_random_engine(rand()));
}

//Función para repartir cartas: Se le reparten 7 cartas a cada jugador, sacandolas desde el mazo.
void repartirCartas(vector<Carta>& mano, Tablero* tablero) {
    for (int i = 0; i < 7; i++) {//Repartir 7 Cartas desde el mazo
        if(tablero->mazoSize > 0){
            Carta ayuda = tablero->mazo[--(tablero->mazoSize)];
            mano.push_back(ayuda);  
        }else{
            cout<<"El mazo esta vacio, no se pueden repartir más cartas"<<endl;
            exit(1);
        }
    }
}

//Función para verificar si se puede jugar la carta: Es una funcion fundamental en el desarrollo del juego, se verifica si la carta jugada es válida para ser jugada comparandola con la carta tope de la pila.
bool puedeJugar(const Carta& cartaJugada, const Carta& cartaTope) {
    return (cartaJugada.color == cartaTope.color || cartaJugada.numero == cartaTope.numero || cartaJugada.numero == -4 || cartaJugada.numero == -5 || (cartaJugada.numero == -2 && ((cartaTope.numero == -8) || (cartaTope.numero == -2))));
}

//Función para sacar una carta del mazo: Se saca la última carta del mazo y se disminuye el tamaño del mazo, si el maoz esta vacio no dejara sacar mas cartas.
Carta sacarCarta(Tablero* tablero) {
    if (tablero->mazoSize <= 0) {
        cout << "El mazo está vacío, no se puede robar más cartas." << endl;
        cout<<"Se termina el juego sin ganador"<<endl;
        tablero->juegoTerminado = true;
        Carta respuesta = {-1,-10};//Carta vacia.
        return respuesta;
    }
    return tablero->mazo[--tablero->mazoSize]; //Devolver la última carta
}

//Función para crear memoria compartida con mmap: Se crea la memoria compartida con mmap y se retorna un puntero a la memoria compartida.
Tablero* crearMemoriaCompartida(size_t size) {
    //Crear memoria compartida utilizando mmap
    Tablero* ptr = (Tablero*)mmap(NULL, size, PROT_READ | PROT_WRITE, MAP_SHARED | MAP_ANONYMOUS, -1, 0);
    if (ptr == MAP_FAILED) {
        cerr << "Error al mapear memoria compartida con mmap." << endl;
        exit(1);
    }
    return ptr;
}

//Función para ir actualizando el turno: Se actualiza el turno del juego, se le suma la dirección y se verifica si el turno es menor a 0, en ese caso se le suma 4 para que el turno sea positivo.
void actualizarTurno(Tablero* tablero) {
    tablero->turno = (tablero->turno + tablero->direccion) % 4;
    if (tablero->turno < 0) {
        tablero->turno += 4; 
    }
}

//Función para cambiar el sentido del juego: Se cambia la dirección del juego, si la dirección es 1 se cambia a -1 y viceversa.
void jugarCambioSentido(Tablero* tablero) {
    tablero->direccion = -tablero->direccion; //Cambiar de dirección
    cout << "Se ha cambiado el sentido del juego!" << endl;
}



// Función principal: Se crea la memoria compartida, se inicializan los valores iniciales del juego, se reparten las cartas a los jugadores y se inicia el juego
// aca ocurre la mayor parte de la lógica del juego.
int main() {

    srand(time(0)); //Semilla para la función rand
    pid_t pid; //Variable para guardar el pid de los procesos hijos
    size_t size = sizeof(Tablero); //Tamaño de la memoria compartida
    
    // Crear memoria compartida con mmap
    void* ptr = mmap(NULL, size, PROT_READ | PROT_WRITE, MAP_SHARED | MAP_ANONYMOUS, -1, 0);
    if (ptr == MAP_FAILED) {
        cerr << "Error al mapear memoria compartida con mmap." << endl;
        exit(1);
    }

    // Castear el puntero a Tablero
    Tablero* tablero = static_cast<Tablero*>(ptr);
    if (tablero == nullptr) {
        cerr << "Error: El puntero a Tablero es nulo." << endl;
        exit(1);
    }

    llenarMazo(tablero); //Llenar el mazo con las cartas del juego
    mezclarMazo(tablero); //Mezclar las cartas del mazo

    // Inicializar valores en la memoria compartida
    if (tablero->mazoSize > 0) { //Establecer la carta tope
        tablero->cartaTope = tablero->mazo[--(tablero->mazoSize)];   
    } else {
        cerr << "Error: No hay cartas en el mazo para establecer la carta tope." << endl; 
        exit(1); 
    }

    //Inicializar valores iniciales
    tablero->turno = 0; 
    tablero->direccion = 1; 
    tablero->juegoTerminado = false; 

    //Repartir cartas a los jugadores: Se reparten 7 cartas a cada jugador
    vector<Carta> manos[4];
    for (int i = 0; i < 4; i++) {
        repartirCartas(manos[i], tablero);
    }

    while ((tablero->cartaTope.numero == -1) or (tablero->cartaTope.numero == -2) or (tablero->cartaTope.numero == -3) or (tablero->cartaTope.numero == -4) or (tablero->cartaTope.numero == -5)) {
        mezclarMazo(tablero);
        tablero->cartaTope = tablero->mazo[--(tablero->mazoSize)];  
    }

    for (int i = 0; i < 4; i++) { //for para lograr 4 procesos hijos
        pid = fork(); //Crear un proceso hijo
        if (pid == 0) { //Proceso hijo
            while (!tablero->juegoTerminado && tablero->mazoSize > 0) { //Mientras el juego no haya terminado y el mazo tenga cartas proseguir con el juego
                if(tablero->turno == i){ //Verificar si es tu turno
                    bool saltarTurno = false;

                    if(i == 0){

                        if (tablero->cartaTope.numero == -2) { // Si la carta del tope es +2, el jugador debe robar 2 cartas y perder el turno
                            for (int k = 0; k < 2; k++) { 
                                manos[0].push_back(sacarCarta(tablero));
                            }
                            cout<< "\n"<<"Robaste 2 cartas"<<endl;
                            tablero->cartaTope.numero = -8;
                            saltarTurno = true;

                        } else if (tablero->cartaTope.numero == -4) { // Si la carta del tope es +4, el jugador debe robar 4 cartas y perder el turno
                            for (int k = 0; k < 4; k++) { 
                                manos[0].push_back(sacarCarta(tablero)); 
                            }
                            cout<< "\n"<<"Robaste 4 cartas"<<endl;
                            tablero->cartaTope.numero = -9;
                            saltarTurno = true;
                        }

                        if (saltarTurno == false) { // Si no se debe saltar el turno, jugar...
                            cout<<endl;
                            cout<<"Te toca jugar. La carta en el tope es: " << endl;
                            tablero->cartaTope.mostrarCarta(); //Mostrar la carta en el tope
                            cout<< "\n" << "Tus cartas:" << endl; //Mostrar las cartas del jugador
                            for(size_t j = 0; j < manos[0].size(); j++){
                                cout<< j + 1 <<":";
                                manos[0][j].mostrarCarta();
                            }

                            int eleccion; // Carta elegida por el jugador
                            cout<< "\n" << "Elige una carta para jugar (0 para sacar otra carta):"; 
                            cin>> eleccion;

                            if(eleccion > 0 && eleccion <= (int)manos[0].size()){ 
                                Carta cartaJugada = manos[0][eleccion - 1]; // Carta seleccionada
                                if(puedeJugar(cartaJugada, tablero->cartaTope)){
                                    
                                    tablero->cartaTope = cartaJugada; // Actualizar la carta tope
                                    
                                    cout<<"Se jugó: " ;
                                    tablero->cartaTope.mostrarCarta(); // Mostrar la carta jugada

                                    // --- Efectos de las cartas especiales ---
                                    if (cartaJugada.numero == -1) {  // Carta skip
                                        cout << "El siguiente jugador pierde su turno!" << endl;
                                        actualizarTurno(tablero);

                                    } else if (cartaJugada.numero == -2) {  /// Carta +2
                                        cout << "El siguiente jugador debe robar 2 cartas y pierde el turno!" << endl;
                                        
                                    }else if(cartaJugada.numero == -3){  /// Carta Cambio de dirección
                                        jugarCambioSentido(tablero);
                                        
                                    } else if(cartaJugada.numero == -4){ // Carta +4
                                        int col;
                                        cout<< "\n" << "Elige un color: 1=Rojo, 2=Verde, 3=Azul, 4=Amarillo"<<endl;
                                        cin>> col;

                                        tablero->cartaTope.color = col; //Cambiar el color del tope al color elegido
                                        tablero->cartaTope.numero = -4;
                                        cout << "El siguiente jugador debe robar 4 cartas y pierde el turno!" << endl;
                                        
                                        
                                    }else if(cartaJugada.numero == -5){ //Carta cambio color
                                        int col;
                                        cout<< "Elige un color: 1=Rojo, 2=Verde, 3=Azul, 4=Amarillo"<<endl;
                                        cin>> col;

                                        tablero->cartaTope.color = col; //Cambiar el color del tope al color elegido
                                        tablero->cartaTope.numero = -5;                                    
                                    }

                                    manos[0].erase(manos[0].begin() + (eleccion - 1)); // Eliminar la carta jugada de la mano del jugador

                                } else {
                                    cout<<"No puedes jugar esa carta"<< endl;
                                }
                            } else if(eleccion == 0){ //Si elige 0, sacar otra carta
                                cout<<"Sacando carta..."<<endl;
                                Carta cartaSacada = sacarCarta(tablero); // Sacar una carta del mazo
                                if(cartaSacada.color == -1){
                                    break;
                                }
                                cout<<"Sacaste: "<<endl; 
                                cartaSacada.mostrarCarta(); // Mostrar la carta sacada
                                
                                
                                if(puedeJugar(cartaSacada, tablero->cartaTope)){
                                    
                                    tablero->cartaTope = cartaSacada; // Actualizar la carta tope

                                    cout<<"Se jugó: "<<endl;
                                    cartaSacada.mostrarCarta(); // Mostrar la carta jugada

                                    // --- Efectos de las cartas especiales ---
                                    if (cartaSacada.numero == -1) {  // Carta skip
                                        cout << "El siguiente jugador pierde su turno!" << endl;
                                        actualizarTurno(tablero);

                                    } else if (cartaSacada.numero == -2) {  // Carta +2
                                        cout << "El siguiente jugador debe robar 2 cartas y pierde el turno!" << endl;

                                    }else if(cartaSacada.numero == -3){  // Carta Cambio de dirección
                                        jugarCambioSentido(tablero);
                                        
                                    } else if(cartaSacada.numero == -4){ // Carta +4
                                        int col;
                                        cout<< "Elige un color: 1=Rojo, 2=Verde, 3=Azul, 4=Amarillo"<<endl;
                                        cin>> col;

                                        tablero->cartaTope.color = col; //Cambiar el color del tope al color elegido    
                                        tablero->cartaTope.numero = -4;
                                        cout << "El siguiente jugador debe robar 4 cartas y pierde el turno!" << endl;

                                        

                                    }else if(cartaSacada.numero == -5){ //Carta cambio color
                                        int col;
                                        cout<< "Elige un color: 1=Rojo, 2=Verde, 3=Azul, 4=Amarillo"<<endl;
                                        cin>> col;

                                        tablero->cartaTope.color = col; //Cambiar el color del tope al color elegido
                                        tablero->cartaTope.numero = -5;                                    
                                    }

                                    cout<<"Carta tope actualizada: "<<endl;
                                    // tablero->cartaTope.mostrarCarta(); 
                                
                                } else {
                                    manos[0].push_back(cartaSacada); // Si no se puede jugar la carta robada, agregarla a la mano del jugador
                                    cout<<"No se pudo jugar la carta sacada"<< endl;
                                }
                                
                            }

                            if(manos[0].empty()){ // Si el jugador se queda sin cartas, terminar el juego
                                cout<<"Ganaste!!!"<<endl;
                                tablero->juegoTerminado = true;
                            }
                        }
                    

                   
                    }else{

                        if (tablero->cartaTope.numero == -2) { // Si la carta del tope es +2, el jugador debe robar 2 cartas y perder el turno
                            for (int k = 0; k < 2; k++) {
                                manos[i].push_back(sacarCarta(tablero));
                            }
                            cout<< "\n"<<"Jugador "<< i + 1 << " ha robado 2 cartas"<<endl;
                            tablero->cartaTope.numero = -8;
                            saltarTurno = true;

                        } else if (tablero->cartaTope.numero == -4) { // Si la carta del tope es +4, el jugador debe robar 4 cartas y perder el turno
                            for (int k = 0; k < 4; k++) {
                                manos[i].push_back(sacarCarta(tablero));
                            }
                            cout<< "\n"<<"Jugador "<< i + 1 << " ha robado 4 cartas"<<endl;
                            tablero->cartaTope.numero = -9;
                            saltarTurno = true;
                        }
                        if (saltarTurno == false){ // Si no se debe saltar el turno, jugar...

                            bool juego = false;

                            //Contar cuantas cartas tiene el bot para mostrarlo en pantalla
                            int contador = 0;
                            for(size_t j = 0; j < manos[i].size(); j++){
                                contador++;
                            }
                            
                            cout<< "\n" <<"Jugador "<< i + 1 << "(bot) "<< "tiene " << contador << " cartas." << endl;
                            
                            // tablero->cartaTope.mostrarCarta(); //Mostrar la carta en el tope

                            /*Muestra las cartas del bot que este jugando.
                            cout<<"Cartas:"<<endl;
                            for(size_t j = 0; j < manos[i].size(); j++){
                                cout<< j + 1 <<":";
                                manos[i][j].mostrarCarta();
                            }
                            */
                            for(size_t j = 0; j < manos[i].size(); j++){ //Recorrer las cartas del bot para saber si puede jugar alguna
                                if(puedeJugar(manos[i][j], tablero->cartaTope) && (saltarTurno == false)){ //Si puede jugar, proceder a jugar

                                    tablero->cartaTope = manos[i][j]; // Actualizar la carta tope

                                    // --- Efectos de las cartas especiales ---
                                    if (manos[i][j].numero == -1) {  // Carta skip
                                        cout << "El siguiente jugador pierde su turno!" << endl;
                                        actualizarTurno(tablero);

                                    } else if (manos[i][j].numero == -2) {  // Carta +2
                                        cout << "El siguiente jugador debe robar 2 cartas y pierde el turno!" << endl;

                                    }else if(manos[i][j].numero == -3){  // Carta Cambio de dirección
                                        jugarCambioSentido(tablero);
                                        
                                    } else if(manos[i][j].numero == -4){ // Carta +4
                                        
                                        int rojo = 0, verde = 0, azul = 0, amarillo = 0;

                                        // Recorrer la mano del jugador para contar cuántas cartas tiene de cada color
                                        for(size_t k = 0; k < manos[i].size(); k++) {
                                            switch (manos[i][k].color) {
                                                case 1: // Rojo
                                                    rojo++;
                                                    break;
                                                case 2: // Verde
                                                    verde++;
                                                    break;
                                                case 3: // Azul
                                                    azul++;
                                                    break;
                                                case 4: // Amarillo
                                                    amarillo++;
                                                    break;
                                                default:
                                                    break;
                                            }
                                        }

                                        // Determinar cuál color tiene más cartas
                                        int colorElegido;
                                        if (rojo >= verde && rojo >= azul && rojo >= amarillo) {
                                            colorElegido = 1; // Rojo
                                        } else if (verde >= rojo && verde >= azul && verde >= amarillo) {
                                            colorElegido = 2; // Verde
                                        } else if (azul >= rojo && azul >= verde && azul >= amarillo) {
                                            colorElegido = 3; // Azul
                                        } else {
                                            colorElegido = 4; // Amarillo
                                        }       

                                        tablero->cartaTope.color = colorElegido; // Cambiar el color de la carta tope al color con más cartas
                                        tablero->cartaTope.numero = -4;

                                        cout<<"El siguiente jugador debe robar 4 cartas y pierde el turno!" << endl;

                                    }else if(manos[i][j].numero == -5){ //Carta cambio color
                                        
                                        int rojo = 0, verde = 0, azul = 0, amarillo = 0; // Inicializar contadores para los colores

                                        // Recorrer la mano del jugador para contar cuántas cartas tiene de cada color
                                        for(size_t k = 0; k < manos[i].size(); k++) {
                                            switch (manos[i][k].color) {
                                                case 1: // Rojo
                                                    rojo++;
                                                    break;
                                                case 2: // Verde
                                                    verde++;
                                                    break;
                                                case 3: // Azul
                                                    azul++;
                                                    break;
                                                case 4: // Amarillo
                                                    amarillo++;
                                                    break;
                                                default:
                                                    break;
                                            }
                                        }

                                        // Determinar cuál color tiene más cartas
                                        int colorElegido;
                                        if (rojo >= verde && rojo >= azul && rojo >= amarillo) {
                                            colorElegido = 1; // Rojo
                                        } else if (verde >= rojo && verde >= azul && verde >= amarillo) {
                                            colorElegido = 2; // Verde
                                        } else if (azul >= rojo && azul >= verde && azul >= amarillo) {
                                            colorElegido = 3; // Azul
                                        } else {
                                            colorElegido = 4; // Amarillo
                                        }       

                                        tablero->cartaTope.color = colorElegido; // Cambiar el color de la carta tope al color con más cartas
                                        tablero->cartaTope.numero = -5;                                   
                                    }
                                    cout<< "Jugador"<< i + 1<< " jugó: ";
                                    tablero->cartaTope.mostrarCarta(); // Mostrar la carta jugada
                                    manos[i].erase(manos[i].begin() + j);  // Eliminar la carta jugada de la mano del jugador
                                    juego = true;
                                    break;
                                }
                            } 
                            if(!juego){ // Si no se pudo jugar ninguna carta, robar una carta

                                cout << "Jugador " << i + 1 << " no pudo jugar" << endl;
                                Carta cartaSacada = sacarCarta(tablero); // Sacar una carta del mazo
                                if(cartaSacada.color == -1){ // Si el mazo está vacío, terminar el juego
                                    break;
                                }
                                cout << "Jugador"<< i + 1 << " robó: " << endl;
                                cartaSacada.mostrarCarta(); // Mostrar la carta robada

                                if(puedeJugar(cartaSacada, tablero->cartaTope)){ // Si se puede jugar la carta robada, jugarla
                                    
                                    tablero->cartaTope = cartaSacada; // Actualizar la carta tope
                                    cout << "Se jugó la carta robada" << endl;                   


                                    // --- Efectos de las cartas especiales ---
                                    if (cartaSacada.numero == -1) {  // Carta skip
                                        cout << "El siguiente jugador pierde su turno!" << endl;
                                        actualizarTurno(tablero);

                                    } else if (cartaSacada.numero == -2) {  // Carta +2
                                        cout << "El siguiente jugador debe robar 2 cartas y pierde el turno!" << endl;

                                    }else if(cartaSacada.numero == -3){  // Carta Cambio de dirección
                                        jugarCambioSentido(tablero);

                                    } else if(cartaSacada.numero == -4){ // Carta +4
                                        
                                        int rojo = 0, verde = 0, azul = 0, amarillo = 0; // Inicializar contadores para los colores

                                        // Recorrer la mano del jugador para contar cuántas cartas tiene de cada color
                                        for(size_t k = 0; k < manos[i].size(); k++) {
                                            switch (manos[i][k].color) {
                                                case 1: // Rojo
                                                    rojo++;
                                                    break;
                                                case 2: // Verde
                                                    verde++;
                                                    break;
                                                case 3: // Azul
                                                    azul++;
                                                    break;
                                                case 4: // Amarillo
                                                    amarillo++;
                                                    break;
                                                default:
                                                    break;
                                            }
                                        }

                                        // Determinar cuál color tiene más cartas
                                        int colorElegido;
                                        if (rojo >= verde && rojo >= azul && rojo >= amarillo) {
                                            colorElegido = 1; // Rojo
                                        } else if (verde >= rojo && verde >= azul && verde >= amarillo) {
                                            colorElegido = 2; // Verde
                                        } else if (azul >= rojo && azul >= verde && azul >= amarillo) {
                                            colorElegido = 3; // Azul
                                        } else {
                                            colorElegido = 4; // Amarillo
                                        }       

                                        tablero->cartaTope.color = colorElegido; // Cambiar el color de la carta tope al color con más cartas
                                        tablero->cartaTope.numero = -4;

                                        cout << "El siguiente jugador debe robar 4 cartas y pierde el turno!" << endl;

                                    }else if(cartaSacada.numero == -5){ //Carta cambio color
                                
                                        int rojo = 0, verde = 0, azul = 0, amarillo = 0; // Inicializar contadores para los colores

                                        // Recorrer la mano del jugador para contar cuántas cartas tiene de cada color
                                        for(size_t k = 0; k < manos[i].size(); k++) {
                                            switch (manos[i][k].color) {
                                                case 1: // Rojo
                                                    rojo++;
                                                    break;
                                                case 2: // Verde
                                                    verde++;
                                                    break;
                                                case 3: // Azul
                                                    azul++;
                                                    break;
                                                case 4: // Amarillo
                                                    amarillo++;
                                                    break;
                                                default:
                                                    break;
                                            }
                                        }

                                        // Determinar cuál color tiene más cartas
                                        int colorElegido;
                                        if (rojo >= verde && rojo >= azul && rojo >= amarillo) {
                                            colorElegido = 1; // Rojo
                                        } else if (verde >= rojo && verde >= azul && verde >= amarillo) {
                                            colorElegido = 2; // Verde
                                        } else if (azul >= rojo && azul >= verde && azul >= amarillo) {
                                            colorElegido = 3; // Azul
                                        } else {
                                            colorElegido = 4; // Amarillo
                                        }       

                                        tablero->cartaTope.color = colorElegido; // Cambiar el color de la carta tope al color con más cartas
                                        tablero->cartaTope.numero = -5;                                   
                                    }
                                    
                                } else {
                                    manos[i].push_back(cartaSacada); // Si no se puede jugar la carta robada, agregarla a la mano del jugador
                                    cout<<"No se pudo jugar la carta sacada"<<endl;
                                }
                            }

                            if(manos[i].empty()){ // Si el jugador se queda sin cartas, terminar el juego
                                cout<<"Jugador"<< i + 1 << " ha ganado!"<<endl;
                                tablero->juegoTerminado = true;
                            }
                            
                        }
                        
                    }
                    actualizarTurno(tablero); // Avaanzar al siguiente jugador
                    saltarTurno = false; // Reiniciar la variable de saltar turno
                }

                usleep(1000000); // Pequeña espera antes de volver a intentar
            }
            if (tablero->mazoSize <= 0) { // Si el mazo se queda sin cartas, terminar el juego
                cout << "El mazo está vacío, no se puede robar más cartas." << endl;
            }

            // Salir del proceso hijo
            delete[] tablero->mazo; // Liberar memoria del mazo
            munmap(tablero, size);  //Desmapear memoria compartida
            return 0;
        }
    }

    
    //Esperar a que los hijos terminen
    for (int i = 0; i < 4; i++) {
        wait(NULL);
    }

    //Liberar recursos en el proceso padre
    delete[] tablero->mazo; // Liberar memoria del mazo
    munmap(tablero, size);  //Desmapear memoria compartida
    
    return 0;
}