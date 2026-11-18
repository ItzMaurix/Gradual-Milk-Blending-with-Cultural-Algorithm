#include "cultural.hpp"
#include "../construction/construction.hpp"
#include "../utils/random.hpp"

#include <algorithm>
#include <iostream>
#include <iomanip>
#include <limits>

using namespace std;

// ===== Utilidades sobre rutas =====

/*************************** 
double route_volume(const Instance& inst, const Route& r)
const Instance& inst: la instancia (para acceder a producciones)
const Route& r: la ruta a medir
retorna: volumen total de leche de la ruta
***************************/
static double route_volume(const Instance& inst, const Route& r) {
    double v = 0.0;
    for (int f : r.farms) v += inst.farms[f - 1].production;
    return v;
}

/*************************** 
double route_avg_sc(const Instance& inst, const Route& r)
const Instance& inst: la instancia (para acceder a SC de cada granja)
const Route& r: la ruta a medir
retorna: promedio ponderado de SC/ml en la ruta, o 0 si esta vacia
***************************/
static double route_avg_sc(const Instance& inst, const Route& r) {
    double vol = 0.0, sc = 0.0;
    for (int f : r.farms) {
        vol += inst.farms[f - 1].production;
        sc += inst.farms[f - 1].production * inst.farms[f - 1].sc_per_ml;
    }
    return (vol > 0.0) ? (sc / vol) : 0.0;
}

/*************************** 
int classify_sc(const Instance& inst, double sc)
const Instance& inst: la instancia (para acceder a los limites sc_upper)
double sc: valor de SC/ml a clasificar
retorna: indice del grado (0=A, 1=B, 2=C)
***************************/
static int classify_sc(const Instance& inst, double sc) {
    if (sc <= inst.sc_upper[0]) return 0;
    if (sc <= inst.sc_upper[1]) return 1;
    return 2;
}

// ===== Crossover (Route-Based Crossover) =====

/*************************** 
Solution route_crossover(const Instance& inst, const Solution& a, const Solution& b)
const Instance& inst: la instancia
const Solution& a: primer padre
const Solution& b: segundo padre
retorna: un hijo que hereda rutas de A y una ruta inyectada de B
***************************/
static Solution route_crossover(const Instance& inst, const Solution& a, const Solution& b) {
    Solution child = a;

    // Elegir una ruta de B que se intente inyectar en A
    int k_b = random_int(0, inst.num_trucks - 1);
    if (b.routes[k_b].farms.empty()) return child;

    // El destino en A es una ruta aleatoria (distinta de k_b si se puede)
    int k_a = random_int(0, inst.num_trucks - 1);

    // Sacar de child todas las granjas que vienen de b.routes[k_b]
    for (int f : b.routes[k_b].farms) {
        for (auto& r : child.routes) {
            r.farms.erase(remove(r.farms.begin(), r.farms.end(), f), r.farms.end());
        }
    }

    // Insertar las granjas de b.routes[k_b] al final de child.routes[k_a]
    for (int f : b.routes[k_b].farms) {
        child.routes[k_a].farms.push_back(f);
    }

    return child;
}

// ===== Influencia del belief sobre mutacion =====

/*************************** 
double influence_weight(const Instance& inst, const Belief& belief, const Route& r, const Farm& farm)
const Instance& inst: la instancia
const Belief& belief: creencias actuales
const Route& r: ruta destino candidata
const Farm& farm: granja que se intenta mover
retorna: peso (0.3 si degrada, 2.0 si consistente con belief SC/ml, w_attract si en archive, 1.0 neutro)
***************************/
static double influence_weight(const Instance& inst, const Belief& belief, const Route& r, const Farm& farm) {
    double vol = route_volume(inst, r);
    double sc = route_avg_sc(inst, r);

    // Si la ruta esta vacia, no hay degradacion posible
    if (vol < 1e-9) {
        // Aun asi, boostear si la granja esta en el archive
        if (belief.archive_farms.count(farm.id)) return belief.w_attract;
        return 1.0;
    }

    double new_vol = vol + farm.production;
    double new_sc = (sc * vol + farm.sc_per_ml * farm.production) / new_vol;
    int new_grade = classify_sc(inst, new_sc);
    int old_grade = classify_sc(inst, sc);

    // Penalizar si degrada la ruta
    if (new_grade > old_grade) return 0.3;

    // Boostear si cae dentro del rango historico del grado
    if (new_grade < 3 &&
        (int)belief.grade_sc_max.size() > new_grade &&
        belief.grade_sc_max[new_grade] > 0.0) {
        if (new_sc <= belief.grade_sc_max[new_grade]) {
            // Combinar con boost del archive si aplica
            double base_w = 2.0;
            if (belief.archive_farms.count(farm.id)) {
                return max(base_w, belief.w_attract);
            }
            return base_w;
        }
    }

    // Boostear si la granja esta en el archive (y no aplica lo anterior)
    if (belief.archive_farms.count(farm.id)) {
        return belief.w_attract;
    }

    return 1.0;
}

