#include "construction.hpp"
#include "../utils/random.hpp"

#include <algorithm>
#include <cmath>
#include <limits>

using namespace std;

/*************************** 
double find_best_insertion(const Instance& inst, const Route& route, int farm_id, int& best_pos)
const Instance& inst: la instancia (para matriz de costos)
const Route& route: la ruta donde se intenta insertar
int farm_id: id de la granja a insertar
int& best_pos: salida, posicion optima encontrada
retorna: delta de transporte (costo nuevo - costo viejo) o +infinito si no hay insercion valida
***************************/
static double find_best_insertion(const Instance& inst, const Route& route, int farm_id, int& best_pos) {
    double best_delta = numeric_limits<double>::infinity();
    best_pos = -1;

    // Si la ruta esta vacia, la insercion es depot->farm->depot
    if (route.farms.empty()) {
        double delta = inst.cost[0][farm_id] + inst.cost[farm_id][0];
        best_delta = delta;
        best_pos = 0;
        return best_delta;
    }

    // Insertar al inicio: depot->farm->farm[0]->...
    {
        double old = inst.cost[0][route.farms[0]];
        double now = inst.cost[0][farm_id] + inst.cost[farm_id][route.farms[0]];
        double delta = now - old;
        if (delta < best_delta) { best_delta = delta; best_pos = 0; }
    }

    // Insertar entre farms i-1 e i
    for (size_t i = 1; i < route.farms.size(); i++) {
        int prev = route.farms[i - 1];
        int curr = route.farms[i];
        double old = inst.cost[prev][curr];
        double now = inst.cost[prev][farm_id] + inst.cost[farm_id][curr];
        double delta = now - old;
        if (delta < best_delta) { best_delta = delta; best_pos = (int)i; }
    }

    // Insertar al final: ...->farm[last]->farm->depot
    {
        int last = route.farms.back();
        double old = inst.cost[last][0];
        double now = inst.cost[last][farm_id] + inst.cost[farm_id][0];
        double delta = now - old;
        if (delta < best_delta) { best_delta = delta; best_pos = (int)route.farms.size(); }
    }

    return best_delta;
}

/*************************** 
Solution graspx(const Instance& inst, int k_rcl)
const Instance& inst: la instancia del problema
int k_rcl: tamano de la lista restringida de candidatos (RCL)
retorna: una solucion construida por GRASP
***************************/
Solution graspx(const Instance& inst, int k_rcl) {
    Solution sol;
    sol.routes.resize(inst.num_trucks);
    for (int k = 0; k < inst.num_trucks; k++) sol.routes[k].truck_idx = k;

    // Cargas actuales por camion (para validar capacidad)
    vector<double> load(inst.num_trucks, 0.0);

    // Granjas pendientes de insertar
    vector<int> remaining(inst.num_farms);
    for (int i = 0; i < inst.num_farms; i++) remaining[i] = i + 1;

    while (!remaining.empty()) {
        // Construir la lista de candidatos
        vector<InsertionCandidate> candidates;

        for (int f : remaining) {
            const Farm& farm = inst.farms[f - 1];
            for (int k = 0; k < inst.num_trucks; k++) {
                // Verificar capacidad
                if (load[k] + farm.production > inst.trucks[k].capacity) continue;

                int pos;
                double delta = find_best_insertion(inst, sol.routes[k], f, pos);
                if (pos < 0) continue;

                // Costo = cuanto aumenta el transporte - cuanto gano en revenue
                double expected_revenue = farm.production * inst.revenue[farm.grade - 1];
                double cost = delta - expected_revenue;

                candidates.push_back({f, k, pos, cost});
            }
        }

        // Si no hay candidatos, salir (situacion extrema: nadie cabe)
        if (candidates.empty()) break;

        // Ordenar por costo ascendente
        sort(candidates.begin(), candidates.end(),
             [](const InsertionCandidate& a, const InsertionCandidate& b) {
                 return a.delta_cost < b.delta_cost;
             });

        // RCL: elegir entre los top-k
        int rcl_size = min(k_rcl, (int)candidates.size());
        int chosen_idx = random_int(0, rcl_size - 1);
        InsertionCandidate chosen = candidates[chosen_idx];

        // Insertar
        Route& r = sol.routes[chosen.truck_idx];
        r.farms.insert(r.farms.begin() + chosen.position, chosen.farm_id);
        load[chosen.truck_idx] += inst.farms[chosen.farm_id - 1].production;

        // Remover de remaining
        remaining.erase(remove(remaining.begin(), remaining.end(), chosen.farm_id), remaining.end());
    }

    return sol;
}

/*************************** 
vector<Solution> build_initial_population(const Instance& inst, int pop_size, int k_rcl)
const Instance& inst: la instancia del problema
int pop_size: cantidad de soluciones a generar
int k_rcl: tamano de la RCL para cada construccion
retorna: un vector de soluciones iniciales
***************************/
vector<Solution> build_initial_population(const Instance& inst, int pop_size, int k_rcl) {
    vector<Solution> pop;
    pop.reserve(pop_size);
    for (int i = 0; i < pop_size; i++) {
        pop.push_back(graspx(inst, k_rcl));
    }
    return pop;
}