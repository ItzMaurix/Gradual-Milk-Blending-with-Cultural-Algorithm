#include "instance.hpp"

#include <iostream>
#include <fstream>
#include <sstream>
#include <stdexcept>
#include <cmath>

using namespace std;

/*************************** 
Instance parse_instance(const string& filepath)
const string& filepath: ruta al archivo de instancia
retorna: una instancia con todos los datos del archivo organizados
***************************/
Instance parse_instance(const string& filepath) {
    ifstream file(filepath);
    if (!file.is_open()) {
        throw runtime_error("No se pudo abrir el archivo: " + filepath);
    }

    Instance inst;

    // Datos temporales que se van llenando al leer el archivo
    vector<int> nodes;
    vector<int> farms;
    vector<int> trucks;
    map<int, double> q_map;
    vector<double> d_vec;
    vector<double> v_vec;
    vector<double> u_vec;

    struct QuRow { int id; double prod; int grade; double sc; };
    vector<QuRow> qu_rows;

    vector<int> c_cols;
    map<int, vector<double>> c_rows;

    // Estado del parser: indica en qué sección del archivo estamos
    enum State {
        NONE, SET_NODOS, SET_I, SET_K,
        PARAM_Q, PARAM_D, PARAM_V, PARAM_U, PARAM_QU, PARAM_C_HDR, PARAM_C_ROWS
    } state = NONE;

    // Función auxiliar: devuelve el texto que sigue a un separador
    auto extract_after = [](const string& line, const string& sep) {
        size_t pos = line.find(sep);
        if (pos == string::npos) return string("");
        size_t start = pos + sep.size();
        while (start < line.size() && (line[start] == ' ' || line[start] == '\t')) start++;
        return line.substr(start);
    };

    string line;
    while (getline(file, line)) {
        // Quitar comentarios: todo lo que sigue a '#'
        size_t hash = line.find('#');
        if (hash != string::npos) line = line.substr(0, hash);

        // Quitar espacios y saltos al inicio y al final
        size_t s = line.find_first_not_of(" \t\r\n");
        if (s == string::npos) continue;
        size_t e = line.find_last_not_of(" \t\r\n");
        line = line.substr(s, e - s + 1);
        if (line.empty()) continue;

        // Detectar si la línea cierra una sección con ';'
        bool end_section = false;
        if (line.back() == ';') {
            line.pop_back();
            end_section = true;
            s = line.find_first_not_of(" \t\r\n");
            if (s == string::npos) { state = NONE; continue; }
            e = line.find_last_not_of(" \t\r\n");
            line = line.substr(s, e - s + 1);
        }

        // Detectar inicio de sección
        if (line.find("set Nodos") != string::npos) {
            state = SET_NODOS;
            line = extract_after(line, ":=");
            if (line.empty()) continue;
        }
        else if (line.find("set I") != string::npos) {
            state = SET_I;
            line = extract_after(line, ":=");
            if (line.empty()) continue;
        }
        else if (line.find("set K") != string::npos) {
            state = SET_K;
            line = extract_after(line, ":=");
            if (line.empty()) continue;
        }
        else if (line.find("param Q") != string::npos) {
            state = PARAM_Q;
            line = extract_after(line, ":=");
            if (line.empty()) continue;
        }
        else if (line.find("param D") != string::npos) {
            state = PARAM_D;
            line = extract_after(line, ":=");
            if (line.empty()) continue;
        }
        else if (line.find("param V") != string::npos) {
            state = PARAM_V;
            line = extract_after(line, ":=");
            if (line.empty()) continue;
        }
        else if (line.find("param U") != string::npos) {
            state = PARAM_U;
            line = extract_after(line, ":=");
            if (line.empty()) continue;
        }
        else if (line.find("param qu") != string::npos) {
            state = PARAM_QU;
            line = extract_after(line, ":=");
            if (line.empty()) continue;
        }
        else if (line.find("param c") != string::npos) {
            state = PARAM_C_HDR;
            continue;
        }

        // Parsear los valores según la sección actual
        istringstream ss(line);
        switch (state) {
            case SET_NODOS: {
                int v;
                while (ss >> v) nodes.push_back(v);
                break;
            }
            case SET_I: {
                int v;
                while (ss >> v) farms.push_back(v);
                break;
            }
            case SET_K: {
                int v;
                while (ss >> v) trucks.push_back(v);
                break;
            }
            case PARAM_Q: {
                int id; double cap;
                while (ss >> id >> cap) q_map[id] = cap;
                break;
            }
            case PARAM_D: {
                double v;
                while (ss >> v) d_vec.push_back(v);
                break;
            }
            case PARAM_V: {
                double v;
                while (ss >> v) v_vec.push_back(v);
                break;
            }
            case PARAM_U: {
                double v;
                while (ss >> v) u_vec.push_back(v);
                break;
            }
            case PARAM_QU: {
                int id; double prod;
                if (!(ss >> id >> prod)) break;
                QuRow row{id, prod, 0, 0.0};
                if (id != inst.depot_id) {
                    int grade; double sc;
                    ss >> grade >> sc;
                    row.grade = grade;
                    row.sc = sc;
                }
                qu_rows.push_back(row);
                break;
            }
            case PARAM_C_HDR: {
                // El header de la matriz tiene la forma ': id1 id2 ... :='
                size_t colon = line.find(':');
                size_t col_eq = line.find(":=");
                if (colon == string::npos || col_eq == string::npos) {
                    throw runtime_error("Formato invalido en param c: " + line);
                }
                string ids_str = line.substr(colon + 1, col_eq - colon - 1);
                istringstream ss_ids(ids_str);
                int id;
                while (ss_ids >> id) c_cols.push_back(id);
                state = PARAM_C_ROWS;
                break;
            }
            case PARAM_C_ROWS: {
                int row_id;
                ss >> row_id;
                vector<double> row_vals;
                double v;
                while (ss >> v) row_vals.push_back(v);
                c_rows[row_id] = row_vals;
                break;
            }
            default:
                break;
        }

        if (end_section) state = NONE;
    }

    // Llenar la instancia con los datos recolectados
    inst.num_farms = (int)farms.size();
    inst.num_trucks = (int)trucks.size();
    inst.num_grades = 3;

    // Granjas
    inst.farms.resize(inst.num_farms);
    for (auto& row : qu_rows) {
        if (row.id == inst.depot_id) continue;
        int idx = row.id - 1;
        if (idx < 0 || idx >= inst.num_farms) {
            throw runtime_error("ID de granja fuera de rango: " + to_string(row.id));
        }
        inst.farms[idx].id = row.id;
        inst.farms[idx].production = row.prod;
        inst.farms[idx].grade = row.grade;
        inst.farms[idx].sc_per_ml = row.sc;
    }

    // Camiones
    inst.trucks.resize(inst.num_trucks);
    for (int i = 0; i < inst.num_trucks; i++) {
        inst.trucks[i].id = trucks[i];
        inst.trucks[i].capacity = q_map[trucks[i]];
    }

    // Vectores de tamaño 3
    inst.revenue = v_vec;
    inst.sc_upper = u_vec;
    inst.quota = vector<double>(3, 0.0);  // sin cuota por defecto

    // Validar tamaños
    if ((int)inst.revenue.size() != 3) throw runtime_error("V debe tener 3 valores");
    if ((int)inst.sc_upper.size() != 3) throw runtime_error("U debe tener 3 valores");

    // Matriz de costos (n+1) x (n+1), índice 0 = depósito
    inst.cost.assign(inst.num_farms + 1, vector<double>(inst.num_farms + 1, 0.0));
    auto to_internal = [&](int id) -> int {
        return (id == inst.depot_id) ? 0 : id;
    };
    for (auto& kv : c_rows) {
        int i = to_internal(kv.first);
        for (size_t j = 0; j < c_cols.size(); j++) {
            int jj = to_internal(c_cols[j]);
            inst.cost[i][jj] = kv.second[j];
        }
    }

    // Validación opcional: D debe coincidir con la suma de producciones
    vector<double> computed(3, 0.0);
    for (auto& f : inst.farms) computed[f.grade - 1] += f.production;
    for (int r = 0; r < 3 && r < (int)d_vec.size(); r++) {
        if (abs(d_vec[r] - computed[r]) > 1.0) {
            cerr << "Advertencia: D[" << r << "]=" << d_vec[r]
                 << " no coincide con suma de producciones=" << computed[r] << endl;
        }
    }

    return inst;
}