// ===== Mutacion =====

/*************************** 
void mutate(const Instance& inst, Solution& s, const Belief& belief, double pm)
const Instance& inst: la instancia
Solution& s: solucion a mutar (se modifica en el lugar)
const Belief& belief: creencias actuales
double pm: probabilidad de aplicar mutacion
retorna: nada
***************************/
static void mutate(const Instance& inst, Solution& s, const Belief& belief, double pm) {
    if (random_double(0.0, 1.0) > pm) return;

    int op = random_int(0, 2);
    if (op == 0) {
        // Swap intra-ruta
        int k = random_int(0, inst.num_trucks - 1);
        auto& farms = s.routes[k].farms;
        if (farms.size() < 2) return;
        int i = random_int(0, (int)farms.size() - 1);
        int j = random_int(0, (int)farms.size() - 1);
        if (i != j) swap(farms[i], farms[j]);
    }
    else if (op == 1) {
        // Mover granja entre rutas con influencia del belief
        int k_src = random_int(0, inst.num_trucks - 1);
        auto& src = s.routes[k_src].farms;
        if (src.empty()) return;
        int idx = random_int(0, (int)src.size() - 1);
        int f = src[idx];
        const Farm& farm = inst.farms[f - 1];

        // Elegir destino ponderado por influencia
        vector<double> weights(inst.num_trucks, 0.0);
        double total_w = 0.0;
        for (int k = 0; k < inst.num_trucks; k++) {
            if (k == k_src) continue;
            double w = influence_weight(inst, belief, s.routes[k], farm);
            weights[k] = w;
            total_w += w;
        }
        if (total_w < 1e-9) return;

        // Ruleta
        double r = random_double(0.0, total_w);
        double acc = 0.0;
        int k_dst = -1;
        for (int k = 0; k < inst.num_trucks; k++) {
            if (k == k_src) continue;
            acc += weights[k];
            if (r <= acc) { k_dst = k; break; }
        }
        if (k_dst < 0) return;

        // Mover al final del destino
        src.erase(src.begin() + idx);
        s.routes[k_dst].farms.push_back(f);
    }
    else {
        // Swap entre rutas
        int k1 = random_int(0, inst.num_trucks - 1);
        int k2 = random_int(0, inst.num_trucks - 1);
        if (k1 == k2) return;
        if (s.routes[k1].farms.empty() || s.routes[k2].farms.empty()) return;
        int i = random_int(0, (int)s.routes[k1].farms.size() - 1);
        int j = random_int(0, (int)s.routes[k2].farms.size() - 1);
        swap(s.routes[k1].farms[i], s.routes[k2].farms[j]);
    }
}

// ===== Seleccion por torneo =====

/*************************** 
int tournament_select(const vector<Metrics>& metrics, int k)
const vector<Metrics>& metrics: metricas de la poblacion
int k: tamano del torneo
retorna: indice del ganador
***************************/
static int tournament_select(const vector<Metrics>& metrics, int k) {
    int best = random_int(0, (int)metrics.size() - 1);
    for (int i = 1; i < k; i++) {
        int cand = random_int(0, (int)metrics.size() - 1);
        if (metrics[cand].profit > metrics[best].profit) best = cand;
    }
    return best;
}

// ===== Actualizar belief =====

