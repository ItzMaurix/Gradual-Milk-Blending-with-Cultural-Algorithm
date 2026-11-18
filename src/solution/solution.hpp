#pragma once

#include "../instance/instance.hpp"
#include <vector>
#include <string>

using namespace std;

/*************************** 
struct Route
vector<int> farms: lista de ids de granjas visitadas en orden (1..n)
int truck_idx: indice (0..K-1) del camion asignado a esta ruta
***************************/
struct Route {
    vector<int> farms;
    int truck_idx;
};

/*************************** 
struct Solution
vector<Route> routes: una ruta por camion disponible. Puede haber rutas vacias
***************************/
struct Solution {
    vector<Route> routes;
};

/*************************** 
struct Metrics
double revenue: dinero ganado por la leche entregada (segun grado final del blend)
double transport_cost: dinero gastado en transporte
double penalty_capacity: litros excedidos de capacidad (ya ponderados por Pc)
double penalty_quota: litros faltantes de cuota (ya ponderados por Pq)
double profit: revenue - transport_cost - penalty_capacity - penalty_quota
bool feasible: true si no hay penalizaciones (capacidad y cuota cumplidas)
vector<double> delivered_per_grade: litros entregados por grado natural (A=0, B=1, C=2)
vector<double> volume_per_truck: litros cargados por camion (tamano K)
***************************/
struct Metrics {
    double revenue = 0.0;
    double transport_cost = 0.0;
    double penalty_capacity = 0.0;
    double penalty_quota = 0.0;
    double profit = 0.0;
    bool feasible = false;
    vector<double> delivered_per_grade;  // tamano 3
    vector<double> volume_per_truck;     // tamano K
};

/*************************** 
string check_structure(const Instance& inst, const Solution& sol)
const Instance& inst: la instancia del problema
const Solution& sol: la solucion a revisar
retorna: "" si la estructura es valida, o un mensaje de error si no
***************************/
string check_structure(const Instance& inst, const Solution& sol);

/*************************** 
Metrics evaluate(const Instance& inst, const Solution& sol, double Pq, double Pc)
const Instance& inst: la instancia del problema
const Solution& sol: la solucion a evaluar (se asume estructura valida)
double Pq: peso de la penalizacion por litro faltante de cuota
double Pc: peso de la penalizacion por litro excedido de capacidad
retorna: un struct con todas las metricas calculadas
***************************/
Metrics evaluate(const Instance& inst, const Solution& sol, double Pq, double Pc);

/*************************** 
double route_transport(const Instance& inst, const Route& route)
const Instance& inst: la instancia (para acceder a la matriz de costos)
const Route& route: la ruta a medir
retorna: el costo total de recorrer la ruta empezando y terminando en el deposito
***************************/
double route_transport(const Instance& inst, const Route& route);