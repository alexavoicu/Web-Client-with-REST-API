#include <stdio.h>      /* printf, sprintf */
#include <stdlib.h>     /* exit, atoi, malloc, free */
#include <unistd.h>     /* read, write, close */
#include <string.h>     /* memcpy, memset */
#include <sys/socket.h> /* socket, connect */
#include <netinet/in.h> /* struct sockaddr_in, struct sockaddr */
#include <netdb.h>      /* struct hostent, gethostbyname */
#include <arpa/inet.h>
#include "helpers.h"
#include "requests.h"
#include "parson.c"
#include "buffer.h"

#define HOST "34.246.184.49"

// o functie care verifica daca raspunsul intors de server contine o eroare
int error_in_response(char* response) {
    return strstr(response, "error") != NULL;
}

// o functie care afiseaza prompt-ul in stdout, care citeste string-ul de intrare si il pune in buffer
void read_input(char *prompt, char *buffer, size_t buffer_size) {
    printf("%s", prompt);
    memset(buffer, 0, buffer_size);
    if (fgets(buffer, buffer_size, stdin)) {
        buffer[strcspn(buffer, "\n")] = 0;
    } else {
        fprintf(stderr, "Error");
    }
}

// initierea unui obiect json
JSON_Object *init_json_object() {
    JSON_Value *root_value = json_value_init_object();
    return json_value_get_object(root_value);
}

// o functie care copiaza intr-un char ** datele necesare pentru cerea http
// folosita pentru cookies si body data
char **create_body_data(char* request_string) {
    char **data = calloc(1, 1);
    data[0] = calloc(LINELEN, 1);
    if (request_string == NULL) {
        data[0] = "no cookie";
        return data;
    }
    memcpy(data[0], request_string, strlen(request_string));
    return data;
}


void register_command(int sock);
char* login_command(int sock);
char* enter_library_command(int sock, char* sessionCookie);
void get_books_command(int sock, char* accessKey);
void add_book_command(int sock, const char* accessKey);
void get_book_command(int sock, const char* accessKey);
void delete_book_command(int sock, const char* accessKey);
void logout_command(int sock, char* sessionCookie);
char* extractCookie(char* serverResponse);
char* extractToken(char* serverResponse);
void registration_response(char *response);
void login_response(char *response);
void enter_response(char *response);
void get_books_response(char *response);
void get_book_response(char *response);
void add_book_response(char *response);
void delete_book_response(char *response);


int main() {
    int sock, user_logged_in = 0;
    char *sessionCookie = NULL, *accessToken = NULL;
    char inputCommand[4096];

    while (fgets(inputCommand, sizeof(inputCommand), stdin)) {
        // se citeste de la tastatura o linie
        // se dechide conexiunea cu serverul
        sock = open_connection(HOST, 8080, AF_INET, SOCK_STREAM, 0);
        // extragerea comenzii din linia citita
        char *command = strtok(inputCommand, " \t\n");

        // verificam ce comanda e, pentru fiecare se apeleaza functia care implementeaza comanda
        if (strcmp(command, "register") == 0) {
            // un user logat nu poate inregistra alt user
            if (user_logged_in) {
                printf("%s\n", "Eroare - Nu se poate inregistra un nou utilizator daca utilizatorul este logat!");
                continue;
            }
            register_command(sock);
        } else if (strcmp(command, "login") == 0) {
            // un user logat trebuie sa se delogheze inainte de a loga
            if (user_logged_in) {
                printf("%s\n", "Eroare - Nu se poate loga un nou utilizator daca utilizatorul este logat!");
                continue;
            }
            // retinem cookie-ul sesiunii si faptul ca este logat
            sessionCookie = login_command(sock);
            user_logged_in = sessionCookie != NULL;
        } else if (strcmp(command, "enter_library") == 0) {
            // retinem token-ul JWT
            accessToken = enter_library_command(sock, sessionCookie);
        } else if (strcmp(command, "get_books") == 0) {
            get_books_command(sock, accessToken);
        } else if (strcmp(command, "add_book") == 0) {
            add_book_command(sock, accessToken);
        } else if (strcmp(command, "get_book") == 0) {
            get_book_command(sock, accessToken);
        } else if (strcmp(command, "delete_book") == 0) {
            delete_book_command(sock, accessToken);
        } else if (strcmp(command, "logout") == 0) {
            logout_command(sock, sessionCookie);

            // daca s-a delogat, stergem cookie-ul si token-ul JWT si actualizam starea de logare
            sessionCookie = NULL;
            accessToken = NULL;
            user_logged_in = 0;
        } else if (strstr(inputCommand, "exit")) {
            // iesim din loop-ul de citire
            break;
        }

        // inchidem conexiunea cu serverul
        close(sock);
    }
    return 0;
}

