#include <fcntl.h>
#include <semaphore.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <time.h>
#include <unistd.h>

#define TI 1000
#define TB 1000

sem_t* printMutex = NULL;
sem_t* oxyQueue = NULL;
sem_t* hydroQueue = NULL;
sem_t* barrierMutex = NULL;
sem_t* turnstile = NULL;
sem_t* turnstile2 = NULL;
sem_t* mainMutex = NULL;
sem_t* endMutex = NULL;
sem_t* moleculeSem = NULL;

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
    int hydrogenCounter;
    int oxygenCounter;
    int numberOfO;
    int numberOfH;
    int barrierCounter;
    int maxMolecules;
    int atoms;
} sharedMemory_t;

void errors(int errNum)
{
    char messages[14][50] = {
        "Byl zadan nespravny pocet argumentu.",
        "Na vstupu nebyl zadan celocisleny parametr.",
        "Byly zadany argumenty v nespravnem rozsahu.",
        "Nepodarilo se rozdelit procesy.",
        "printMutex sem_open(3) error",
        "Nepodarilo se otevrit soubor.",
        "barrierMutex sem_open(3) error",
        "turnstile sem_open(3) error",
        "turnstile2 sem_open(3) error",
        "oxyQueue sem_open(3) error",
        "hydroQueue sem_open(3) error",
        "mainMutex sem_open(3) error",
        "endMutex sem_open(3) error",
        "moleculeSem sem_open(3) error",
    };

    fprintf(stderr, "%s\n", messages[errNum - 1]);
    exit(1);
}

int paramControl(int argc, char* argv[], parameters_t* params)
{
    if (argc != 5) {
        errors(1);
    }
    char* endptrs[4];
    for (int i = 0; i < 4; i++) {
        endptrs[i] = '\0';
    }

    params->no = strtol(argv[1], &endptrs[0], 10);
    params->nh = strtol(argv[2], &endptrs[1], 10);
    params->ti = strtol(argv[3], &endptrs[2], 10);
    params->tb = strtol(argv[4], &endptrs[3], 10);
    for (int i = 0; i < 4; i++) {
        if (endptrs[i][0] != '\0') {
            errors(2);
        }
    }
    if (params->no < 0 || params->nh < 0 || params->tb < 0 || params->tb > TB ||
     params->ti < 0 || params->ti > TI || (params->nh == 0 && params->no == 0)){
        errors(3);
    }
    return 0;
}

void semaphoresClear()
{
    sem_close(printMutex);
    sem_unlink("xnesut00_printMutex");
    sem_close(barrierMutex);
    sem_unlink("xnesut00_barrierMutex");
    sem_close(turnstile);
    sem_unlink("xnesut00_turnstile");
    sem_close(turnstile2);
    sem_unlink("xnesut00_turnstile2");
    sem_close(oxyQueue);
    sem_unlink("xnesut00_oxyQueue");
    sem_close(hydroQueue);
    sem_unlink("xnesut00_hydroQueue");
    sem_close(mainMutex);
    sem_unlink("xnesut00_mainMutex");
    sem_close(endMutex);
    sem_unlink("xnesut00_endMutex");
    sem_close(moleculeSem);
    sem_unlink("xnesut00_moleculeSem");
}

void semaphoresInit()
{
    printMutex = sem_open("xnesut00_printMutex", O_CREAT | O_EXCL, 0666, 1);
    if (printMutex == SEM_FAILED) {
        errors(5);
    }

    barrierMutex = sem_open("xnesut00_barrierMutex", O_CREAT | O_EXCL, 0666, 1);
    if (barrierMutex == SEM_FAILED) {
        errors(7);
    }

    turnstile = sem_open("xnesut00_turnstile", O_CREAT | O_EXCL, 0666, 0);
    if (turnstile == SEM_FAILED) {
        errors(8);
    }

    turnstile2 = sem_open("xnesut00_turnstile2", O_CREAT | O_EXCL, 0666, 1);
    if (turnstile2 == SEM_FAILED) {
        errors(9);
    }

    oxyQueue = sem_open("xnesut00_oxyQueue", O_CREAT | O_EXCL, 0666, 0);
    if (oxyQueue == SEM_FAILED) {
        errors(10);
    }

    hydroQueue = sem_open("xnesut00_hydroQueue", O_CREAT | O_EXCL, 0666, 0);
    if (hydroQueue == SEM_FAILED) {
        errors(11);
    }

    mainMutex = sem_open("xnesut00_mainMutex", O_CREAT | O_EXCL, 0666, 1);
    if (mainMutex == SEM_FAILED) {
        errors(12);
    }
    endMutex = sem_open("xnesut00_endMutex", O_CREAT | O_EXCL, 0666, 0);
    if (endMutex == SEM_FAILED) {
        errors(13);
    }
    moleculeSem = sem_open("xnesut00_moleculeSem", O_CREAT | O_EXCL, 0666, 0);
    if (endMutex == SEM_FAILED) {
        errors(14);
    }
}

void barrier(sharedMemory_t* memory)
{
    sem_wait(barrierMutex);
    memory->barrierCounter += 1;
    if (memory->barrierCounter == 3) {
        sem_wait(turnstile2);
        sem_post(turnstile);
    }
    sem_post(barrierMutex);
    sem_wait(turnstile);
    sem_post(turnstile);

    sem_wait(barrierMutex);
    memory->barrierCounter -= 1;
    if (memory->barrierCounter == 0) {
        sem_wait(turnstile);
        sem_post(turnstile2);
    }
    sem_post(barrierMutex);
    sem_wait(turnstile2);
    sem_post(turnstile2);
}

