#include "instance/instance.hpp"
#include "solution/solution.hpp"
#include "construction/construction.hpp"
#include "cultural/cultural.hpp"
#include "utils/random.hpp"

#include <iostream>
#include <iomanip>
#include <vector>
#include <string>
#include <exception>
#include <stdexcept>
#include <chrono>

using namespace std;

/*************************** 
Solution build_by_grade(const Instance& inst)
const Instance& inst: la instancia (para saber cuantas granjas y camiones hay)
retorna: una solucion que asigna cada grado de leche a un camion distinto.
         Si num_trucks < 3, se degrada a "todo en el ultimo camion" para no crashear.
***************************/
Solution build_by_grade(const Instance& inst) {
    Solution sol;
    sol.routes.resize(inst.num_trucks);
    for (int k = 0; k < inst.num_trucks; k++) sol.routes[k].truck_idx = k;
    for (int f = 1; f <= inst.num_farms; f++) {
        int g = inst.farms[f - 1].grade;   // 1, 2, 3
        int idx = g - 1;
        if (idx < 0) idx = 0;
        if (idx >= inst.num_trucks) idx = inst.num_trucks - 1;
        sol.routes[idx].farms.push_back(f);
    }
    return sol;
}

/*************************** 
Solution build_greedy_capacity(const Instance& inst)
const Instance& inst: la instancia
retorna: una solucion que llena cada camion en orden de id hasta capacidad
***************************/
Solution build_greedy_capacity(const Instance& inst) {
    Solution sol;
    sol.routes.resize(inst.num_trucks);
    for (int k = 0; k < inst.num_trucks; k++) sol.routes[k].truck_idx = k;

    int k = 0;
    double load = 0.0;
    for (int f = 1; f <= inst.num_farms; f++) {
        double p = inst.farms[f - 1].production;
        if (k < inst.num_trucks && load + p <= inst.trucks[k].capacity) {
            sol.routes[k].farms.push_back(f);
            load += p;
        } else {
            k++;
            load = 0.0;
            if (k >= inst.num_trucks) {
                sol.routes[inst.num_trucks - 1].farms.push_back(f);
            } else {
                sol.routes[k].farms.push_back(f);
                load = p;
            }
        }
    }
    return sol;
}

/*************************** 
void print_metrics(const string& name, const Solution& sol, const Metrics& m, const Instance& inst)
const string& name: nombre de la solucion (para mostrar)
const Solution& sol: la solucion (para contar rutas y granjas)
const Metrics& m: las metricas calculadas
const Instance& inst: la instancia (para capacidades)
retorna: nada, solo imprime un bloque con el resumen
***************************/
void print_metrics(const string& name, const Solution& sol, const Metrics& m, const Instance& inst) {
    cout << "\n=== " << name << " (" << inst.num_trucks << " camiones) ===\n";
    cout << "  Revenue:          " << fixed << setprecision(2) << m.revenue << "\n";
    cout << "  Transport cost:   " << m.transport_cost << "\n";
    cout << "  Penalty (cap):    " << m.penalty_capacity << "\n";
    cout << "  Penalty (quota):  " << m.penalty_quota << "\n";
    cout << "  Profit:           " << m.profit << "\n";
    cout << "  Feasible:         " << (m.feasible ? "SI" : "NO") << "\n";

    if ((int)m.delivered_per_grade.size() >= 3) {
        cout << "  Litros entregados por grado: A=" << m.delivered_per_grade[0]
             << "  B=" << m.delivered_per_grade[1]
             << "  C=" << m.delivered_per_grade[2] << "\n";
    } else {
        cout << "  Litros entregados por grado: (sin datos)\n";
    }

    if ((int)m.volume_per_truck.size() >= inst.num_trucks) {
        cout << "  Litros por camion: ";
        for (int k = 0; k < inst.num_trucks; k++) {
            cout << "k" << k << "=" << m.volume_per_truck[k]
                 << "/" << inst.trucks[k].capacity << "  ";
        }
        cout << "\n";
    }

    cout << "  Rutas:\n";
    for (int k = 0; k < (int)sol.routes.size(); k++) {
        const Route& r = sol.routes[k];
        if (r.farms.empty()) { cout << "    camion " << k << ": (vacio)\n"; continue; }
        cout << "    camion " << k << " [" << r.farms.size() << " granjas]: ";
        for (int f : r.farms) cout << f << " ";
        cout << "\n";
    }
}

