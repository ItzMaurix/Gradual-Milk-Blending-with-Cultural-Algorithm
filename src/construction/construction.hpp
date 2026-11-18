#pragma once

#include "../instance/instance.hpp"
#include "../solution/solution.hpp"
#include <vector>

using namespace std;

/*************************** 
struct InsertionCandidate
int farm_id: id de la granja a insertar
int truck_idx: indice del camion donde se inserta
int position: posicion dentro de la ruta (0..size, 0=al inicio, size=al final)
double delta_cost: costo de insertar (delta_transport - expected_revenue)
***************************/
struct InsertionCandidate {
    int farm_id;
    int truck_idx;
    int position;
    double delta_cost;
};

/*************************** 
Solution graspx(const Instance& inst, int k_rcl)
const Instance& inst: la instancia del problema
int k_rcl: tamano de la lista restringida de candidatos (RCL)
retorna: una solucion construida por GRASP
***************************/
Solution graspx(const Instance& inst, int k_rcl);

/*************************** 
vector<Solution> build_initial_population(const Instance& inst, int pop_size, int k_rcl)
const Instance& inst: la instancia del problema
int pop_size: cantidad de soluciones a generar
int k_rcl: tamano de la RCL para cada construccion
retorna: un vector de soluciones iniciales
***************************/
vector<Solution> build_initial_population(const Instance& inst, int pop_size, int k_rcl);