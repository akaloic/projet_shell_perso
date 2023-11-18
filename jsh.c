#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <readline/readline.h>
#include <readline/history.h>
#include <sys/wait.h>

/**
 * Represents the maximum size of the path
*/
#define MAX_PATH_SIZE 150

/**
 * Represents the maximum size of shell_path
*/
#define TRONCATURE_SHELL 30

/**
 * @brief Truncate the string to the size of TRONCATURE_SHELL
 * @param original The string to truncate
*/
void truncate_string(char **original) {
    int len = strlen(*original);
    if (len > TRONCATURE_SHELL-3) {
        char *new_string = malloc(TRONCATURE_SHELL+1); // 3 dots + TRONCATURE_SHELL-3 characters + null terminator
        snprintf(new_string, 31, "...%s", *original + len - 27);
        free(*original);
        *original = new_string;
    }
}

/**
 * Represents the different colors that can be used
*/
enum color {red,green,blue,yellow,cyan,white};

/**
 * @brief Add the color to the string, and put the color back to white at the end of the string
 * @param string The string to color
 * @param c The color to add
*/
void color_switch(char **string,enum color c){

    char *new_string = calloc((strlen(*string) + 2*strlen("\033[37m") + 1), sizeof(char));  // Allocating the new string

    const char *color;  // The color to add
    switch(c){  // Choosing the color
        case red:
            color = "\033[31m";
            break;
        case green:
            color = "\033[32m";
            break;
        case blue:
            color = "\033[34m";
            break;
        case yellow:
            color = "\033[33m";
            break;
        case cyan:
            color = "\033[36m";
            break;
        default:    // white
            color = "\033[37m";
            break;
    }

    strcat(new_string,color);   // Adding the color
    strcat(new_string,*string); // Adding the string
    strcat(new_string,"\033[37m");  // Adding the white color

    free(*string);  // Freeing the old string
    *string = new_string;   // Changing the pointer to the new string
}

/**
 * @brief Execute the command 'pwd' and return the path
*/
char *execute_pwd(){
    char *pwd = malloc(sizeof(char)*MAX_PATH_SIZE); // Allocating the path
    if(getcwd(pwd,MAX_PATH_SIZE) == NULL){  // Getting the path
        printf("Error in 'path_shell' : couldn't execute 'getcwd'...\n");
        exit(1);
    }
    if ((pwd = realloc(pwd, sizeof(char) * (strlen(pwd) + 1))) == NULL) {   // Reallocating the path to the right size
        printf("Error in 'path_shell' : couldn't realloc...\n");
        exit(1);
    }
    return pwd;
}

char** get_tab_of_commande  (char *commande){
    int size = 0;
    for(int i = 0; i<strlen(commande);i++){
        if(commande[i] == ' '){
            size++;
        }
    }
    int i = 0;
    char **commande_args= malloc(sizeof(char*)*(size+1));
    char *token = strtok(commande," ");
    while(token != NULL){
        commande_args[i] = token;
        token = strtok(NULL," ");
        i++;
    }
    commande_args[i] = NULL;

    return commande_args;
}

int execute_commande_externe(char **commande_args){
    pid_t pid = fork();
    int stat;

    if(pid == 0){
        int exec = execvp(commande_args[0],commande_args);
        return exec;
    }
    else{
        waitpid(pid, &stat, 0);
        return stat;
    }
}

void pointInterrogation(HIST_ENTRY *last_entry, int nb){
    if (strcmp(last_entry->line,"?") == 0){
        if (history_length < nb){
            fprintf(stderr, "Erreur : pas assez de commandes dans l'historique\n");
        }else{
            pointInterrogation(history_get(history_length - nb), nb + 1);
        }
    }else if (last_entry != NULL){
        char * last_commande = malloc(sizeof(char)*strlen(last_entry->line));
        if (last_commande == NULL){
            fprintf(stderr, "Erreur d'allocation mémoire\n");
            exit(EXIT_FAILURE);
        }

        strcpy(last_commande,last_entry->line);
        int recup = execute_commande_externe(get_tab_of_commande(last_commande));
        if (recup == -1) fprintf(stderr, "Erreur d'exécution lors de la commande : %s\n",last_commande);
        free(last_commande);
    }else{
        fprintf(stderr, "Erreur : pas assez de commandes dans l'historique\n");
    }
}

int change_precedent(char **prec,char *new){
    int size = strlen(new);
    free(*prec);
    *prec = malloc(sizeof(char)*(size+1));
    strcpy(*prec,new);
    printf("------prec : %s\n",*prec);
    
    return 0;
}

int execute_cd(char **commande_args,char **precedent){
    int size = 0;
    char *pwd = execute_pwd();

    while(commande_args[size] != NULL){
        size++;
    }
    if(size == 1 ||strcmp( commande_args[1],"$HOME")==0 ){
        chdir("/home");
        change_precedent(precedent,pwd);
        free(pwd);
        return 0;
    }
    if(size > 2){
        free(pwd);
        printf("Erreur: cd prend un seul argument\n");
        return -1;
    }
    if(strcmp(commande_args[1],"-") == 0){
        chdir(*precedent);
    }
    else{
        int n = chdir(commande_args[1]);
          if(n == -1){
            printf("Erreur: le dossier n'existe pas\n");
            free(pwd);
            return -1;
        }
    }
    change_precedent(precedent,pwd);
    free(pwd);
    return 0;
}

/**
 * @brief Return the path with the signe (like '$' or '>'), and color the path (without the signe)
*/
char *path_shell(char *signe, enum color c){
    char *pwd = execute_pwd();  // Getting the path
    truncate_string(&pwd);  // Truncating the path
    color_switch(&pwd,c);    // Adding the color to the path (and not the signe)
    pwd = realloc(pwd, sizeof(char)*(strlen(pwd) + strlen(signe) + 1));   // Increasing the size of the path to add the signe
    if (pwd == NULL) {
        printf("Error in 'path_shell' : couldn't realloc...\n");
        exit(1);
    }
    strcat(pwd, signe);   // Adding the signe to the path
    return pwd;
}

int main(int argc, char const *argv[]){
    
    char *input;
    char *precedent = execute_pwd(MAX_PATH_SIZE);
   
    
    using_history();

    while(1){
        char *path = path_shell("$ ",blue);
        input = readline(path);
        free(path);

        add_history(input);
        char **commande_args = get_tab_of_commande(input);

        if(strcmp(commande_args[0],"exit") == 0){
            break;
        }
        else if (strcmp(commande_args[0],"?") == 0){
            pointInterrogation(history_get(history_length - 1), 2);
        }
        else if(strcmp(commande_args[0],"cd") == 0){
            execute_cd(commande_args,&precedent);
        }
        else{
            execute_commande_externe(commande_args);
        }

        free(commande_args);
    }
    free(precedent);
    
    
    return 0;
}
