#include <stdio.h>
#include <stdlib.h>
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
        "Byly zadany argumenty v nespravnem rozsahu."};

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

int main(int argc, char *argv[])
{
    parameters params;
    paramControl(argc, argv, &params);
    printf("%d %d %d %d\n", params.no, params.nh, params.ti, params.tb);
    return 0;
}