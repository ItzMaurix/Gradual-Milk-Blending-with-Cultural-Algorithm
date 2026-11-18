#include "solution.hpp"
#include "../constraints/constraints.hpp"

#include <algorithm>
#include <cmath>
#include <stdexcept>
#include <sstream>
#include <set>

using namespace std;

/*************************** 
double route_transport(const Instance& inst, const Route& route)
const Instance& inst: la instancia (para acceder a la matriz de costos)
const Route& route: la ruta a medir
retorna: el costo total de recorrer la ruta empezando y terminando en el deposito
***************************/
double route_transport(const Instance& inst, const Route& route) {
    if (route.farms.empty()) return 0.0;

    double total = inst.cost[0][route.farms[0]];
    for (size_t i = 0; i + 1 < route.farms.size(); i++) {
        total += inst.cost[route.farms[i]][route.farms[i + 1]];
    }
    total += inst.cost[route.farms.back()][0];
    return total;
}

/*************************** 
string check_structure(const Instance& inst, const Solution& sol)
const Instance& inst: la instancia del problema
const Solution& sol: la solucion a revisar
retorna: "" si la estructura es valida, o un mensaje de error si no
***************************/
string check_structure(const Instance& inst, const Solution& sol) {
    if ((int)sol.routes.size() != inst.num_trucks) {
        return "La solucion debe tener exactamente K=" + to_string(inst.num_trucks) + " rutas";
    }

    // Validar que truck_idx este en rango
    for (int r = 0; r < (int)sol.routes.size(); r++) {
        int k = sol.routes[r].truck_idx;
        if (k < 0 || k >= inst.num_trucks) {
            return "truck_idx fuera de rango en ruta " + to_string(r);
        }
    }

    // Cada granja debe aparecer exactamente una vez
    vector<int> seen(inst.num_farms, 0);
    for (const Route& route : sol.routes) {
        for (int f : route.farms) {
            if (f < 1 || f > inst.num_farms) {
                return "id de granja fuera de rango: " + to_string(f);
            }
            seen[f - 1]++;
        }
    }
    for (int i = 0; i < inst.num_farms; i++) {
        if (seen[i] != 1) {
            return "la granja " + to_string(i + 1) + " aparece " + to_string(seen[i]) + " veces (deberia ser 1)";
        }
    }

    return "";
}

/*************************** 
Metrics evaluate(const Instance& inst, const Solution& sol, double Pq, double Pc)
const Instance& inst: la instancia del problema
const Solution& sol: la solucion a evaluar (se asume estructura valida)
double Pq: peso de la penalizacion por litro faltante de cuota
double Pc: peso de la penalizacion por litro excedido de capacidad
retorna: un struct con todas las metricas calculadas
***************************/
Metrics evaluate(const Instance& inst, const Solution& sol, double Pq, double Pc) {
    Metrics m;
    m.delivered_per_grade.assign(3, 0.0);
    m.volume_per_truck.assign(inst.num_trucks, 0.0);

    // Paso 1: recorrer cada ruta, calcular blend natural y acumular metricas
    for (const Route& route : sol.routes) {
        if (route.farms.empty()) continue;

        double volume = 0.0;
        double sc_sum = 0.0;
        for (int f : route.farms) {
            const Farm& farm = inst.farms[f - 1];
            volume += farm.production;
            sc_sum += farm.production * farm.sc_per_ml;
        }

        // Grado natural del blend segun SC/ml ponderado
        double blend_sc = sc_sum / volume;
        int grade;
        if (blend_sc <= inst.sc_upper[0]) grade = 0;       // A
        else if (blend_sc <= inst.sc_upper[1]) grade = 1;  // B
        else grade = 2;                                     // C

        // Sumar a las metricas
        m.delivered_per_grade[grade] += volume;
        m.volume_per_truck[route.truck_idx] += volume;
        m.revenue += volume * inst.revenue[grade];
        m.transport_cost += route_transport(inst, route);
    }

    double cap_viol = capacity_violation(inst, m.volume_per_truck);
    double q_viol = quota_violation(m.delivered_per_grade, inst.quota);

    m.penalty_capacity = Pc * cap_viol;
    m.penalty_quota = Pq * q_viol;

    // Paso 3: profit y factibilidad
    m.profit = m.revenue - m.transport_cost - m.penalty_capacity - m.penalty_quota;
    const double EPS = 1e-6;
    m.feasible = (cap_viol <= EPS) && (q_viol <= EPS);

    return m;
}