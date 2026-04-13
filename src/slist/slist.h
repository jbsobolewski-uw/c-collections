//
// Created by jakub on 4/13/26.
//

#ifndef C_COLLECTIONS_SLIST_H
#define C_COLLECTIONS_SLIST_H

#include <stddef.h>

/* Kody powrotu */
#define SLIST_OK  0
#define SLIST_ERR (-1)

/* Typy wskaźników na funkcje dla destruktora i operacji foreach */
typedef void (*object_destructor_function_t)(void*);

/* Funkcja foreach powinna zwrócić 0 aby kontynuować, lub wartość != 0 aby przerwać iterację */
typedef int (*object_job_function_t)(void* obj, void* argstruct);

/* Opaque pointer - ukryta struktura listy */
typedef struct slist slist_t;

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Tworzy nową listę.
 * @param dtor Funkcja czyszcząca elementy listy. Może być NULL.
 * @return Wskaźnik na nową listę lub NULL w przypadku błędu (ustawia errno = ENOMEM).
 */
slist_t* slist_create(object_destructor_function_t dtor);

/**
 * @brief Niszczy listę, wywołując destruktory dla istniejących elementów i zwalniając pamięć.
 * @param list Wskaźnik na listę.
 * @return SLIST_OK lub SLIST_ERR (ustawia errno).
 */
int slist_destroy(slist_t* list);

/**
 * @brief Dodaje nowy element na początek listy.
 * @param list Wskaźnik na listę.
 * @param data Wskaźnik na dane.
 * @return SLIST_OK lub SLIST_ERR (ustawia errno).
 */
int slist_add(slist_t* list, void* data);

/**
 * @brief Usuwa pierwsze wystąpienie elementu z listy, wywołując na nim destruktor.
 * @param list Wskaźnik na listę.
 * @param data Wskaźnik na usuwane dane.
 * @return SLIST_OK lub SLIST_ERR (ustawia errno = ENOENT, gdy nie znaleziono).
 */
int slist_remove(slist_t* list, void* data);

/**
 * @brief Zwraca rozmiar listy poprzez parametr wyjściowy.
 * @param list Wskaźnik na listę.
 * @param out_size Wskaźnik, pod którym zapisany zostanie rozmiar.
 * @return SLIST_OK lub SLIST_ERR (ustawia errno).
 */
int slist_size(slist_t* list, size_t* out_size);

/**
 * @brief Sprawdza, czy lista jest pusta poprzez parametr wyjściowy.
 * @param list Wskaźnik na listę.
 * @param out_is_empty 1 jeśli pusta, 0 jeśli nie.
 * @return SLIST_OK lub SLIST_ERR (ustawia errno).
 */
int slist_is_empty(slist_t* list, int* out_is_empty);

/**
 * @brief Iteruje po wszystkich elementach listy. Przerwanie następuje gdy job() zwróci wartość inną niż 0.
 * @param list Wskaźnik na listę.
 * @param job Funkcja do wykonania na każdym elemencie.
 * @param argstruct Dodatkowy argument przekazywany do funkcji job.
 * @return SLIST_OK lub SLIST_ERR (ustawia errno).
 */
int slist_foreach(slist_t* list, object_job_function_t job, void* argstruct);

#ifdef __cplusplus
}
#endif

#endif //C_COLLECTIONS_SLIST_H
