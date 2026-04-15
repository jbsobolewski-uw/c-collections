# c-collections

> A java-style collections package for C.

**c-collections** to wydajna, bezpieczna i zorientowana obiektowo (w stylu języka C) biblioteka struktur danych. Jej
głównym celem jest dostarczenie programistom C wygodnych narzędzi znanych z języków wyższego poziomu, takich jak Java,
przy jednoczesnym zachowaniu niskopoziomowej wydajności.

## ✨ Główne cechy

* **Hermetyzacja (Opaque Pointers):** Pełne ukrycie implementacji wewnętrznych węzłów. Użytkownik manipuluje tylko
  wskaźnikami na główną strukturę.
* **Generyczność (`void*`):** Możliwość przechowywania dowolnych typów danych.
* **Zarządzanie pamięcią (Destruktory):** Każda kolekcja przyjmuje wskaźnik na funkcję niszczącą (
  `object_destructor_function_t`), dzięki czemu struktury potrafią same zwolnić pamięć swoich elementów podczas
  usuwania.
* **Recykling węzłów (Free-list):** Wbudowana optymalizacja alokacji pamięci. Usunięte węzły nie są od razu zwalniane do
  systemu (`free()`), lecz trafiają na wewnętrzny stos i są ponownie wykorzystywane przy kolejnych wstawieniach.
  Redukuje to narzut na powolne odwołania do systemu operacyjnego.
* **Jednolite API:** Standaryzowane kody powrotu (`_OK`, `_ERR`) oraz ścisła integracja z systemową zmienną `errno`.

---

## 📦 Pakiety i podpakiety

### 1. Singly Linked List (`slist.h` / `slist.c`)

Pakiet dostarczający prostą listę jednokierunkową. Idealna do przechowywania liniowych kolekcji danych, w których nie
znamy z góry ostatecznego rozmiaru.

* **Możliwości:** Dodawanie na początek, bezpieczne usuwanie konkretnego obiektu, sprawdzanie rozmiaru.
* **Iteracja:** Wbudowana funkcja `slist_foreach` pozwalająca na bezpieczne przejście po liście z możliwością przerwania
  operacji.

### 2. Queue (`queue.h` / `queue.c`)

Implementacja klasycznej kolejki **FIFO** (First-In, First-Out).

* **Wydajność:** O(1) dla operacji `enqueue` oraz `dequeue` dzięki wewnętrznemu śledzeniu zarówno początku (`head`), jak
  i końca (`tail`) kolejki.
* **Obsługa danych:** Pobieranie elementów z kolejki pozwala na odebranie wskaźnika przez użytkownika lub – w przypadku
  zignorowania – automatyczne wywołanie destruktora i usunięcie zasobu.

### 3. Stack (`stack.h` / `stack.c`)

Implementacja stosu **LIFO** (Last-In, First-Out).

* **Wydajność:** O(1) dla operacji `push` oraz `pop`, wszystkie operacje wykonywane są na wierzchołku struktury.
* **Zastosowanie:** Algorytmy śledzenia wstecznego (backtracking), maszyny stanów, parsery.

### 4. Binary Search Tree (`bst.h` / `bst.c`)

Wydajne drzewo poszukiwań binarnych oparte na własnym komparatorze.

* **Elastyczność:** Przyjmuje dedykowaną funkcję `object_comparator_function_t`, zwracającą ustandaryzowane makra (
  `BST_LE`, `BST_EQ`, `BST_GR`), co pozwala na sortowanie dowolnych struktur wedle własnych reguł.
* **Złożoność:** Gwarantuje szybkie wyszukiwanie (`bst_search`), wstawianie i bezpieczne usuwanie (z pełną obsługą
  przypadków węzłów z dwójką dzieci).

#### ↳ Podpakiet: BST Traversals (`bst_traversals.h` / `bst_traversals.c`)

Moduł rozszerzający funkcjonalność drzewa BST o algorytmy jego przeglądania (DFS). Implementacja korzysta z wewnątrznych
akcesorów drzewa zachowując jego całkowitą hermetyzację.

* Dostępne strategie (Enum `bst_traversal_t`):
    * **NLR** (Pre-order)
    * **LNR** (In-order - sortowanie rosnące)
    * **LRN** (Post-order)
    * **NRL** (Reverse Pre-order)
    * **RNL** (Reverse In-order - sortowanie malejące)
    * **RLN** (Reverse Post-order)

---

## 👤 Autor

**[Twoje Imię / Twój Nick]**

* GitHub: [@TwójProfil](https://github.com/TwojProfil)
* Kontakt: twoj.email@example.com
* *Krótki opis: Np. Pasjonat języka C, inżynierii oprogramowania i algorytmiki.*

---

## 📂 Struktura repozytorium

Poniżej znajduje się struktura plików w projekcie ilustrująca podział na nagłówki (API) i kod źródłowy implementacji:

```bash

c-collections/
├── src/
│   ├── bst/
│   │   ├── bst.c
│   │   ├── bst.h
│   │   ├── bst_traversals.c
│   │   ├── bst_traversals.h
│   │   └── CMakeLists.txt
│   ├── hashmap/
│   │   ├── CMakeLists.txt
│   │   ├── hashmap.c
│   │   └── hashmap.h
│   ├── id_manager/
│   │   ├── CMakeLists.txt
│   │   ├── id_manager.c
│   │   └── id_manager.h
│   ├── queue/
│   │   ├── CMakeLists.txt
│   │   ├── queue.c
│   │   └── queue.h
│   ├── slist/
│   │   ├── CMakeLists.txt
│   │   ├── slist.c
│   │   └── slist.h
│   ├── stack/
│   │   ├── CMakeLists.txt
│   │   ├── stack.c
│   │   └── stack.h
│   ├── CMakeLists.txt
│   ├── collections.c
│   └── collections.h
├── .clang-tidy
├── .gitignore
├── CMakeLists.txt
├── LICENSE
└── README.md
```
