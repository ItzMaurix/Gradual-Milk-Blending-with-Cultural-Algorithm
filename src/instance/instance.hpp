#pragma once

#include <vector>
#include <string>
#include <map>

using namespace std;

/*************************** 
struct Farm
int id: número que identifica a la granja (1, 2, 3, ...)
double production: cuántos litros de leche produce esta granja
int grade: tipo de leche (1=calidad A, 2=calidad B, 3=calidad C)
double sc_per_ml: cuántas células somáticas tiene la leche de esta granja
***************************/
struct Farm {
    int id;
    double production;
    int grade;
    double sc_per_ml;
};

/*************************** 
struct Truck
int id: número que identifica al camión (1, 2, 3, ...)
double capacity: cuántos litros puede cargar este camión
***************************/
struct Truck {
    int id;
    double capacity;
};

/*************************** 
struct Instance
int depot_id: número del depósito de origen (siempre es 1000)
int num_farms: cantidad total de granjas en la instancia
int num_trucks: cantidad total de camiones disponibles
int num_grades: cantidad de tipos de leche (siempre son 3)
vector<Farm> farms: lista de granjas ordenada por id
vector<Truck> trucks: lista de camiones disponibles
vector<double> quota: cuota mínima a entregar de cada tipo de leche (A, B, C). Si vale [0,0,0] no hay cuota
vector<double> revenue: precio por litro de cada tipo de leche (A, B, C)
vector<double> sc_upper: límite máximo de células somáticas para cada tipo (A, B, C)
vector<vector<double>> cost: matriz de distancias entre nodos. cost[0] es el depósito, cost[i] es la granja con id i
***************************/
struct Instance {
    int depot_id = 1000;
    int num_farms = 0;
    int num_trucks = 0;
    int num_grades = 3;

    vector<Farm> farms;
    vector<Truck> trucks;
    vector<double> quota;
    vector<double> revenue;
    vector<double> sc_upper;
    vector<vector<double>> cost;
};

/*************************** 
Instance parse_instance(const string& filepath)
const string& filepath: ruta al archivo de instancia
retorna: una instancia con todos los datos del archivo organizados
***************************/
Instance parse_instance(const string& filepath);

/*************************** 
void print_instance(const Instance& inst)
const Instance& inst: la instancia que se quiere imprimir por pantalla
retorna: nada, solo imprime un resumen en pantalla
***************************/
void print_instance(const Instance& inst);