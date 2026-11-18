#include "constraints.hpp"

#include <algorithm>
#include <cmath>

using namespace std;

/*************************** 
double capacity_violation(const Instance& inst, const vector<double>& volume_per_truck)
const Instance& inst: la instancia del problema
const vector<double>& volume_per_truck: litros cargados por cada camion (tamano K)
retorna: suma de litros excedidos sobre la capacidad de cada camion
***************************/
double capacity_violation(const Instance& inst, const vector<double>& volume_per_truck) {
    double total = 0.0;
    for (int k = 0; k < inst.num_trucks; k++) {
        double excess = volume_per_truck[k] - inst.trucks[k].capacity;
        if (excess > 0.0) total += excess;
    }
    return total;
}

/*************************** 
double quota_violation(const vector<double>& delivered, const vector<double>& quota)
const vector<double>& delivered: litros entregados por grado natural (A=0, B=1, C=2)
const vector<double>& quota: cuota minima por grado
retorna: suma de litros faltantes despues de aplicar degradacion (A->B->C)
***************************/
double quota_violation(const vector<double>& delivered, const vector<double>& quota) {
    double A = delivered[0], B = delivered[1], C = delivered[2];
    double QA = quota[0], QB = quota[1], QC = quota[2];

    // Cuota A no se puede llenar con nada (A es el mejor grado)
    double shortA = max(0.0, QA - A);
    double excessA = max(0.0, A - QA);

    // Cuota B se llena con B primero, luego con exceso de A
    double shortB = max(0.0, QB - B);
    double excessB = max(0.0, B - QB);
    double useAforB = min(excessA, shortB);
    shortB -= useAforB;
    excessA -= useAforB;

    // Cuota C se llena con C primero, luego con exceso de B, luego con exceso de A
    double shortC = max(0.0, QC - C);
    double useBforC = min(excessB, shortC);
    shortC -= useBforC;
    excessB -= useBforC;

    double useAforC = min(excessA, shortC);
    shortC -= useAforC;

    return shortA + shortB + shortC;
}