/*************************** 
void print_instance(const Instance& inst)
const Instance& inst: la instancia que se quiere imprimir por pantalla
retorna: nada, solo imprime un resumen en pantalla
***************************/
void print_instance(const Instance& inst) {
    cout << "=== Resumen de la instancia ===\n";
    cout << "Granjas: " << inst.num_farms << "\n";
    cout << "Camiones: " << inst.num_trucks << "\n";
    cout << "Tipos de leche: " << inst.num_grades << "\n\n";

    cout << "--- Camiones ---\n";
    for (auto& t : inst.trucks) {
        cout << "  Camion " << t.id << ": capacidad = " << t.capacity << " L\n";
    }

    cout << "\n--- Precios por litro ---\n";
    cout << "  A=" << inst.revenue[0]
         << "  B=" << inst.revenue[1]
         << "  C=" << inst.revenue[2] << "\n";

    cout << "\n--- Limites de celulas somaticas ---\n";
    cout << "  A: hasta " << inst.sc_upper[0] << "\n";
    cout << "  B: hasta " << inst.sc_upper[1] << "\n";
    cout << "  C: hasta " << inst.sc_upper[2] << "\n";

    cout << "\n--- Cuota ---\n";
    cout << "  A=" << inst.quota[0]
         << "  B=" << inst.quota[1]
         << "  C=" << inst.quota[2] << "\n";

    cout << "\n--- Primeras 5 granjas ---\n";
    for (int i = 0; i < 5 && i < inst.num_farms; i++) {
        auto& f = inst.farms[i];
        cout << "  Granja " << f.id
             << ": prod=" << f.production
             << " grado=" << f.grade
             << " sc/ml=" << f.sc_per_ml << "\n";
    }

    cout << "\n--- Matriz de costos (fila 0 = deposito) ---\n";
    for (int i = 0; i < inst.num_farms + 1; i++) {
        cout << "  [";
        for (int j = 0; j < inst.num_farms + 1; j++) {
            cout << inst.cost[i][j];
            if (j < inst.num_farms) cout << ", ";
        }
        cout << "]\n";
    }
    cout << "=== Fin del resumen ===\n";
}