#pragma once
#include <random>

/*************************** 
void set_seed(unsigned int seed)
unsigned int seed: semilla para el generador aleatorio
retorna: nada, solo inicializa el estado global
***************************/
void set_seed(unsigned int seed);

/*************************** 
int random_int(int lo, int hi)
int lo: limite inferior (inclusive)
int hi: limite superior (inclusive)
retorna: entero uniforme en [lo, hi]
***************************/
int random_int(int lo, int hi);

/*************************** 
double random_double(double lo, double hi)
double lo: limite inferior
double hi: limite superior
retorna: double uniforme en [lo, hi)
***************************/
double random_double(double lo, double hi);

/*************************** 
unsigned int random_uint()
retorna: entero uniforme sin signo en todo el rango del generador
***************************/
unsigned int random_uint();