int main(int argc, char** argv) {
    if (argc < 2) {
        cerr << "Uso: " << argv[0] << " <instancia> [opciones]\n"
             << "  --quota A B C          Cuota minima por grado (default 0 0 0)\n"
             << "  --Pq val               Penalizacion cuota (default 10)\n"
             << "  --Pc val               Penalizacion capacidad (default 10)\n"
             << "  --pop n                Tamano poblacion (default 30)\n"
             << "  --gens n               Cap de generaciones (default 100)\n"
             << "  --elite-size n         Elite absoluto (default 3)\n"
             << "  --pc val               Prob crossover (default 0.8)\n"
             << "  --pm val               Prob mutacion (default 0.3)\n"
             << "  --tour n               Tamano torneo (default 3)\n"
             << "  --k_rcl n              RCL de GRASP (default 3)\n"
             << "  --seed n               Semilla (default 42)\n"
             << "  --max-evals n          Criterio de termino por evaluaciones (0=off)\n"
             << "  --log-every-1000-evals Registrar (eval, mejor_f) cada 1000\n"
             << "  --verbose-every n      Log cada n generaciones (default 10)\n"
             << "  --adaptive-pm          Activar pm adaptativo (default off)\n"
             << "  --pm-min val           pm minimo (default 0.1)\n"
             << "  --pm-max val           pm maximo (default 0.5)\n"
             << "  --stall-patience n     Generaciones para llegar a pm_max (default 20)\n"
             << "  --stall-patience n     Generaciones para llegar a pm_max (default 20)\n"
             << "  --archive-size n       Tamano del archive de infactibles (default 0=off)\n"
             << "  --w-attract val        Peso de atraccion para granjas del archive (default 1.5)\n";
        return 1;
    }

    string instance_path;
    vector<double> quota_override;
    double Pq = 10.0, Pc = 10.0;
    int pop_size = 31, gens = 100, elite_size = 7;
    double pc = 0.8, pm = 0.054;
    int tour_k = 5, k_rcl = 2;
    unsigned int seed = 42;
    long long max_evals = 0;
    bool log_every_1000 = false;
    int verbose_every = 10;
    bool adaptive_pm = false;
    double pm_min = 0.1, pm_max = 0.5;
    int stall_patience = 20;
    int archive_size = 3;
    double w_attract = 2.10;

    for (int i = 1; i < argc; i++) {
        string arg = argv[i];
        if (arg == "--quota" && i + 3 < argc) {
            quota_override = { stod(argv[i+1]), stod(argv[i+2]), stod(argv[i+3]) };
            i += 3;
        } else if (arg == "--Pq" && i + 1 < argc)       { Pq = stod(argv[++i]); }
        else if (arg == "--Pc" && i + 1 < argc)         { Pc = stod(argv[++i]); }
        else if (arg == "--pop" && i + 1 < argc)        { pop_size = stoi(argv[++i]); }
        else if (arg == "--gens" && i + 1 < argc)       { gens = stoi(argv[++i]); }
        else if (arg == "--elite-size" && i + 1 < argc) { elite_size = stoi(argv[++i]); }
        else if (arg == "--pc" && i + 1 < argc)         { pc = stod(argv[++i]); }
        else if (arg == "--pm" && i + 1 < argc)         { pm = stod(argv[++i]); }
        else if (arg == "--tour" && i + 1 < argc)       { tour_k = stoi(argv[++i]); }
        else if (arg == "--k_rcl" && i + 1 < argc)      { k_rcl = stoi(argv[++i]); }
        else if (arg == "--seed" && i + 1 < argc)       { seed = stoul(argv[++i]); }
        else if (arg == "--max-evals" && i + 1 < argc)  { max_evals = stoll(argv[++i]); }
        else if (arg == "--log-every-1000-evals")       { log_every_1000 = true; }
        else if (arg == "--verbose-every" && i + 1 < argc) { verbose_every = stoi(argv[++i]); }
        else if (arg == "--adaptive-pm")               { adaptive_pm = true; }
        else if (arg == "--pm-min" && i + 1 < argc)    { pm_min = stod(argv[++i]); }
        else if (arg == "--pm-max" && i + 1 < argc)    { pm_max = stod(argv[++i]); }
        else if (arg == "--stall-patience" && i + 1 < argc) { stall_patience = stoi(argv[++i]); }
        else if (arg == "--archive-size" && i + 1 < argc) { archive_size = stoi(argv[++i]); }
        else if (arg == "--w-attract" && i + 1 < argc)    { w_attract = stod(argv[++i]); }
        else { instance_path = arg; }
    }

    try {
        Instance inst = parse_instance(instance_path);
        if (!quota_override.empty()) inst.quota = quota_override;

        cout << "=== Instancia: " << instance_path << " ===\n";
        cout << "Granjas=" << inst.num_farms
             << "  Camiones=" << inst.num_trucks
             << "  Cuota=[" << inst.quota[0] << ", " << inst.quota[1] << ", " << inst.quota[2] << "]\n";

        CAConfig cfg;
        cfg.pop_size = pop_size;
        cfg.gens = gens;
        cfg.elite_size = elite_size;
        cfg.pc = pc;
        cfg.pm = pm;
        cfg.tour_k = tour_k;
        cfg.k_rcl = k_rcl;
        cfg.Pq = Pq;
        cfg.Pc = Pc;
        cfg.seed = seed;
        cfg.max_evals = max_evals;
        cfg.log_every_1000_evals = log_every_1000;
        cfg.verbose_every = verbose_every;
        cfg.adaptive_pm = adaptive_pm;
        cfg.pm_min = pm_min;
        cfg.pm_max = pm_max;
        cfg.stall_patience = stall_patience;
        cfg.archive_size = archive_size;
        cfg.w_attract = w_attract;
        cfg.verbose_every = 100;

        if (cfg.adaptive_pm && cfg.pm_min >= cfg.pm_max) {
            cerr << "ERROR: pm_min (" << cfg.pm_min
                 << ") debe ser < pm_max (" << cfg.pm_max << ")\n";
            return 1;
        }

        cout << "\n=== CONFIGURACION CA ===\n";
        cout << "  pop=" << cfg.pop_size
             << "  elite=" << cfg.elite_size
             << "  gens_cap=" << cfg.gens
             << "  pc=" << cfg.pc;
        if (cfg.adaptive_pm) {
            cout << "  pm=adaptive[" << cfg.pm_min << "," << cfg.pm_max
                 << "] patience=" << cfg.stall_patience;
        } else {
            cout << "  pm=" << cfg.pm;
        }
        cout << "  tour=" << cfg.tour_k << "  k_rcl=" << cfg.k_rcl
             << "  max_evals=" << cfg.max_evals << "\n";

        if (cfg.archive_size > 0) {
            cout << "  archive_size=" << cfg.archive_size
                 << "  w_attract=" << cfg.w_attract;
        }
        cout << "\n";

        auto t0 = chrono::steady_clock::now();
        cout << "\n=== EJECUTANDO CA ===\n";

        if (inst.num_trucks < 2) {
            cerr << "ADVERTENCIA: instancia con " << inst.num_trucks
                 << " camiones. El CA pierde diversidad por operadores que asumen ≥2 camiones.\n";
        }

        CAResult res = run_ca(inst, cfg);
        auto t1 = chrono::steady_clock::now();
        double elapsed = chrono::duration<double>(t1 - t0).count();

        print_metrics("MEJOR SOLUCION FACTIBLE", res.best_sol, res.best_metrics, inst);

        cout << "\n=== EVALUACIONES ===\n";
        cout << "  evals_total: " << res.evals_total << "\n";
        cout << "  evals_best:  " << res.evals_best << "\n";
        cout << "  tiempo:      " << fixed << setprecision(3) << elapsed << " s\n";

        if (!res.best_every_1000.empty()) {
            cout << "\n=== PROGRESO CADA 1000 EVALS ===\n";
            cout << "  eval   | mejor_f\n";
            cout << "  -------|--------\n";
            for (auto& p : res.best_every_1000) {
                cout << "  " << setw(6) << p.first << " | " << fixed << setprecision(2) << p.second << "\n";
            }
        }

        // Baselines
        cout << "\n=== BASELINES ===\n";
        cout << fixed << setprecision(2);  // reset precision
        Solution bg = build_by_grade(inst);
        Metrics m_bg = evaluate(inst, bg, Pq, Pc);
        cout << "  by_grade:  profit=" << m_bg.profit << "  feasible=" << (m_bg.feasible ? "SI" : "NO") << "\n";
        Solution gr = build_greedy_capacity(inst);
        Metrics m_gr = evaluate(inst, gr, Pq, Pc);
        cout << "  greedy:    profit=" << m_gr.profit << "  feasible=" << (m_gr.feasible ? "SI" : "NO") << "\n";

        // RESULT
        cout << "RESULT," << fixed << setprecision(2)
             << res.best_metrics.profit << ","
             << res.best_metrics.revenue << ","
             << res.best_metrics.transport_cost << ","
             << (res.best_metrics.feasible ? "1" : "0") << ","
             << res.evals_total << ","
             << res.evals_best << ","
             << setprecision(3) << elapsed << "\n";

        // BASELINE line
        cout << "BASELINE,"
             << fixed << setprecision(2)
             << m_bg.profit << "," << (m_bg.feasible ? "1" : "0") << ","
             << m_gr.profit << "," << (m_gr.feasible ? "1" : "0") << "\n";

    } catch (const exception& e) {
        cerr << "Error: " << e.what() << "\n";
        return 1;
    }
    return 0;
}