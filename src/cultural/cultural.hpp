#pragma once

#include "../instance/instance.hpp"
#include "../solution/solution.hpp"
#include <vector>
#include <set>
#include <utility>

using namespace std;

/*************************** 
struct CAConfig
int pop_size: tamano de la poblacion
int gens: cap maximo de generaciones. Si max_evals>0, termina por evals
double pc: probabilidad de crossover
double pm: pm base (usado solo si adaptive_pm=false)
int elite_size: numero absoluto de elites que pasan directo
int tour_k: tamano del torneo de seleccion
int k_rcl: tamano de la RCL de GRASP
double Pq: peso de penalizacion por cuota
double Pc: peso de penalizacion por capacidad
unsigned int seed: semilla
int verbose_every: cada cuantas generaciones imprimir progreso
long long max_evals: si >0, termina cuando evals_used >= max_evals
bool log_every_1000_evals: si true, registra (eval, mejor_f) cada 1000 evals
bool adaptive_pm: si true, pm varia segun gens_stalled
double pm_min: pm minimo (explotar)
double pm_max: pm maximo (explorar)
int stall_patience: generaciones sin mejora antes de llegar a pm_max
int archive_size: capacidad del archive de infactibles (0 = desactivado)
double w_attract: peso de atraccion para granjas del archive
***************************/
struct CAConfig {
    int pop_size = 31;
    int gens = 100;
    double pc = 0.8;
    double pm = 0.054;
    int elite_size = 7;
    int tour_k = 5;
    int k_rcl = 2;
    double Pq = 10.0;
    double Pc = 10.0;
    unsigned int seed = 42;
    int verbose_every = 10;
    long long max_evals = 0;
    bool log_every_1000_evals = false;
    bool adaptive_pm = false;
    double pm_min = 0.1;
    double pm_max = 0.5;
    int stall_patience = 20;
    int archive_size = 3;
    double w_attract = 2.10;
};

/*************************** 
struct Belief
Solution best_sol: mejor solucion global (usada para influence)
Metrics best_metrics: metricas de best_sol
vector<double> grade_sc_min: min SC/ml por grado
vector<double> grade_sc_max: max SC/ml por grado
int evals_since_improve: evals sin mejora del best global
long long best_evals: evals usadas al encontrar best_sol
Solution best_feasible_sol: mejor solucion factible encontrada
Metrics best_feasible_metrics: metricas de best_feasible_sol
long long best_feasible_evals: evals al encontrar best_feasible
bool has_feasible: true si ya hay al menos una solucion factible
vector<Solution> infeasible_archive: top-N soluciones infactibles por profit
vector<Metrics> infeasible_archive_metrics: metricas de cada solucion del archive
set<int> archive_farms: union de todas las granjas en el archive (lookup rapido)
int archive_size: capacidad maxima del archive
***************************/
struct Belief {
    Solution best_sol;
    Metrics best_metrics;
    vector<double> grade_sc_min;
    vector<double> grade_sc_max;
    int evals_since_improve = 0;
    long long best_evals = 0;
    Solution best_feasible_sol;
    Metrics best_feasible_metrics;
    long long best_feasible_evals = 0;
    bool has_feasible = false;
    vector<Solution> infeasible_archive;
    vector<Metrics> infeasible_archive_metrics;
    set<int> archive_farms;
    int archive_size = 0;
    double w_attract = 1.5;
};

/*************************** 
struct CAResult
Solution best_sol: mejor solucion FACTIBLE (protocolo)
Metrics best_metrics: metricas de best_sol
vector<double> history_best: profit global por generacion
vector<double> history_avg: profit promedio por generacion
long long evals_total: evaluaciones totales usadas
long long evals_best: evaluaciones al encontrar mejor factible
vector<pair<long long,double>> best_every_1000: (eval, mejor factible) cada 1000
***************************/
struct CAResult {
    Solution best_sol;
    Metrics best_metrics;
    vector<double> history_best;
    vector<double> history_avg;
    long long evals_total = 0;
    long long evals_best = 0;
    vector<pair<long long, double>> best_every_1000;
};

/*************************** 
CAResult run_ca(const Instance& inst, const CAConfig& cfg)
const Instance& inst: la instancia del problema
const CAConfig& cfg: configuracion del algoritmo cultural
retorna: resultado con la mejor solucion factible y la historia
***************************/
CAResult run_ca(const Instance& inst, const CAConfig& cfg);