/*************************** 
void update_belief(const Instance& inst, Belief& belief, const Solution& sol, const Metrics& m, long long evals_used)
const Instance& inst: la instancia (para calcular SC/ml de las rutas)
Belief& belief: creencias a actualizar
const Solution& sol: solucion candidata
const Metrics& m: metricas de la solucion
long long evals_used: numero de evaluaciones usadas al evaluar esta solucion
retorna: nada
***************************/
static void update_belief(const Instance& inst, Belief& belief, const Solution& sol,
                          const Metrics& m, long long evals_used) {
    // --- Mejora global ---
    if (m.profit > belief.best_metrics.profit) {
        belief.best_sol = sol;
        belief.best_metrics = m;
        belief.best_evals = evals_used;
        belief.evals_since_improve = 0;

        belief.grade_sc_min.assign(3, numeric_limits<double>::max());
        belief.grade_sc_max.assign(3, 0.0);
        vector<bool> seen(3, false);
        for (const Route& r : sol.routes) {
            if (r.farms.empty()) continue;
            double sc = route_avg_sc(inst, r);
            int g = classify_sc(inst, sc);
            if (!seen[g]) {
                belief.grade_sc_min[g] = sc;
                belief.grade_sc_max[g] = sc;
                seen[g] = true;
            } else {
                belief.grade_sc_min[g] = min(belief.grade_sc_min[g], sc);
                belief.grade_sc_max[g] = max(belief.grade_sc_max[g], sc);
            }
        }
        for (int g = 0; g < 3; g++) {
            if (!seen[g]) { belief.grade_sc_min[g] = 0.0; belief.grade_sc_max[g] = 0.0; }
        }
    } else {
        belief.evals_since_improve++;
    }

    // --- Mejora factible ---
    if (m.feasible) {
        if (!belief.has_feasible || m.profit > belief.best_feasible_metrics.profit) {
            belief.best_feasible_sol = sol;
            belief.best_feasible_metrics = m;
            belief.best_feasible_evals = evals_used;
            belief.has_feasible = true;
        }
    }

    // --- Archive de infactibles prometedores ---
    if (belief.archive_size > 0 && !m.feasible) {
        // Insertar si hay espacio o si mejor que el peor del archive
        if ((int)belief.infeasible_archive.size() < belief.archive_size) {
            belief.infeasible_archive.push_back(sol);
            belief.infeasible_archive_metrics.push_back(m);
        } else {
            // Encontrar el peor del archive
            int worst_idx = 0;
            double worst_profit = belief.infeasible_archive_metrics[0].profit;
            for (int i = 1; i < (int)belief.infeasible_archive_metrics.size(); i++) {
                if (belief.infeasible_archive_metrics[i].profit < worst_profit) {
                    worst_profit = belief.infeasible_archive_metrics[i].profit;
                    worst_idx = i;
                }
            }
            if (m.profit > worst_profit) {
                belief.infeasible_archive[worst_idx] = sol;
                belief.infeasible_archive_metrics[worst_idx] = m;
            }
        }

        // Reconstruir archive_farms (set de granjas que aparecen en el archive)
        belief.archive_farms.clear();
        for (const Solution& s : belief.infeasible_archive) {
            for (const Route& r : s.routes) {
                for (int f : r.farms) belief.archive_farms.insert(f);
            }
        }
    }
}

// ===== Loop principal del CA =====

