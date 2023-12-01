#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <string.h>


#define SIMPLE ">"
#define FORCE ">|"
#define SANS_ECRASEMENT ">>"
#define SORTIE_ERREUR "2>"
#define SORTIE_ERREUR_FORCE "2>|"
#define SORTIE_ERREUR_SANS_ECRASEMENT "2>>"



int numberOfRedirection(char **commande){
    int i = 0;
    int number = 0;
    while(1){
        char *tmp = commande[i];
        if (tmp == NULL){
            number++;
        }
        if(strcmp(tmp,SIMPLE)==0){
            number++;        
        }
        if(strcmp(tmp,FORCE)==0){
            number++;        
        }
        if(strcmp(tmp,SANS_ECRASEMENT)==0){
            number++;        
        }
        if(strcmp(tmp,SORTIE_ERREUR)==0){
            number++;        
        }
        if(strcmp(tmp,SORTIE_ERREUR_FORCE)==0){
            number++;        
        }
        if(strcmp(tmp,SORTIE_ERREUR_SANS_ECRASEMENT)==0){
            number++;
        }
        i++;
    }

    return number;
}

int isRedirection(char **commande){
    int i = 0;
    while(1){
        char *tmp = commande[i];
        if (tmp == NULL){
            return -1;
        }
        if(strcmp(tmp,SIMPLE)==0){
            return i;
        }
        if(strcmp(tmp,FORCE)==0){
            return i;
        }
        if(strcmp(tmp,SANS_ECRASEMENT)==0){
            return i;
        }
        if(strcmp(tmp,SORTIE_ERREUR)==0){
            return i;
        }
        if(strcmp(tmp,SORTIE_ERREUR_FORCE)==0){
            return i;
        }
        if(strcmp(tmp,SORTIE_ERREUR_SANS_ECRASEMENT)==0){
            return i;
        }
        i++;
    }

    return -1;
}

char ** getCommandeOfRedirection (char **commande){
    int size = isRedirection(commande);
    char **newCommande= malloc(sizeof(char*) * (size + 1));

    for(unsigned i = 0;i<size;i++){
        newCommande[i] = commande[i];
    }

    newCommande[size]=NULL;

    free(commande);
    return newCommande;
}


int* getDescriptorOfRedirection(char **commande){
    int *fd = malloc(sizeof(int)*2);
    fd[0]=-1;
    fd[1]=-1;

    int index_err = -1;
    int index_std = -1;

    char *standard = NULL;
    char *erreur = NULL;

    int i = 0;

    /*
    *On recupere le dernier fichier de redirection pour la sortie standard et la sortie erreur
    */
    while(1){
        char *tmp = commande[i];
        if(tmp == NULL){
            break;
        }
        if(strcmp(tmp,SIMPLE)==0||strcmp(tmp,FORCE)==0||strcmp(tmp,SANS_ECRASEMENT)==0){
            standard = commande[i+1];
            index_std = i;
        }
        if(strcmp(tmp,SORTIE_ERREUR)==0||strcmp(tmp,SORTIE_ERREUR_FORCE)==0||strcmp(tmp,SORTIE_ERREUR_SANS_ECRASEMENT)==0){
            erreur = commande[i+1];
            index_err = i;
        }
        i++;
    }

    /*
    *On ouvre le fichier en fonction de la redirection
    */

    if(index_err >0){
        if(strcmp(commande[index_err],SORTIE_ERREUR)==0){
            fd[1] = open(erreur,O_WRONLY|O_CREAT|O_EXCL,0666);
        }
        else{
            if(strcmp(commande[index_err],SORTIE_ERREUR_FORCE)==0){
                fd[1] = open(erreur,O_WRONLY|O_CREAT|O_TRUNC,0666);
            }
            else{
                fd[1] = open(erreur,O_WRONLY|O_CREAT|O_APPEND,0666);
            }
        }
    }
    if(index_std>0){
        if(strcmp(commande[index_std],SIMPLE)==0){
            fd[0] = open(standard,O_WRONLY|O_CREAT|O_EXCL,0666);
        }
        else{
            if(strcmp(commande[index_std],FORCE)==0){
                fd[0] = open(standard,O_WRONLY|O_CREAT|O_TRUNC,0666);
            }
            else{
                fd[0] = open(standard,O_WRONLY|O_CREAT|O_APPEND,0666);
            }
        }
    }

    

    /*
    *On redirige la sortie standard 1 vers fd[0] si il est superieur a 0
    * On redirige la sortie erreur 2 vers fd[1] si il est superieur a 0
    */

    if(fd[0]>0){
        dup2(fd[0],1);
    }
    if(fd[1]>0){
        dup2(fd[1],2);
    }


    return fd;
}
