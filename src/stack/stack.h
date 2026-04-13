//
// Created by jakub on 4/13/26.
//

#ifndef PLANT_STACK_H
#define PLANT_STACK_H

#include <stddef.h>

/* Kody powrotu */
#define STACK_OK  0
#define STACK_ERR (-1)

/* Typy wskaźników na funkcje */
typedef void (*object_destructor_function_t)(void*);
typedef int (*object_job_function_t)(void* obj, void* argstruct);

/* Opaque pointer - ukryta struktura stosu */
typedef struct stack stack_t;

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Tworzy nowy stos.
 * @param dtor Funkcja czyszcząca elementy stosu w przypadku jego zniszczenia. Może być NULL.
 * @return Wskaźnik na nowy stos lub NULL w przypadku błędu (ustawia errno = ENOMEM).
 */
stack_t* stack_create(object_destructor_function_t dtor);

/**
 * @brief Niszczy stos, wywołując destruktory dla elementów pozostających na stosie i zwalniając pamięć.
 * @param stack Wskaźnik na stos.
 * @return STACK_OK lub STACK_ERR (ustawia errno).
 */
int stack_destroy(stack_t* stack);

/**
 * @brief Odkłada nowy element na wierzchołek stosu (Push).
 * @param stack Wskaźnik na stos.
 * @param data Wskaźnik na dane.
 * @return STACK_OK lub STACK_ERR (ustawia errno).
 */
int stack_push(stack_t* stack, void* data);

/**
 * @brief Zdejmuje element z wierzchołka stosu (Pop).
 * @param stack Wskaźnik na stos.
 * @param out_data Miejsce na zapisanie wskaźnika do zdjętych danych.
 * Jeśli podano NULL, dane są niszczone za pomocą destruktora.
 * @return STACK_OK lub STACK_ERR (ustawia errno = ENOENT jeśli stos jest pusty).
 */
int stack_pop(stack_t* stack, void** out_data);

/**
 * @brief Zwraca rozmiar stosu (ilość elementów) poprzez parametr wyjściowy.
 * @param stack Wskaźnik na stos.
 * @param out_size Wskaźnik, pod którym zapisany zostanie rozmiar.
 * @return STACK_OK lub STACK_ERR (ustawia errno).
 */
int stack_size(stack_t* stack, size_t* out_size);

/**
 * @brief Sprawdza, czy stos jest pusty poprzez parametr wyjściowy.
 * @param stack Wskaźnik na stos.
 * @param out_is_empty 1 jeśli pusty, 0 jeśli nie.
 * @return STACK_OK lub STACK_ERR (ustawia errno).
 */
int stack_is_empty(stack_t* stack, int* out_is_empty);

/**
 * @brief Iteruje po wszystkich elementach stosu (od wierzchołka do dna).
 * @param stack Wskaźnik na stos.
 * @param job Funkcja do wykonania na każdym elemencie (zwrócenie != 0 przerywa iterację).
 * @param argstruct Dodatkowy argument przekazywany do funkcji job.
 * @return STACK_OK lub STACK_ERR (ustawia errno).
 */
int stack_foreach(stack_t* stack, object_job_function_t job, void* argstruct);

#ifdef __cplusplus
}
#endif

#endif //PLANT_STACK_H