void register_command(int sock) {
    char username[100];
    char password[100];

    // se citesc datele de la utilizator
    read_input("username=", username, sizeof(username));
    read_input("password=", password, sizeof(password));

    if (strstr(username, " ") != NULL || strstr(username, " ") != NULL) {
        printf("%s\n", "Eroare - Formatarea username-ului sau a parolei contine spatii!");
        return;
    }

    // punem datele citite intr-un JSON si formam string-ul pentru cerere
    JSON_Object *object = init_json_object();
    json_object_set_string(object, "username", username);
    json_object_set_string(object, "password", password);
    char *request_string = json_serialize_to_string_pretty(json_object_get_wrapping_value(object));

    // cream mesajul pe baza tipului cererii si a informatiilor citite
    char *message = compute_post_request(HOST, "/api/v1/tema/auth/register", "application/json", create_body_data(request_string), 1, NULL, 0);
    send_to_server(sock, message);
    char *response = receive_from_server(sock);
    // apelam functia care creeaza raspunsul pentru user
    registration_response(response);

}

void registration_response(char *response) {
    // in functie de tipul de cum arata eroare se intoarce un mesaj specific
    if (error_in_response(response)) {
        if (strstr(response, "taken") != NULL) {
            printf("%s\n", "Eroare - The username is taken!");
        } else printf("%s\n", "Eroare la inregistrare");
    } else printf("%s\n", "200 - OK - Utilizator înregistrat cu succes!");
}


char* login_command(int sock) {
    char username[100];
    char password[100];

    // se citesc datele de la utilizator
    read_input("username=", username, sizeof(username));
    read_input("password=", password, sizeof(password));

    if (strstr(username, " ") != NULL || strstr(username, " ") != NULL) {
        printf("%s\n", "Eroare - Formatarea username-ului sau a parolei contine spatii!");
        return NULL;
    }

    // punem datele citite intr-un JSON si formam string-ul pentru cerere
    JSON_Object *object = init_json_object();
    json_object_set_string(object, "username", username);
    json_object_set_string(object, "password", password);
    char *request_string = json_serialize_to_string_pretty(json_object_get_wrapping_value(object));

    // cream mesajul pe baza tipului cererii si a informatiilor citite
    char *message = compute_post_request(HOST, "/api/v1/tema/auth/login", "application/json", create_body_data(request_string), 1, NULL, 0);
    send_to_server(sock, message);
    char *response = receive_from_server(sock);
    // apelam functia care creeaza raspunsul pentru user
    login_response(response);
    
    // gasim in raspuns unde incepe linia de cookie
    char *beggining_of_cookie = strstr(response, "Set-Cookie:");
    if (beggining_of_cookie != NULL) {
        // daca ea exista, vedem unde se termina
        char *end_of_cookie = strstr(beggining_of_cookie, ";");
        // alocam spatiu pt cookie
        char *cookie = malloc(end_of_cookie - beggining_of_cookie - 12);
        // copiem cookie ul
        strncpy(cookie, beggining_of_cookie + 12, end_of_cookie - beggining_of_cookie - 12);
        cookie[end_of_cookie - beggining_of_cookie - 12] = '\0';

        return cookie;
    }    

    return NULL;
}

void login_response(char *response) {
    // in functie de tipul de cum arata eroare se intoarce un mesaj specific
    if (error_in_response(response)) {
        printf("%s\n", "Eroare - Credentials are not good!");
    } else printf("%s\n", "200 - OK - Utilizatorul a fost logat cu succes - Bun venit!");
}

