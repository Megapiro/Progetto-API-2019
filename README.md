# Prova Finale di Algoritmi e Strutture Dati -  A.A 2018-2019
  Scopo del progetto è quello di implementare in linguaggio C standard un sistema in grado di gestire __entità__ e __relazioni__ . 
  
  L'istanza da gestire è fornita in un file di testo ricevuto come standard input, nel quale vengono definite le __entità__ da monitorare e le __relazioni__ tra queste.
  
  Attraverso i seguenti comandi:
  
  * addent
  * addrel
  * delent
  * delrel
  * report
  * end
  
  vengono istanziate nuove entità o relazioni (__addent__ e __addrel__) ed eliminate (__delent__ e __delrel__); il comando __report__ serve per verificare la corretta gestione del caso fornito.
  
  La __report__ scrive su standard output, in ordine alfabetico, tutte le relazioni che sta gestendo il sistema fornendo per ciascuna di queste, l'entità che riceve il maggior numero di quella relazione; in caso più entità ricevessero lo stesso numero di una relazione verrebbero considerate tutte in ordine alfabetico prima del numero di occorrenze.
  
  ## Implementazione
  Le strutture dati utilizzate per gestire il sistema sono di 3 tipi:
  * Hash Table
  * Lista Doppiamente Concatenata
  * Albero Rosso Nero
  
  Appena una __entità__ inizia ad essere monitorata, viene inserito un nodo all'interno di una hash-table il cui indice è determinato con la funzione di hash [djb2](http://www.cse.yorku.ca/~oz/hash.html) che opera sul nome dell'entità stessa.
  
  Appena una __relazione__ inizia ad essere monitorata, attraverso una lista doppiamente concatenata, viene inserito il nodo relativo a questa in modo tale che, in seguito a nuovi inserimenti della stessa relazione, vengano aggiornati solamente i contenuti delle strutture dati presenti all'interno di ciascun nodo.
  
  Ciascun nodo della lista di relazioni contiene:
  * Nome
  * Massimo occorrenze
  * Albero Rosso Nero delle entità riceventi
  * Albero Rosso Nero per la report
  
  In questo modo per qualsiasi operazione che coinvolga una __entità__ o __relazione__ si va ad aggiungere (o eliminare) nell'albero delle __entità__ riceventi il nodo relativo alla __entità__ in esame e, in caso, ad aggiornare il contatore e il relativo albero per la report. I nodi dell'albero delle __entità__ riceventi posseggono ciascuno un altro albero che tiene traccia delle __entità__ da cui quella __relazione__ è ricevuta.
  
  I principali __algoritmi__ utilizzati per la gestione delle strutture dati illustrate sono relativi principalmente all'aggiunta e alla eliminazione di nodi in un __albero rosso-nero__ e l'esplorazione di questo attraverso [tree trasversal visits](https://en.wikipedia.org/wiki/Tree_traversal).
  
  ## Casi di Test
  L'implementazione è stata testata e debuggata attraverso i [Test Pubblici](https://github.com/Megapiro/Progetto-API-2019/tree/master/Public_Tests) e valutata attraverso i [Test Privati](https://github.com/Megapiro/Progetto-API-2019/tree/master/Private_Tests) da una piattaforma apposita in grado di determinare la memoria occupata e il tempo di esecuzione del programma.
  
  ## Compilazione ed Esecuzione
  Per eseguire il programma compilare il file [main.c](https://github.com/Megapiro/Progetto-API-2019/blob/master/main.c) da linea di comando con i seguenti flag: 
  
  `gcc -Wmaybe-uninitialized -Wuninitialized -Wall -pedantic -Werror -g3 main.c -o main`
  
  Per eseguire il programma utilizzare uno degli input pubblici e verificare l'output ottenuto con il relativo output:
  
  ```
  cat input.txt | ./main > output.txt
  diff output.txt public_output.txt
  ```
