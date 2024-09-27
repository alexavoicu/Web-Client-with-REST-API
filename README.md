Tema 4. Client web. Comunicaţie cu REST API.

Voicu Alexa-Andreea 322D

Flow-ul programului este urmatorul:
In interiorul unei bucle se initiaza conexiunea cu serverul, se citeste de la tastatura o linie, dupa care comanda este extrasa.
In functie de ce comanda a fost data de utilizator, se va apela functiile care gestioneaza creearea unei cereri http si gestionarea raspunsului.

1. register
Se verifica daca un utilizator este deja autentificat. Daca da, se afiseaza un mesaj de eroare. Se citesc datele de intrare
pentru username si parola si se creeaza un obiect JSON cu acestea. Se trimite o cerere POST catre server si se primeste raspunsul pe baza caruia se
afiseaza mesajele de succes/eroare.
2. login
Se verifica daca un utilizator este deja autentificat. Daca da, se afiseaza un mesaj de eroare. Se citesc datele de intrare
pentru username si parola si se creeaza un obiect JSON cu acestea. Se trimite o cerere POST catre server si se primeste raspunsul pe baza caruia se
afiseaza mesajele de succes/eroare. Ulterior, daca autentificarea a reusit, se extrage cookie-ul de sesiune.

3. enter_library
Se trimite o cerere GET catre server pentru acces la biblioteca si se primeste raspunsul pe baza caruia se
afiseaza mesajele de succes/eroare. Ulterior, daca cererea a reusit, se extrage token-ul de acces JWT.

4. get_books
Se trimite o cerere GET catre server (incluzand token-ul de acces) si se primeste raspunsul pe baza caruia se
afiseaza mesajele de succes/eroare. Ulterior, daca cererea a reusit, se extrage si afiseaza lista de carti.

5. add_book
Se citesc datele de intrare pentru titlu, autor, gen, editura si numar de pagini si se creeaza un obiect JSON cu acestea.
Se verifica daca numarul de pagini are un format valid. 
Se trimite o cerere POST catre server (incluzand token-ul de acces) si se primeste raspunsul pe baza caruia se afiseaza 
mesajele de succes/eroare.

6. get_book
Se citesc datele de intrare pentru id, se compune url-ul, se trimite o cerere GET catre server (incluzand token-ul de acces) si se primeste 
raspunsul pe baza caruia se afiseaza mesajele de succes/eroare. Ulterior, daca cererea a reusit, se extrage si afiseaza informatiile despre carte.

7. delete_book
Se citesc datele de intrare pentru id, se compune url-ul, se trimite o cerere DELETE catre server (incluzand token-ul de acces) si se primeste 
raspunsul pe baza caruia se afiseaza mesajele de succes/eroare.

8. logout
Se trimite o cerere GET catre server (incluzand cookie-ul de sesiune) si se primeste raspunsul pe baza caruia se
afiseaza mesajele de succes/eroare. Se reseteaza variabilele pt cookie si token.

9. exit
Iese din bucla si termina executia programului.

Am ales sa folosesc bibliotecia de parsare Parson deoarece era sugerata de cerinta si mi s-a parut destul de usor de folosit si
de identificat cam care ar fi functiile pe care as putea sa le utilizez in implementarea mea.