//
// Created by jakub on 4/13/26.
//

#ifndef C_COLLECTIONS_QUEUE_H
#define C_COLLECTIONS_QUEUE_H

#include <stddef.h>

/* Kody powrotu */
#define QUEUE_OK  0
#define QUEUE_ERR (-1)

/* Typy wskaźników na funkcje */
typedef void (*object_destructor_function_t)(void*);
typedef int (*object_job_function_t)(void* obj, void* argstruct);

/* Opaque pointer - ukryta struktura kolejki */
typedef struct queue queue_t;

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Tworzy nową kolejkę.
 * @param dtor Funkcja czyszcząca elementy kolejki w przypadku jej zniszczenia. Może być NULL.
 * @return Wskaźnik na nową kolejkę lub NULL w przypadku błędu (ustawia errno = ENOMEM).
 */
queue_t* queue_create(object_destructor_function_t dtor);

/**
 * @brief Niszczy kolejkę, wywołując destruktory dla elementów pozostających w kolejce i zwalniając pamięć.
 * @param q Wskaźnik na kolejkę.
 * @return QUEUE_OK lub QUEUE_ERR (ustawia errno).
 */
int queue_destroy(queue_t* q);

/**
 * @brief Dodaje nowy element na koniec kolejki (Enqueue).
 * @param q Wskaźnik na kolejkę.
 * @param data Wskaźnik na dane.
 * @return QUEUE_OK lub QUEUE_ERR (ustawia errno).
 */
int queue_enqueue(queue_t* q, void* data);

/**
 * @brief Zdejmuje element z początku kolejki (Dequeue).
 * @param q Wskaźnik na kolejkę.
 * @param out_data Miejsce na zapisanie wskaźnika do zdjętych danych.
 * Jeśli podano NULL, dane są niszczone za pomocą destruktora.
 * @return QUEUE_OK lub QUEUE_ERR (ustawia errno = ENOENT jeśli kolejka jest pusta).
 */
int queue_dequeue(queue_t* q, void** out_data);

/**
 * @brief Zwraca rozmiar kolejki poprzez parametr wyjściowy.
 * @param q Wskaźnik na kolejkę.
 * @param out_size Wskaźnik, pod którym zapisany zostanie rozmiar.
 * @return QUEUE_OK lub QUEUE_ERR (ustawia errno).
 */
int queue_size(queue_t* q, size_t* out_size);

/**
 * @brief Sprawdza, czy kolejka jest pusta poprzez parametr wyjściowy.
 * @param q Wskaźnik na kolejkę.
 * @param out_is_empty 1 jeśli pusta, 0 jeśli nie.
 * @return QUEUE_OK lub QUEUE_ERR (ustawia errno).
 */
int queue_is_empty(queue_t* q, int* out_is_empty);

/**
 * @brief Iteruje po wszystkich elementach kolejki (od początku do końca).
 * @param q Wskaźnik na kolejkę.
 * @param job Funkcja do wykonania na każdym elemencie (zwrócenie != 0 przerywa iterację).
 * @param argstruct Dodatkowy argument przekazywany do funkcji job.
 * @return QUEUE_OK lub QUEUE_ERR (ustawia errno).
 */
int queue_foreach(queue_t* q, object_job_function_t job, void* argstruct);

#ifdef __cplusplus
}
#endif

#endif //C_COLLECTIONS_QUEUE_H
