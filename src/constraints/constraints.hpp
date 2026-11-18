#pragma once

#include "../instance/instance.hpp"
#include <vector>

using namespace std;

/*************************** 
double capacity_violation(const Instance& inst, const vector<double>& volume_per_truck)
const Instance& inst: la instancia del problema
const vector<double>& volume_per_truck: litros cargados por cada camion (tamano K)
retorna: suma de litros excedidos sobre la capacidad de cada camion
***************************/
double capacity_violation(const Instance& inst, const vector<double>& volume_per_truck);

/*************************** 
double quota_violation(const vector<double>& delivered, const vector<double>& quota)
const vector<double>& delivered: litros entregados por grado natural (A=0, B=1, C=2)
const vector<double>& quota: cuota minima por grado
retorna: suma de litros faltantes despues de aplicar degradacion (A->B->C)
***************************/
double quota_violation(const vector<double>& delivered, const vector<double>& quota);