/*************************** 
CAResult run_ca(const Instance& inst, const CAConfig& cfg)
const Instance& inst: la instancia del problema
const CAConfig& cfg: configuracion del algoritmo cultural
retorna: resultado con la mejor solucion factible y la historia de convergencia
***************************/
CAResult run_ca(const Instance& inst, const CAConfig& cfg) {
    set_seed(cfg.seed);

    CAResult result;
    Belief belief;
    belief.archive_size = cfg.archive_size;
    belief.w_attract = cfg.w_attract;

    // Inicializar best_metrics con vectores correctos y profit minimo
    // para que cualquier solucion real (incluso infactible) lo actualice.
    belief.best_metrics.delivered_per_grade.assign(3, 0.0);
    belief.best_metrics.volume_per_truck.assign(inst.num_trucks, 0.0);
    belief.best_metrics.profit = -1e18;
    
    // Validar elite_size (absoluto)
    int elite_size = max(1, min(cfg.elite_size, cfg.pop_size - 1));
    int children_per_gen = cfg.pop_size - elite_size;

    // Inicializar poblacion con GRASP
    vector<Solution> pop = build_initial_population(inst, cfg.pop_size, cfg.k_rcl);
    vector<Metrics> metrics(pop.size());
    long long evals_used = 0;
    for (int i = 0; i < (int)pop.size(); i++) {
        evals_used++;
        metrics[i] = evaluate(inst, pop[i], cfg.Pq, cfg.Pc);
        update_belief(inst, belief, pop[i], metrics[i], evals_used);
    }

    result.history_best.push_back(belief.best_metrics.profit);
    result.history_avg.push_back(0.0);

    // Aviso si cfg.gens no alcanza para max_evals
    if (cfg.max_evals > 0) {
        long long estimado = evals_used + (long long)cfg.gens * children_per_gen;
        if (estimado < cfg.max_evals && cfg.verbose_every > 0) {
            cerr << "  ADVERTENCIA: gens=" << cfg.gens << " solo alcanza ~" << estimado
                 << " evals (pedido max_evals=" << cfg.max_evals << "). Sube gens.\n";
        }
    }

    // Ciclo evolutivo
    for (int gen = 1; gen <= cfg.gens; gen++) {
        // Chequeo de max_evals al tope de cada generacion
        if (cfg.max_evals > 0 && evals_used >= cfg.max_evals) {
            if (cfg.verbose_every > 0)
                cout << "  Max evals (" << cfg.max_evals << ") alcanzado en gen " << (gen - 1) << "\n";
            break;
        }

        // pm dinamico (state-dependent / success-based)
        double pm_current = cfg.pm;
        if (cfg.adaptive_pm) {
            int gens_stalled = belief.evals_since_improve / children_per_gen;
            double progress = min(1.0, (double)gens_stalled / cfg.stall_patience);
            pm_current = cfg.pm_min + progress * (cfg.pm_max - cfg.pm_min);
        }

        // Ordenar por profit para elitismo
        vector<int> order(pop.size());
        for (int i = 0; i < (int)order.size(); i++) order[i] = i;
        sort(order.begin(), order.end(), [&](int x, int y) {
            return metrics[x].profit > metrics[y].profit;
        });

        vector<Solution> next_pop;
        vector<Metrics> next_metrics;
        next_pop.reserve(cfg.pop_size);
        next_metrics.reserve(cfg.pop_size);

        // Elitismo (no re-evalua)
        for (int i = 0; i < elite_size; i++) {
            next_pop.push_back(pop[order[i]]);
            next_metrics.push_back(metrics[order[i]]);
        }

        // Hijos
        while ((int)next_pop.size() < cfg.pop_size) {
            int p1 = tournament_select(metrics, cfg.tour_k);
            int p2 = tournament_select(metrics, cfg.tour_k);
            Solution child;
            if (random_double(0.0, 1.0) < cfg.pc) {
                child = route_crossover(inst, pop[p1], pop[p2]);
            } else {
                child = pop[p1];
            }
            mutate(inst, child, belief, pm_current);

            long long prev_evals = evals_used;
            evals_used++;
            Metrics m = evaluate(inst, child, cfg.Pq, cfg.Pc);
            next_pop.push_back(child);
            next_metrics.push_back(m);
            update_belief(inst, belief, child, m, evals_used);

            // Log cada 1000 evals
            if (cfg.log_every_1000_evals && (evals_used / 1000) > (prev_evals / 1000)) {
                long long milestone = (evals_used / 1000) * 1000;
                double f = belief.has_feasible ? belief.best_feasible_metrics.profit
                                                : belief.best_metrics.profit;
                result.best_every_1000.push_back({milestone, f});
            }
        }

        pop = move(next_pop);
        metrics = move(next_metrics);

        double best_gen = -1e18, sum = 0.0;
        for (auto& m : metrics) {
            if (m.profit > best_gen) best_gen = m.profit;
            sum += m.profit;
        }
        result.history_best.push_back(best_gen);
        result.history_avg.push_back(sum / metrics.size());

        if (cfg.verbose_every > 0 && gen % cfg.verbose_every == 0) {
            int gens_stalled = belief.evals_since_improve / children_per_gen;
            cout << "  gen " << setw(5) << gen
                 << "  evals=" << setw(7) << evals_used
                 << "  pm=" << fixed << setprecision(2) << pm_current
                 << "  best=" << fixed << setprecision(2) << best_gen
                 << "  belief_best=" << belief.best_metrics.profit
                 << "  gens_stalled=" << gens_stalled << "\n";
        }
    }

    result.evals_total = evals_used;
    result.evals_best = belief.best_feasible_evals;
    if (belief.has_feasible) {
        result.best_sol = belief.best_feasible_sol;
        result.best_metrics = belief.best_feasible_metrics;
    } else {
        // Fallback: si no hay factible, reportar mejor global (con warning)
        cerr << "  ADVERTENCIA: no se encontro solucion factible. Reportando mejor global.\n";
        result.best_sol = belief.best_sol;
        result.best_metrics = belief.best_metrics;
    }
    return result;
}