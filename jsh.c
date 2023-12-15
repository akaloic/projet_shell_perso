#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <stdbool.h>
#include <readline/readline.h>
#include <readline/history.h>
#include <sys/wait.h>

#include "redirection.h"
#include "colors.h"
#include "utils.h"
#include "pipe.h"
#include "jobs.h"
#include "kill.h"

/**
 * Represents the maximum size of the path
*/
#define MAX_PATH_SIZE 2048
/**
 * Represents the maximum size of shell_path
*/
#define TRONCATURE_SHELL 30

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

int execute_commande_externe(char **commande_args){
    int n = nbPipes(commande_args); //On regarde le nombre de pipes
    if(n != 0){ //Si il y a des pipes alors on execute la fonction add_jobs avec true en parametre
        return add_job(commande_args,true);
    }
    else{ //sinon on execute la fonction add_jobs avec false en parametre
        return add_job(commande_args,false);
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
    if(size == 1 || strcmp( commande_args[1],"$HOME")==0 ){
        char *home = getenv("HOME");
        if(home == NULL){
            fprintf(stderr,"Erreur: la variable d'environnement HOME n'est pas définie\n");
            free(pwd);
            return 1;
        }
        chdir(home);
        change_precedent(precedent,pwd);
        free(pwd);
        return 0;
    }
    if(size > 2){
        free(pwd);
        fprintf(stderr,"Erreur: cd prend un seul %d argument\n", size);
        return 1;
    }
    if(strcmp(commande_args[1],"-") == 0){
        chdir(*precedent);
    }
    else{
        int n = chdir(commande_args[1]);
          if(n == -1){
            fprintf(stderr,"Erreur: le dossier n'existe pas\n");
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
char *path_shell(char *signe, enum color job, enum color path){

    char *jobs = malloc(sizeof(char)*(2+number_length(getNbJobs())+1));  // Allocating the string for the jobs (2 brackets + number of jobs + null terminator)
    sprintf(jobs,"[%d]",getNbJobs());  // Adding the number of jobs to the string
    char *pwd = execute_pwd();  // Getting the path

    truncate_string(&pwd, TRONCATURE_SHELL-strlen(jobs)-strlen(signe));  // Truncating the path if it's too long

    color_switch(&jobs, job);   // Coloring the jobs
    color_switch(&pwd, path);   // Coloring the path

    char *prompt = calloc(sizeof(char),strlen(pwd)+strlen(jobs)+strlen(signe)+1);  // Allocating the prompt
    if (prompt == NULL) {
        fprintf(stderr,"Error in 'path_shell' : couldn't realloc...\n");
        exit(1);
    }

    strcat(prompt, jobs);  // Adding the jobs
    strcat(prompt, pwd);   // Adding the path
    strcat(prompt, signe); // Adding the signe

    free(pwd);  // Freeing the path
    free(jobs); // Freeing the jobs

    return prompt;
}

int main(int argc, char const *argv[]){
    
    char *input;
    char *precedent = execute_pwd();
   
    int last_return_code = 0;
    
    using_history();
    rl_outstream = stderr;

    while(1){

        char *path = path_shell("$ ", green, blue);
        
        input = readline(path);
        verify_done_jobs(); // We verify if there are jobs that are done, and we remove them from the list of jobs
        add_history(input);
        if(input == NULL){
            exit(last_return_code);
        }
        free(path);

        remove_last_spaces(&input); // Removing the last spaces only after verifying that the input is not empty
        if(strlen(input) == 0){    // If the input is empty after removing the last spaces, we continue
            continue;
        }

        char **commande_args;
        commande_args = get_tab_of_commande(input);
        if(commande_args == NULL){
            continue;
        }

        if(strcmp(commande_args[0],"exit") == 0){
            int numberOfJobs = getNbJobs();
            if(numberOfJobs != 0){  // If there are jobs running, we print an error message
                printf("Il y a %d jobs en cours d'execution\n",numberOfJobs);
                last_return_code = 1;
            }
            else{
                if(len(commande_args) > 2){ // If there are more than 2 arguments, we print an error message
                    printf("Erreur: exit prend un seul argument\n");
                    last_return_code = 1;
                }
                else if(len(commande_args) == 2 && !is_number(commande_args[1])){   // If there is one argument and it's not a number, we print an error message
                    printf("Erreur: argument de exit non valide\n");
                    last_return_code = 1;
                }
                else if(len(commande_args) == 2){   // If there is one argument and it's a number, we exit with the return code
                    last_return_code = atoi(commande_args[1]);
                    free(commande_args);
                    free(input);
                    free(precedent);
                    clear_history();
                    exit(last_return_code);
                }
                else{   // If there is no argument, we exit with the return code
                    free(commande_args);
                    free(input);
                    free(precedent);
                    clear_history();
                    exit(last_return_code);
                }
            }
            
        }
        else if (strcmp(commande_args[0],"?") == 0){
            printf("%d\n",last_return_code);
            last_return_code = 0;
        }
        else if(strcmp(commande_args[0],"cd") == 0){
            last_return_code = execute_cd(commande_args, &precedent);
        }
        else if(strcmp(commande_args[0],"jobs") == 0){
            last_return_code = print_all_jobs();
        }
        else if(strcmp(commande_args[0],"kill") == 0){
            bool wrong_args = false;
            if(len(commande_args) == 2){    // if we have command like "kill %1"
                if(start_with_char_then_digits(commande_args[1],'%')){ // if the second argument is an id string (starts with % and contains only digits)
                    last_return_code = kill_id(atoi(commande_args[1]+1));   // We remove the first character of the string (the %) and we convert it to an int
                }
                else{
                    wrong_args = true;
                }
            }
            else if(len(commande_args) == 3){   // if we have command like "kill -9 %1"
                if(start_with_char_then_digits(commande_args[1],'-') && start_with_char_then_digits(commande_args[2],'%')){
                    last_return_code = send_signal_to_id(atoi(commande_args[2]+1),atoi(commande_args[1]+1));   // We remove the first characters of the strings (the - and %) and we convert them to an int
                }
                else{
                    wrong_args = true;
                }

            }
            else{
                wrong_args = true;
            }

            if(wrong_args){
                last_return_code = execute_commande_externe(commande_args);
            }
        }
        else{
            last_return_code = execute_commande_externe(commande_args);
        }
        
        free(commande_args);
        free(input);
    }

    free(precedent);
    clear_history();
    
    
    return 0;
}
