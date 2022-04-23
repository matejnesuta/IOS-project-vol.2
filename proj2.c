#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <sys/stat.h>

#define TI 1000
#define TB 1000 


typedef struct
{
    int no;
    int nh;
    int ti;
    int tb;
} parameters;

void errors(int errNum)
{
    char messages[3][50] = {
        "Byl zadan nespravny pocet argumentu.",
        "Na vstupu nebyl zadan celocisleny parametr.",
        "Byly zadany argumenty v nespravnem rozsahu.",
        "Nepodarilo se rozdelit procesy."
    };

    fprintf(stderr, "%s\n", messages[errNum - 1]);
    exit(1);
}

int paramControl(int argc, char *argv[], parameters *params)
{
    if (argc != 5)
    {
        errors(1);
    }
    char *endptrs[4];
    for (int i=0; i<4;i++){
        endptrs[i] = '\0';
    }

    params->no = strtol(argv[1], &endptrs[0], 10);
    params->nh = strtol(argv[2], &endptrs[1], 10);
    params->ti = strtol(argv[3], &endptrs[2], 10);
    params->tb = strtol(argv[4], &endptrs[3], 10);
    for (int i=0; i<4;i++){
        if (endptrs[i][0] != '\0'){
            errors(2);
        }
    }
    if (params->no < 0 || params->nh < 0 || params->tb < 0 || params->tb > TB ||
        params->ti < 0 || params->ti > TI){
        errors(3);        
    }
    return 0;
}

void oxygen(){
    printf("oxygen\n");
    exit(0);
}

void hydrogen(){
    printf("hydrogen\n");
    exit(0);
}

int main(int argc, char *argv[])
{
    parameters params;
    paramControl(argc, argv, &params);
    int *m = mmap(NULL, sizeof(int), PROT_READ|PROT_WRITE, MAP_SHARED|MAP_ANONYMOUS, -1, 0);
    *m = 10;
    
    for (int i=0; i<params.no;i++){
        pid_t process = fork();
        if (process == 0){
            oxygen();
        }
        else if (process == -1){
            errors(4);
        }
    }

    for (int i=0; i<params.nh;i++){
        pid_t process = fork();
        if (process == 0){
            hydrogen();
        }
        else if (process == -1){
            errors(4);
        }
    }
    return 0;
}