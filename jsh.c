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
#define MAX_PATH_SIZE 2048

#define NUMBER_OF_JOBS 0
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
        snprintf(new_string, 31, "[%d]...%s",NUMBER_OF_JOBS, *original + len - 22);
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
    if (!pwd) return NULL;      // -> adapter du coup tout le reste
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

char** get_tab_of_commande  (char* commande){
    char *copy1 = strdup(commande);
    if (!copy1) exit(EXIT_FAILURE);

    int size = 0;
    for (char *x = strtok(copy1, " "); x != NULL; x = strtok(NULL, " ")) {
        size++;
    }
    free(copy1);

    char **commande_args= malloc(sizeof(char*) * (size + 1));
    if (commande_args == NULL) exit(EXIT_FAILURE);

    char *copy2 = strdup(commande);
    if (!copy2) exit(EXIT_FAILURE);

    int i = 0;
    for (char *token = strtok(copy2, " "); token != NULL; token = strtok(NULL, " ")) {
        commande_args[i] = strdup(token);
        if (commande_args[i] == NULL) exit(EXIT_FAILURE);
        i++;
    }
    free(copy2);
    commande_args[i] = NULL;

    return commande_args;
}

int execute_commande_externe(char **commande_args){
    pid_t pid = fork();

    if(pid == 0){
        execvp(commande_args[0], commande_args);
        fprintf(stderr,"erreur avec commande : %s\n",commande_args[0]);
        exit(EXIT_FAILURE);
    }
    else{
        int s;
        waitpid(pid,&s,0);
        if(WIFEXITED(s)){
            return WEXITSTATUS(s);
        }
        return 0;
    }
}

int change_precedent(char **prec,char *new){
    int size = strlen(new);
    free(*prec);
    *prec = malloc(sizeof(char)*(size+1));
    strcpy(*prec,new);
    
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
        printf("Erreur: cd prend un seul %d argument\n", size);
        return 1;
    }
    if(strcmp(commande_args[1],"-") == 0){
        chdir(*precedent);
    }
    else{
        int n = chdir(commande_args[1]);
          if(n == -1){
            printf("Erreur: le dossier n'existe pas\n");
            free(pwd);
            return 1;
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
    char *precedent = execute_pwd();
   
    int last_return_code = 0;
    
    using_history();
    rl_outstream = stderr;

    while(1){
        char *path = path_shell("$ ",blue);
        
        input = readline(path);
        if(input == NULL){
            exit(0);
        }
        free(path);
        if(strlen(input) == 0){
            continue;
        }

        add_history(input);
        char **commande_args = get_tab_of_commande(input);

        if(strcmp(commande_args[0],"exit") == 0){
            if(commande_args[1] != NULL){
                exit(atoi(commande_args[1]));
            }
            else{
                exit(last_return_code);
            }
        }
        else if (strcmp(commande_args[0],"?") == 0){
            printf("%d\n",last_return_code);
        }
        else if(strcmp(commande_args[0],"cd") == 0){
            last_return_code = execute_cd(commande_args, &precedent);
        }
        else{
            last_return_code = execute_commande_externe(commande_args);
        }
        
        free(commande_args);
    }
    free(precedent);
    clear_history();
    
    
    return 0;
}
