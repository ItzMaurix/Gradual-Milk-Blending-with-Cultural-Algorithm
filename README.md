# GMB — Gradual Milk Blending

Implementación en C++ del problema de recolección de leche con mezcla gradual (Gradual Milk Blending, GMB), resuelto mediante un Algoritmo Cultural (CA) con inicialización GRASP. Incluye sintonización automática de hiperparámetros con irace. El proyecto es parte del curso INF388 de la Universidad Técnica Federico Santa María.

---

## Tabla de contenidos

1. [Descripción del problema](#1-descripción-del-problema)
2. [Requisitos del sistema](#2-requisitos-del-sistema)
3. [Compilación](#3-compilación)
4. [Ejecución básica](#4-ejecución-básica)
5. [Parámetros de línea de comandos](#5-parámetros-de-línea-de-comandos)
6. [Estructura del proyecto](#6-estructura-del-proyecto)
7. [Descripción de módulos](#7-descripción-de-módulos)
8. [Formato de salida](#8-formato-de-salida)
9. [Scripts auxiliares](#9-scripts-auxiliares)
10. [Sintonización con irace](#10-sintonización-con-irace)
11. [Ejemplo de sesión](#11-ejemplo-de-sesión)
12. [Hardware y reproducibilidad](#12-hardware-y-reproducibilidad)

---

## 1. Descripción del problema

Un conjunto de granjas produce leche de tres calidades (A, B y C), determinadas por el conteo de células somáticas por mililitro (SC/ml). Cada granja produce un volumen conocido y de una única calidad. Toda la leche debe ser recolectada y transportada a una planta procesadora mediante una flota heterogénea de camiones.

El problema consiste en diseñar rutas para los camiones de forma que se maximice el profit, definido como:

~~~
profit = revenue (leche entregada) − costo de transporte
~~~

La particularidad del GMB es la **regla de mezcla gradual**: cuando un camión recolecta leche de varias granjas, el blend se clasifica según el SC/ml ponderado por volumen, no según el peor componente. Esto permite diluir pequeñas cantidades de leche de baja calidad dentro de cargas de mayor calidad sin degradar toda la carga, aumentando el revenue. Reglas anteriores (mezcla discreta) forzaban que un solo litro de calidad C degradara todo el camión a C.

**Trabajo relacionado:** esta implementación sigue el modelo y problema introducidos por Paredes-Belmar et al. (2022) en *"Vehicle routing for milk collection with gradual blending: a case arising in Chile"* (European Journal of Operational Research, 303(3), 1403–1416). Los métodos de inicialización (GRASP), de búsqueda (Algoritmo Cultural) y la sintonización con irace son aportes de este proyecto.

---

## 2. Requisitos del sistema

El código está diseñado para compilarse y ejecutarse en **Linux (Ubuntu 20.04 o similar)**.

| Requisito | Versión mínima | Verificación |
|-----------|----------------|--------------|
| Sistema operativo | Linux (Ubuntu 20.04+) | `uname -a` |
| Compilador C++ | `g++` ≥ 9 (soporte C++17) | `g++ --version` |
| GNU Make | 4.0+ | `make --version` |
| Herramientas de shell | `bash`, `awk`, `column` | `which awk column` |

Opcional para análisis y gráficas (no necesario para ejecutar el algoritmo):
- Python 3.8+ con `matplotlib`, `scipy`, `numpy`.
- R 4.0+ con el paquete `irace` (solo para reproducir la sintonización).

No hay dependencias externas de C++ (sin Boost, sin bibliotecas de terceros). Todo el código usa la biblioteca estándar.

---

## 3. Compilación

Desde la raíz del proyecto:

~~~bash
make
~~~

Esto compila todos los `.cpp` en `src/` y genera el ejecutable `./gmb`.

**Verificar que se compiló correctamente:**

~~~bash
./gmb
~~~

Debe imprimir el mensaje de uso con todos los flags disponibles. **Debe compilar con 0 warnings** (`-Wall -Wextra` activados).

**Limpiar artefactos de compilación:**

~~~bash
make clean
~~~

**Recompilar desde cero:**

~~~bash
make clean && make
~~~

---

## 4. Ejecución básica

**Ejemplo mínimo** (usa todos los parámetros por defecto):

~~~bash
./gmb data/instances/dist2/instanciaeil22-DB-2distribution.txt
~~~

Con los defaults actuales (`pop=31, elite=7, pm=0.054, tour=5, k_rcl=2, archive=3, w_attract=2.10`), el CA se ejecuta en < 1 segundo para `eil22`.

**Ejemplo con presupuesto de evaluaciones** (recomendado para experimentos):

~~~bash
./gmb data/instances/dist2/instanciaeil22-DB-2distribution.txt \
    --pop 31 --elite-size 7 --pm 0.054 --tour 5 --k_rcl 2 \
    --archive-size 3 --w-attract 2.10 \
    --gens 100000 --max-evals 1000000 --seed 1
~~~

**Forzando una cuota mínima por grado:**

Si no se especifica `--quota`, la cuota por defecto es `[0, 0, 0]` (no se impone mínimo). Para imponerla:

~~~bash
./gmb data/instances/dist2/instanciaeil30-DB-2distribution.txt \
     --quota 5000 4000 1000
~~~

**Ajustando penalizaciones:**

~~~bash
./gmb data/instances/dist2/instanciaeil22-DB-2distribution.txt --Pq 20 --Pc 20
~~~

Valores altos de `Pq`/`Pc` fuerzan al algoritmo a priorizar factibilidad. Valores bajos permiten explorar soluciones infactibles más agresivamente.

---

## 5. Parámetros de línea de comandos

**Todos los parámetros se pasan por línea de ejecución.** No hay archivos de configuración. Los valores predeterminados corresponden a la configuración óptima encontrada por irace (config ID 406).

### Parámetros obligatorios

| Parámetro | Tipo | Descripción |
|-----------|------|-------------|
| `<instancia>` | `string` | Ruta al archivo de instancia (formato AMPL-like, extensión `.txt` o `.mcgb`) |

### Parámetros de la instancia

| Parámetro | Tipo | Default | Descripción |
|-----------|------|---------|-------------|
| `--quota A B C` | `3 × double` | `0 0 0` | Cuota mínima a entregar de cada grado (A, B, C). Si vale `0 0 0`, no se impone cuota. |
| `--Pq val` | `double` | `10.0` | Penalización por litro faltante de cuota. |
| `--Pc val` | `double` | `10.0` | Penalización por litro excedido sobre la capacidad de un camión. |

### Parámetros del CA (config 406 de irace)

| Parámetro | Tipo | Default | Descripción |
|-----------|------|---------|-------------|
| `--pop n` | `int` | `31` | Tamaño de la población del Algoritmo Cultural. |
| `--elite-size n` | `int` | `7` | Número de élites que pasan directo a la siguiente generación. |
| `--pc val` | `double` | `0.8` | Probabilidad de crossover por hijo. |
| `--pm val` | `double` | `0.054` | Probabilidad de mutación por hijo. |
| `--tour n` | `int` | `5` | Tamaño del torneo de selección. |
| `--k_rcl n` | `int` | `2` | Tamaño de la lista restringida de candidatos (RCL) en GRASP. |
| `--archive-size n` | `int` | `3` | Capacidad del archive de soluciones infactibles prometedoras. `0` desactiva el archive. |
| `--w-attract val` | `double` | `2.10` | Peso de atracción para granjas presentes en el archive. |
| `--seed n` | `unsigned int` | `42` | Semilla del generador aleatorio. Determinista. |

### Criterio de término

| Parámetro | Tipo | Default | Descripción |
|-----------|------|---------|-------------|
| `--gens n` | `int` | `100` | Cap máximo de generaciones. |
| `--max-evals n` | `long long` | `0` | Si `> 0`, termina cuando se alcanza este número de evaluaciones. Si `0`, solo se usa `--gens` como criterio. |

**Nota:** con `--max-evals 1000000` y `--pop 31 --elite-size 7`, el CA ejecuta ~41,600 generaciones antes de llegar al presupuesto. El `--gens` se mantiene como safety cap (usar `--gens 100000`).

### Debug / logging

| Parámetro | Tipo | Default | Descripción |
|-----------|------|---------|-------------|
| `--verbose-every n` | `int` | `10` | Log de progreso cada `n` generaciones. |
| `--log-every-1000-evals` | flag | off | Registra `(eval, mejor_f)` cada 1000 evaluaciones (para curvas de convergencia). |

### Parámetros de `pm` adaptativo (descartado empíricamente, se mantienen por completitud)

El análisis con irace mostró que `pm` adaptativo **no aporta mejoras significativas** (fue descartado en las 5 élites finales). Se mantienen los flags por completitud:

| Parámetro | Tipo | Default | Descripción |
|-----------|------|---------|-------------|
| `--adaptive-pm` | flag | off | Activa `pm` adaptativo según `gens_stalled`. |
| `--pm-min val` | `double` | `0.1` | `pm` mínimo (explotar). |
| `--pm-max val` | `double` | `0.5` | `pm` máximo (explorar). |
| `--stall-patience n` | `int` | `20` | Generaciones sin mejora para llegar a `pm_max`. |

---

## 6. Estructura del proyecto

~~~
Proyecto GMB/
├── Makefile                    # reglas de compilación
├── README.md                   # este archivo
├── data/
│   └── instances/              # instancias de prueba en formato AMPL-like
│       ├── dist1/              # 47 instancias distribución 1 (.mcgb)
│       ├── dist2/              # 47 instancias distribución 2 (.txt)
│       ├── clusters/           # 25 clusters del caso real (.mcgb)
│       └── cALL/               # 5 instancias cALL (caso completo, .mcgb)
├── src/                        # código fuente C++
│   ├── main.cpp                # punto de entrada (CLI + orquestación)
│   ├── instance/               # parser de instancias + structs del problema
│   ├── solution/               # representación + evaluación
│   ├── constraints/            # penalizaciones de capacidad y cuota
│   ├── construction/           # inicialización GRASP
│   ├── cultural/               # Algoritmo Cultural (population + belief + influencia + archive)
│   └── utils/                  # utilidades (RNG, seed, timing)
├── scripts/
│   ├── run_all.sh              # recolección de resultados por subset
│   ├── tune.sh                 # (legacy) tuning manual por OFAT + grid 2D
│   ├── compare_pm.sh           # comparación pm adaptativo vs pm fijo
│   ├── compare_archive.sh      # comparación archive vs no-archive
│   ├── wilcoxon_pm.py          # test estadístico pareado (Wilcoxon)
│   └── plot.py                 # gráficas (bar chart, boxplots, convergencia)
├── tuning/                     # escenario de irace
│   ├── instances/              # 10 instancias de sintonización
│   ├── parameters.txt          # 7 parámetros a sintonizar
│   ├── scenario.txt            # escenario irace
│   ├── target-runner.sh        # adaptador irace → gmb
│   ├── pilot-run-v1/           # primer irace (11 params, descartado debido a que fue una tirada de prueba)
│   └── final-run-v2/           # irace definitivo (7 params, config ganadora)
└── results/                    # salida de scripts/run_all.sh
    ├── _archive/               # CSVs de iteraciones descartadas (evidencia)
    ├── dist1_raw.csv           # una fila por (instancia, seed)
    ├── dist1_summary.csv       # agregado por instancia
    ├── dist1_baselines.csv     # by_grade y greedy
    ├── dist2_*.csv             # idem para dist2
    ├── clusters_*.csv          # idem para clusters
    └── cALL_*.csv              # idem para cALL
~~~

Cada archivo de instancia sigue un formato **AMPL-like**. Los archivos `.txt` (dist2) y `.mcgb` (dist1, clusters, cALL) comparten la misma estructura:

- `set Nodos`: todos los nodos, siendo `1000` el depósito.
- `set I`: granjas productoras.
- `set K`: camiones disponibles.
- `param Q`: capacidades de los camiones.
- `param D`: producción total a recolectar por grado (A, B, C).
- `param V`: precio por litro según grado final.
- `param U`: límites de SC/ml para definir cada grado.
- `param qu`: por cada granja, `id, producción, grado, SC/ml`.
- `param c`: matriz de distancias entre nodos (incluye depósito).

---

## 7. Descripción de módulos

### src/instance/

**Responsabilidad:** lectura y validación de archivos de instancia.

`parse_instance(filepath)` abre el archivo (`.txt` o `.mcgb`), parsea cada sección del formato AMPL-like y llena la struct `Instance` con:

- `vector<Farm> farms`: cada granja con `id`, `production`, `grade`, `sc_per_ml`.
- `vector<Truck> trucks`: cada camión con `id`, `capacity`.
- `vector<double> quota`, `revenue`, `sc_upper`: vectores de tamaño 3.
- `vector<vector<double>> cost`: matriz de distancias `(n+1)×(n+1)` con índice 0 = depósito.

El parser usa una máquina de estados: detecta el inicio de cada sección y acumula valores. Valida consistencia (D vs suma de producciones) e informa por `stderr` si detecta discrepancias.

### src/solution/

**Responsabilidad:** representación de soluciones y cálculo de métricas.

**Estructuras:**
- `Route`: `vector<int> farms` + `int truck_idx`.
- `Solution`: `vector<Route> routes`.
- `Metrics`: revenue, transport_cost, penalty_capacity, penalty_quota, profit, feasible, delivered_per_grade, volume_per_truck.

**Funciones:**
- `route_transport(inst, route)`: costo de `depot → farm₁ → … → farmₙ → depot`.
- `check_structure(inst, sol)`: valida estructura (cada granja una vez, `truck_idx` en rango).
- `evaluate(inst, sol, Pq, Pc)`: calcula todas las métricas. Para cada ruta, computa SC/ml ponderado, clasifica el blend según `sc_upper`, acumula revenue y transporte, y calcula penalizaciones.

### src/constraints/

**Responsabilidad:** penalizaciones por violación de restricciones.

- `capacity_violation(inst, volume_per_truck)`: litros excedidos sobre capacidad.
- `quota_violation(delivered, quota)`: litros faltantes por grado, con degradación A→B→C.

El diseño usa **penalización** (en lugar de reparación o codificación restringida) porque la cuota es global y la reparación destruiría la estructura de las soluciones generadas por crossover.

### src/construction/

**Responsabilidad:** construcción de soluciones iniciales vía GRASP.

- `graspx(inst, k_rcl)`: inserta granjas una a una eligiendo entre los `k_rcl` mejores candidatos (`delta_transport − expected_revenue`).
- `build_initial_population(inst, pop_size, k_rcl)`: `pop_size` llamadas a `graspx`.

### src/cultural/

**Responsabilidad:** Algoritmo Cultural completo.

**Population space:**
- Crossover RBX (heredar rutas + inyectar una).
- Mutación (3 operadores: swap intra-ruta, mover granja, swap entre rutas).
- Selección por torneo de tamaño `tour_k`.
- Elitismo de tamaño `elite_size`.

**Belief space:**
- **Circumstantial**: mejor solución + métricas.
- **Normativo**: rango `[min, max]` de SC/ml por grado en la mejor solución.
- **Archive**: top-`archive_size` soluciones infactibles por profit. Favorece la exploración hacia regiones infactibles prometedoras.

**Influencia**: al mover una granja, el peso del camión destino depende de:
- `0.3` si degrada el blend.
- `2.0` si se mantiene dentro del rango histórico del grado.
- `w_attract` si la granja pertenece al archive.
- `1.0` en otro caso.

### src/utils/

**Responsabilidad:** RNG reproducible.

- `set_seed`, `random_int`, `random_double`, `random_uint`. Todo usa `std::mt19937`.

---

## 8. Formato de salida

El ejecutable imprime en `stdout`:

1. **Encabezado de instancia y configuración**.
2. **Progreso del CA** (cada `verbose_every` generaciones):
   ~~~
   gen   100  evals=   2730  pm=0.05  best=19646.00  belief_best=19646.00  gens_stalled=15
   ~~~
   - `gen`: generación actual.
   - `evals`: evaluaciones acumuladas.
   - `pm`: probabilidad de mutación actual (dinámico si `adaptive_pm`).
   - `best`: mejor profit en la generación actual.
   - `avg`: promedio del profit de la población.
   - `belief_best`: mejor profit histórico (nunca baja).
   - `gens_stalled`: generaciones sin mejora del `belief_best`.
3. **Mejor solución detallada** (revenue, transport, penalizaciones, rutas).
4. **Métricas de evaluación**: `evals_total`, `evals_best`, tiempo.
5. **Comparación con baselines** (`by_grade`, `greedy_capacity`).
6. **Líneas CSV finales** (para parsing automático):
   ~~~
   RESULT,19682.00,20100.00,418.00,1,1000015,60785,0.771
   BASELINE,17904.00,1,14219.00,1
   ~~~

   - `RESULT`: profit, revenue, transport, feasible (1/0), evals_total, evals_best, time_s.
   - `BASELINE`: by_grade_profit, by_grade_feasible, greedy_profit, greedy_feasible.

---

## 9. Scripts auxiliares

### scripts/run_all.sh

Recolector de resultados por subset. Ejecuta el binario sobre todas las instancias del subset con 20 seeds (1..20) y genera 3 CSV por subset.

**Uso:**

~~~bash
chmod +x scripts/run_all.sh

# Un subset
./scripts/run_all.sh dist2 20
./scripts/run_all.sh dist1 20
./scripts/run_all.sh clusters 20
./scripts/run_all.sh cALL 20

# Todos los subsets secuencialmente
for s in dist2 dist1 clusters cALL; do
    ./scripts/run_all.sh "$s" 20
done

# Test rápido (solo 2 seeds)
SEED_START=1 SEED_END=2 ./scripts/run_all.sh dist2 2
~~~

**Parámetros configurables por variable de entorno:**

| Variable | Default | Descripción |
|----------|---------|-------------|
| `CA_POP` | `31` | Tamaño de población |
| `CA_ELITE` | `7` | Tamaño de élite |
| `CA_PC` | `0.8` | Probabilidad de crossover |
| `CA_PM` | `0.054` | Probabilidad de mutación |
| `CA_TOUR` | `5` | Tamaño de torneo |
| `CA_K_RCL` | `2` | RCL de GRASP |
| `CA_ARCHIVE` | `3` | Tamaño del archive |
| `CA_W_ATTRACT` | `2.10` | Peso de atracción |
| `CA_GENS_CAP` | `100000` | Cap de generaciones |
| `MAX_EVALS` | `1000000` | Presupuesto de evaluaciones |
| `SEED_START` | `1` | Primera seed |
| `SEED_END` | `20` | Última seed |

**Salidas por subset `<s>`:**

- `results/<s>_raw.csv`: una fila por `(instancia, seed)`.
- `results/<s>_summary.csv`: agregado por instancia (`best`, `avg`, `median`, `std`, `min_feasible_pct`, `avg_evals_best`).
- `results/<s>_baselines.csv`: by_grade y greedy por instancia.

**Ver el resumen:**

~~~bash
column -t -s',' results/dist2_summary.csv
~~~

### scripts/plot.py

Genera gráficas a partir de los CSVs de `results/`:

- Bar chart: CA vs by_grade vs greedy por subset.
- Boxplots: distribución de profit por instancia.
- Curvas de convergencia (requiere `--log-every-1000-evals`).

~~~bash
python3 scripts/plot.py
~~~

Requiere `matplotlib`, `numpy`, `scipy`.

### scripts/wilcoxon_pm.py

Test de Wilcoxon signed-rank pareado entre dos CSVs de resultados. Uso genérico:

~~~bash
python3 scripts/wilcoxon_pm.py results/<base>.csv results/<tratamiento>.csv
~~~

### scripts/compare_pm.sh y scripts/compare_archive.sh

Scripts legacy usados durante el desarrollo para comparar variantes del algoritmo (pm adaptativo, archive). Se conservan como evidencia de las iteraciones de diseño. Los resultados crudos están en `results/_archive/`.

### scripts/tune.sh

Script legacy de sintonización manual (OFAT + grid 2D) usado antes de adoptar irace. Se conserva por completitud.

---

## 10. Sintonización con irace

La sintonización de los 7 hiperparámetros del CA se realizó con **irace** (Iterated Racing). El escenario está en `tuning/`.

### Configuración final (ganadora)

| Parámetro | Valor óptimo |
|-----------|--------------|
| `pop_size` | **31** |
| `elite_size` | **7** |
| `pm` | **0.054** |
| `tour_k` | **5** |
| `k_rcl` | **2** |
| `archive_size` | **3** |
| `w_attract` | **2.10** |

**Interpretación:** el CA funciona mejor con **baja mutación** (`pm=0.054`) y **alto elitismo** (`elite=7` sobre `pop=31`, ≈23%), lo que indica que el aprendizaje del belief space reemplaza la necesidad de exploración aleatoria.

### Presupuesto de sintonización

- **Instancias de tuning**: 10 (ei130, a45, f45, ei151, a60, tai75B, att48, a72, c14, c9), excluidas de las tablas finales.
- **Presupuesto**: 5000 experiments (`maxExperiments` en `scenario.txt`).
- **Cada experiment**: ~500 evaluaciones del algoritmo.

### Features descartados

- **`pm` adaptativo**: `pm` variable según `gens_stalled`. Descartado empíricamente (0 de 5 élites lo activaron). Ver `results/_archive/pm_*.csv` para los datos de la comparación.
- **`archive` en el primer irace**: el primer run piloto (11 parámetros) sugirió que el archive no aportaba; el run definitivo con rango focalizado confirmó que sí aporta (config 406 tiene `archive_size=3`).

### Reproducir la sintonización

Requiere R + irace (v4.4.4 o similar):

~~~bash
# Instalar irace (una vez)
mkdir -p ~/R/library
echo 'R_LIBS_USER="~/R/library"' >> ~/.Renviron
Rscript -e 'install.packages("irace", repos="https://cloud.r-project.org", lib=Sys.getenv("R_LIBS_USER"))'

# Correr irace
cd tuning
Rscript -e '
  .libPaths(c("~/R/library", .libPaths()))
  library(irace)
  s <- readScenario("scenario.txt")
  s$execDir <- "<path-absoluto-al-proyecto>"
  s$parameterFile <- "<path>/tuning/parameters.txt"
  s$targetRunner <- "<path>/tuning/target-runner.sh"
  s$trainInstancesDir <- "<path>/tuning/instances"
  irace(s)
'
~~~

**Salida**: archivos en `tuning/final-run-v2/` (config ganadora + estadísticas).

---

## 11. Ejemplo de sesión

**Ejecución en `eil22` (dist2)** con la config ganadora y 1M de evaluaciones:

~~~bash
$ ./gmb data/instances/dist2/instanciaeil22-DB-2distribution.txt \
    --pop 31 --elite-size 7 --pm 0.054 --tour 5 --k_rcl 2 \
    --archive-size 3 --w-attract 2.10 \
    --gens 100000 --max-evals 1000000 --seed 1

=== Instancia: data/instances/dist2/instanciaeil22-DB-2distribution.txt ===
Granjas=21  Camiones=3  Cuota=[0, 0, 0]

=== CONFIGURACION CA ===
  pop=31  elite=7  gens_cap=100000  pc=0.8  pm=0.054  tour=5  k_rcl=2  max_evals=1000000
  archive_size=3  w_attract=2.1

=== EJECUTANDO CA ===
  (progreso cada 10 generaciones)

=== MEJOR SOLUCION (CA) ===
  Revenue:          20190.00
  Transport cost:   436.00
  Penalty (cap):    0.00
  Penalty (quota):  0.00
  Profit:           19754.00
  Feasible:         SI
  Litros entregados por grado: A=14500.00  B=8000.00  C=0.00
  Litros por camion: k0=...  k1=...  k2=...
  Rutas:
    camion 0 [14 granjas]: 15 7 5 2 1 8 6 3 4 11 13 19 14 18
    camion 1: (vacio)
    camion 2 [7 granjas]: 10 9 12 20 21 17 16

=== EVALUACIONES ===
  evals_total: 1000015
  evals_best:  60785
  tiempo:      0.771 s

=== BASELINES ===
  by_grade:  profit=17904.00  feasible=SI
  greedy:    profit=14219.00  feasible=SI

RESULT,19754.00,20190.00,436.00,1,1000015,60785,0.771
BASELINE,17904.00,1,14219.00,1
~~~

**Análisis:**
- CA encontró solución **+10.3% mejor que `by_grade`** y **+39% mejor que `greedy_capacity`**.
- Leche C (2,500 L) diluida dentro de la carga B del camión 2.
- Leche B diluida dentro de la carga A del camión 0.
- Camión 1 vacío (decisión válida: menos transporte).

**En `eil30`** (donde `by_grade` falla por capacidad):

~~~bash
$ ./gmb data/instances/dist2/instanciaeil30-DB-2distribution.txt \
    --pop 31 --elite-size 7 --pm 0.054 --tour 5 --k_rcl 2 \
    --archive-size 3 --w-attract 2.10 \
    --gens 100000 --max-evals 1000000 --seed 1
...
  Profit:           11500.00 (aproximado)
  Feasible:         SI

=== BASELINES ===
  by_grade:  profit=-938.00  feasible=NO
  greedy:    profit=9112.50  feasible=SI
~~~

Aquí `by_grade` es **infactible** (grado A total > capacidad del camión asignado), mientras que el CA encuentra solución factible con profit positivo.

### Rendimientos decrecientes

Comparación de presupuesto (eil22, seed 1):

| Presupuesto | Profit | Tiempo |
|-------------|--------|--------|
| 100k evals | 19,646 | 0.03 s |
| 1M evals | **19,754** | 0.77 s |
| Δ (10x evals) | **+0.55%** | 25x |

El CA alcanza >99% del plateau con ~500k evaluaciones. Presupuestos >1M dan mejoras marginales (<0.5%).

---

## 12. Hardware y reproducibilidad

Los experimentos reportados en el informe se ejecutaron en la siguiente máquina:

| Componente | Especificación |
|------------|----------------|
| **CPU** | 11th Gen Intel(R) Core(TM) i5-11400H @ 2.70GHz (6 cores, 12 threads) |
| **RAM** | 7.6 GB (WSL2) |
| **Sistema operativo** | Linux (WSL2, Ubuntu 22.04) |
| **Compilador** | g++ (Ubuntu 11.4.0-1ubuntu1~22.04.3) 11.4.0 |
| **Flags** | `-std=c++17 -O2 -Wall -Wextra` |

**Reproducibilidad:**

- **Seeds**: 1..20.
- **Presupuesto**: 1,000,000 evaluaciones por corrida.
- **Determinismo**: mismo seed + misma instancia + mismos parámetros → mismos resultados.
- **Instancias**: disponibles en `data/instances/` o en el repo oficial `elimail/Gradual-Milk-Blending-Instances`.

### Comando completo de recolección de resultados

~~~bash
cd "Proyecto GMB"
make clean && make
for s in dist2 dist1 clusters cALL; do
    ./scripts/run_all.sh "$s" 20
done
~~~

Tiempo total estimado: **~2 horas** (132 min en la máquina de referencia).

---

**Autor:** Mauricio Paolo Ramos Hidalgo — mauricio.ramosh@usm.cl
**Curso:** INF388 — Universidad Técnica Federico Santa María, Valparaíso, Chile
**Última actualización:** 17 de Noviembre 2026