char* enter_library_command(int sock, char* sessionCookie) {
    // cream mesajul pe baza tipului cererii si al cookie-ului de sesiune
    char *message = compute_get_request (HOST, "/api/v1/tema/library/access", NULL, create_body_data(sessionCookie), 1);
    send_to_server(sock, message);
    char *response = receive_from_server(sock);
    // apelam functia care creeaza raspunsul pentru user
    enter_response(response);

    if (!error_in_response(response)) {
        // daca mesajul nu este unul de eroare, gasim unde incepe token ul
        char *token_string = strstr(response, "{");
        // si unde se termina
        char *token_string_end = strstr(token_string, "\r\n");
        if (token_string_end != NULL) {
            *token_string_end = '\0';
        }
        // fiind in format json, il extragem parsand string ul si extragand valoarea corespunzatoare field-ului token
        const char *token = json_object_get_string(json_value_get_object(json_parse_string(token_string)), "token"); 
        return (char *)token;
    }

    return NULL;
}

void enter_response(char *response) {
    // in functie de tipul de cum arata eroare se intoarce un mesaj specific
    if (error_in_response(response)) {
        printf("%s\n", "Eroare - You are not logged in!");
    } else printf("%s\n", "200 - OK - Utilizatorul a accesat biblioteca cu succes!");
}

void get_books_command(int sock, char* accessKey) {
    // cream mesajul pe baza tipului cererii
    char *message = compute_get_request (HOST, "/api/v1/tema/library/books", NULL, NULL, 0);

    char *line = calloc(LINELEN, sizeof(char));

    // stergem linia goala de dupa header
    int mes_len = strlen(message);
    message[mes_len - 2] = 0;
    message[mes_len - 1] = 0;
    
    // cream o linie care sa contina token-ul de acces si o adaugam
    sprintf(line, "Authorization: Bearer %s", accessKey);
    compute_message(message, line);
    compute_message(message, "");

    send_to_server(sock, message);
    char *response = receive_from_server(sock);
    // apelam functia care creeaza raspunsul pentru user
    get_books_response(response);

    // daca in librarie se afla carti, gasim unde incep informatiile despre acestea
    // si le afisam
    char *library_books = strstr(response, "[");
    if (library_books != NULL) {
        printf("%s\n", library_books);
    }

}

void get_books_response(char *response) {
    // in functie de tipul de cum arata eroare se intoarce un mesaj specific
    if (error_in_response(response) || strstr(response, "Bad")) {
        printf("%s\n", "Eroare - Utilizatorul nu are acces la biblioteca!");
    } else printf("%s\n", "200 - OK - Utilizatorul a accesat cartile cu succes!");
}

void add_book_command(int sock, const char* accessKey) {
    char title[100];
    char author[100];
    char genre[100];
    char publisher[100];
    char page_count[100];

    // se citesc datele de la utilizator
    read_input("title=", title, sizeof(title));
    read_input("author=", author, sizeof(author));
    read_input("genre=", genre, sizeof(genre));
    read_input("publisher=", publisher, sizeof(publisher));
    read_input("page_count=", page_count, sizeof(page_count));

    // punem datele citite intr-un JSON si formam string-ul pentru cerere
    JSON_Object *object = init_json_object();
    json_object_set_string(object, "title", title);
    json_object_set_string(object, "author", author);
    json_object_set_string(object, "genre", genre);
    json_object_set_string(object, "page_count", page_count);
    json_object_set_string(object, "publisher", publisher);

    // cream o linie care sa contina token-ul de acces
    char *line = calloc(LINELEN, sizeof(char));
    sprintf(line, "Authorization: Bearer %s", accessKey);
    
    // cream mesajul pe baza tipului cererii si a informatiilor citite
    char *request_string = json_serialize_to_string_pretty(json_object_get_wrapping_value(object));
    char **data = create_body_data(request_string);
    char *message = compute_post_request(HOST, "/api/v1/tema/library/books", "application/json", data, 1, NULL, 0);
    
    // stergem informatiile despre carte pentru a putea adauga antetul cu token-ul si dupa le punem la loc
    message[strlen(message) - strlen(data[0]) - 2] = '\0';
    compute_message(message, line);
    compute_message(message, "");
    compute_message(message, data[0]);

    // verificam daca numarul de pagini este un numar si campul nu e gol
    // afisam eroarea
    char *endptr;
    if(strtol(page_count, &endptr, 10) == 0L && !(page_count[0] == '\0')) {
       printf("%s\n", "Eroare - Numarul de pagini nu este un int");
       return; 
    }

    send_to_server(sock, message);
    char *response = receive_from_server(sock);
    // apelam functia care creeaza raspunsul pentru user
    add_book_response(response);
}