void oxygen(int id, sharedMemory_t* memory, parameters_t* params,
    FILE* pFile)
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
    memory->numberOfO ++;
    int queue = memory->numberOfO;
    sem_post(printMutex);

    if (queue > memory->maxMolecules) {
        sem_wait(printMutex);
        memory->printCount += 1;
        fprintf(pFile, "%d: O %d: not enough H\n", memory->printCount, id);
        sem_post(printMutex);
        fclose(pFile);
        exit(0);
    }

    sem_wait(mainMutex);
    memory->oxygenCounter += 1;
    if (memory->hydrogenCounter >= 2) {
        sem_post(hydroQueue);
        sem_post(hydroQueue);
        memory->hydrogenCounter -= 2;

        sem_post(oxyQueue);
        memory->oxygenCounter -= 1;
    }
    else {
        sem_post(mainMutex);
    }

    sem_wait(oxyQueue);

    sem_wait(printMutex);
    memory->printCount += 1;
    fprintf(pFile, "%d: O %d: creating molecule %d\n", memory->printCount, id,
        memory->molecules);
    memory->atoms++;
    sem_post(printMutex);
    if (memory->atoms == 3){
        sem_post(moleculeSem);
        sem_post(moleculeSem);
        sem_post(moleculeSem);
    }

    sem_wait(moleculeSem);
    srand(time(NULL) * getpid());
    usleep(1000 * (rand() % (params->ti + 1)));

    sem_wait(printMutex);
    memory->printCount += 1;
    fprintf(pFile, "%d: O %d: molecule %d created\n", memory->printCount, id,
        memory->molecules);
    if (memory->molecules == memory->maxMolecules && 
        queue == memory->maxMolecules){
            sem_post(endMutex);
    }
    sem_post(printMutex);
    barrier(memory);

    sem_post(mainMutex);
    memory->molecules += 1;
    memory->atoms=0;
    fclose(pFile);
    exit(0);
}

void hydrogen(int id, sharedMemory_t* memory, parameters_t* params,
    FILE* pFile)
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
    memory->numberOfH ++;
    int queue = memory->numberOfH;
    sem_post(printMutex);

    if (queue > memory->maxMolecules*2) {
        sem_wait(printMutex);
        memory->printCount += 1;
        fprintf(pFile, "%d: H %d: not enough O or H\n", memory->printCount, id);
        sem_post(printMutex);
        fclose(pFile);
        exit(0);
    }
    sem_wait(mainMutex);
    memory->hydrogenCounter += 1;
    if (memory->hydrogenCounter >= 2 && memory->oxygenCounter >= 1) {
        sem_post(hydroQueue);
        sem_post(hydroQueue);
        memory->hydrogenCounter -= 2;
        sem_post(oxyQueue);
        memory->oxygenCounter -= 1;
    }
    else {
        sem_post(mainMutex);
    }
    sem_wait(hydroQueue);
    sem_wait(printMutex);
    memory->printCount += 1;
    fprintf(pFile, "%d: H %d: creating molecule %d\n", memory->printCount, id,
        memory->molecules);
    memory->atoms++;
    sem_post(printMutex);
    if (memory->atoms == 3){
        sem_post(moleculeSem);
        sem_post(moleculeSem);
        sem_post(moleculeSem);
    }
    sem_wait(moleculeSem);
    sem_wait(printMutex);
    memory->printCount += 1;
    fprintf(pFile, "%d: H %d: molecule %d created\n", memory->printCount, id,
        memory->molecules);
    sem_post(printMutex);
    barrier(memory);
    fclose(pFile);
    exit(0);
}

int main(int argc, char* argv[])
{
    parameters_t params;
    paramControl(argc, argv, &params);

    FILE* pFile;
    pFile = fopen("proj2.out", "w");
    if (pFile == NULL) {
        errors(6);
    }

    setbuf(pFile, NULL);
    setbuf(stdout, NULL);
    sharedMemory_t* memory = mmap(NULL, sizeof(sharedMemory_t),
        PROT_READ | PROT_WRITE, MAP_SHARED | MAP_ANONYMOUS, -1, 0);
    memory->printCount = 0;
    memory->barrierCounter = 0;
    memory->hydrogenCounter = 0;
    memory->oxygenCounter = 0;
    memory->molecules = 1;
    memory->numberOfH = 0;
    memory->numberOfO = 0;
    memory->maxMolecules = (params.no < (params.nh/2)) 
        ? params.no : params.nh/2;
    memory->atoms = 0;

    semaphoresClear();
    semaphoresInit();
    for (int i = 1; i <= params.no; i++) {
        pid_t process = fork();
        if (process == 0) {
            oxygen(i, memory, &params, pFile);
        } else if (process == -1) {
            errors(4);
        }
    }

    for (int i = 1; i <= params.nh; i++) {
        pid_t process = fork();
        if (process == 0) {
            hydrogen(i, memory, &params, pFile);
        } else if (process == -1) {
            errors(4);
        }
    }
    while (wait(NULL) > 0)
        ;
    semaphoresClear();
    fclose(pFile);
    munmap(memory, sizeof(memory));
    return 0;
}