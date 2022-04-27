#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <time.h>
#include <semaphore.h>

#define TI 1000
#define TB 1000

sem_t *printMutex = NULL;

typedef struct
{
    int no;
    int nh;
    int ti;
    int tb;
} parameters_t;

typedef struct
{
    int printCount;
    int molecules;
    int numberOfO;
    int numberOfH;
} sharedMemory_t;

void errors(int errNum)
{
    char messages[5][50] = {
        "Byl zadan nespravny pocet argumentu.",
        "Na vstupu nebyl zadan celocisleny parametr.",
        "Byly zadany argumenty v nespravnem rozsahu.",
        "Nepodarilo se rozdelit procesy.",
        "sem_open(3) error"};

    fprintf(stderr, "%s\n", messages[errNum - 1]);
    exit(1);
}

int paramControl(int argc, char *argv[], parameters_t *params)
{
    if (argc != 5)
    {
        errors(1);
    }
    char *endptrs[4];
    for (int i = 0; i < 4; i++)
    {
        endptrs[i] = '\0';
    }

    params->no = strtol(argv[1], &endptrs[0], 10);
    params->nh = strtol(argv[2], &endptrs[1], 10);
    params->ti = strtol(argv[3], &endptrs[2], 10);
    params->tb = strtol(argv[4], &endptrs[3], 10);
    for (int i = 0; i < 4; i++)
    {
        if (endptrs[i][0] != '\0')
        {
            errors(2);
        }
    }
    if (params->no < 0 || params->nh < 0 || params->tb < 0 || params->tb > TB ||
        params->ti < 0 || params->ti > TI)
    {
        errors(3);
    }
    return 0;
}

void semaphoresClear()
{
    sem_close(printMutex);
    sem_unlink("printMutex");
}

void semaphoresInit()
{
    printMutex = sem_open("printMutex", O_CREAT | O_EXCL, 0666, 1);

    if (printMutex == SEM_FAILED)
    {
        errors(5);
    }
}

void oxygen(int id, sharedMemory_t *memory, parameters_t *params, 
    FILE * pFile)
{
    sem_wait(printMutex);
    memory->printCount += 1;
    fprintf(pFile, "%d: O %d: started\n", memory->printCount, id);
    sem_post(printMutex);

    srand(time(NULL) * getpid());
    usleep(1000 * (rand() % (params->ti + 1)));

    sem_wait(printMutex);
    memory->printCount += 1;
    fprintf(pFile, "%d: O %d: going to queue\n", memory->printCount, id);
    sem_post(printMutex);

    fclose(pFile);
    exit(0);
}

void hydrogen(int id, sharedMemory_t *memory, parameters_t *params,
    FILE * pFile)
{
    sem_wait(printMutex);
    memory->printCount += 1;
    fprintf(pFile, "%d: H %d: started\n", memory->printCount, id);
    sem_post(printMutex);

    srand(time(NULL) * getpid());
    usleep(1000 * (rand() % (params->ti + 1)));

    sem_wait(printMutex);
    memory->printCount += 1;
    fprintf(pFile, "%d: H %d: going to queue\n", memory->printCount, id);
    sem_post(printMutex);

    fclose (pFile);
    exit(0);
}

int main(int argc, char *argv[])
{
    parameters_t params;
    paramControl(argc, argv, &params);

    FILE * pFile;
    pFile = fopen ("proj2.out","w");
    if (pFile==NULL)
    {
    } 

    setbuf(pFile, NULL);

    sharedMemory_t *memory = mmap(NULL, sizeof(sharedMemory_t),
                                  PROT_READ | PROT_WRITE, MAP_SHARED | MAP_ANONYMOUS, -1, 0);
    memory->printCount = 0;
    
    semaphoresClear();
    semaphoresInit();

    for (int i = 1; i <= params.no; i++)
    {
        pid_t process = fork();
        if (process == 0)
        {
            oxygen(i, memory, &params, pFile);
        }
        else if (process == -1)
        {
            errors(4);
        }
    }

    for (int i = 1; i <= params.nh; i++)
    {
        pid_t process = fork();
        if (process == 0)
        {
            hydrogen(i, memory, &params, pFile);
        }
        else if (process == -1)
        {
            errors(4);
        }
    }
    while (wait(NULL) > 0)
        ;
    semaphoresClear();
    fclose (pFile);
    return 0;
}