void add_book_response(char *response) {
    // in functie de tipul de cum arata eroare se intoarce un mesaj specific
    if (error_in_response(response)) {
        if (strstr(response, "happened")) {
            printf("%s\n", "Eroare - Datele nu respecta formatarea");
        } else printf("%s\n", "Eroare - Utilizatorul nu are acces la biblioteca!");
    } else printf("%s\n", "200 - OK - Utilizatorul a adaugat cartea cu succes!");
}

void get_book_command(int sock, const char* accessKey) {
    char id[100];
    // se citesc datele de la utilizator
    read_input("id=", id, sizeof(id));

    // combinam url ul cu id ul cartii dorite
    char url[] = "/api/v1/tema/library/books/";
    strcat(url, id);

    // cream mesajul pe baza tipului cererii
    char *message = compute_get_request(HOST, url, NULL, NULL, 0);

    char *line = calloc(LINELEN, sizeof(char));

    // stergem linia goala de dupa header
    int mes_len = strlen(message);
    message[mes_len - 2] = 0;
    message[mes_len - 1] = 0;
    
    // cream o linie care sa contina token-ul de acces
    sprintf(line, "Authorization: Bearer %s", accessKey);
    compute_message(message, line);
    compute_message(message, "");

    send_to_server(sock, message);
    char *response = receive_from_server(sock);
    // apelam functia care creeaza raspunsul pentru user
    get_book_response(response);

    // daca cerea a intors o carte, gasim unde se afla inceputul informatiilor si le afisam
    char *library_book = strstr(response, "{");
    if (library_book != NULL && !error_in_response(response)) {
        printf("%s\n", library_book);
    }

}

void get_book_response(char *response) {
    // in functie de tipul de cum arata eroare se intoarce un mesaj specific
    if (error_in_response(response) || strstr(response, "Bad")) {
        if (strstr(response, "int")) {
            printf("%s\n", "Eroare - ID is not an int!");
        } else if (strstr(response, "book")) {
            printf("%s\n", "Eroare - No book was found with that id");
        } else printf("%s\n", "Eroare - Utilizatorul nu are acces la biblioteca!");
    } else printf("%s\n", "200 - OK - Utilizatorul a accesat cartea cu succes!");
}

void delete_book_command(int sock, const char* accessKey) {
    char id[100];
    // se citesc datele de la utilizator
    read_input("id=", id, sizeof(id));
    // combinam url ul cu id ul cartii vizate
    char url[] = "/api/v1/tema/library/books/";
    strcat(url, id);

    // cream mesajul pe baza tipului cererii
    char *message = compute_delete_request(HOST, url, NULL, 0);

    char *line = calloc(LINELEN, sizeof(char));

    // stergem linia goala de dupa header
    int mes_len = strlen(message);
    message[mes_len - 2] = 0;
    message[mes_len - 1] = 0;
    
    // cream o linie care sa contina token-ul de acces
    sprintf(line, "Authorization: Bearer %s", accessKey);
    compute_message(message, line);
    compute_message(message, "");

    send_to_server(sock, message);
    char *response = receive_from_server(sock);
    // apelam functia care creeaza raspunsul pentru user
    delete_book_response(response);
}

void delete_book_response(char *response) {
    // in functie de tipul de cum arata eroare se intoarce un mesaj specific
    if (error_in_response(response) || strstr(response, "Bad")) {
        if (strstr(response, "book")) {
            printf("%s\n", "Eroare - No book was found with that id");
        } else if (strstr(response, "decoding")) {
            printf("%s\n", "Eroare - Utilizatorul nu are acces la biblioteca!");
        } else printf("%s\n", "Eroare - ID is not an int!");
    } else printf("%s\n", "200 - OK - Utilizatorul a sters cartea cu succes!");
}

void logout_command(int sock, char* sessionCookie) {
    // cream mesajul pe baza tipului cererii si de cookie
    char *message = compute_get_request(HOST, "/api/v1/tema/auth/logout", NULL, create_body_data(sessionCookie), 1);
    send_to_server(sock, message);
    char *response = receive_from_server(sock);

    // in functie de tipul de cum arata eroare se intoarce un mesaj specific
    if (error_in_response(response)) {
        printf("%s\n", "Eroare - You are not logged in!");
    } else printf("%s\n", "200 - OK - Utilizatorul a fost delogat cu succes!");
}

