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
sem_t* compareMutex = NULL;

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
    int atoms;
} sharedMemory_t;

void errors(int errNum)
{
    char messages[13][50] = {
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
        " sem_open(3) error",
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
    if (params->no < 0 || params->nh < 0 || params->tb < 0 || params->tb > TB || params->ti < 0 || params->ti > TI) {
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
    sem_close(compareMutex);
    sem_unlink("xnesut00_compareMutex");
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
    compareMutex = sem_open("xnesut00_compareMutex", O_CREAT | O_EXCL, 0666, 1);
    if (compareMutex == SEM_FAILED) {
        errors(13);
    }
}

FILE* pDebug;

extern inline void debugPrint(const char* fmt, ...)
{
    sem_wait(printMutex);
    va_list args;
    va_start(args, fmt);
    vfprintf(pDebug, fmt, args);
    putc('\n', pDebug);
    fflush(pDebug);
    va_end(args);
    sem_post(printMutex);
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
    sem_post(printMutex);

    sem_wait(compareMutex);
    if (memory->numberOfH <=1) {
        sem_wait(printMutex);
        memory->printCount += 1;
        fprintf(pFile, "%d: O %d: not enough H\n", memory->printCount, id);
        sem_post(printMutex);
        memory->numberOfO -= 1;
        sem_post(compareMutex);
        exit(0);
    }

    memory->numberOfO-=1;
    sem_post(compareMutex);

    debugPrint("sem_wait mainMutex O, %d", id);
    sem_wait(mainMutex);
    memory->oxygenCounter += 1;
    if (memory->hydrogenCounter > 0) {
        debugPrint("sem_post hydroQueue o, %d", id);
        sem_post(hydroQueue);
        sem_post(hydroQueue);
        memory->hydrogenCounter -= 2;

        debugPrint("sem_post oxyQueue O, %d", id);
        sem_post(oxyQueue);
        memory->oxygenCounter -= 1;
    }
    else {
        debugPrint("sem_post mainMutex o, %d", id);
        sem_post(mainMutex);
    }

    debugPrint("sem_wait oxyQueue O, %d", id);
    sem_wait(oxyQueue);

    //memory->numberOfO -= 1;
    sem_wait(printMutex);
    memory->printCount += 1;
    fprintf(pFile, "%d: O %d: creating molecule %d\n", memory->printCount, id,
        memory->molecules);
    sem_post(printMutex);

    srand(time(NULL) * getpid());
    usleep(1000 * (rand() % (params->ti + 1)));

    sem_wait(printMutex);
    memory->printCount += 1;
    fprintf(pFile, "%d: O %d: molecule %d created\n", memory->printCount, id,
        memory->molecules);
    sem_post(printMutex);
    //???
    //  barrier . wait ()
    barrier(memory);
    //  mutex . signal ()

    debugPrint("sem_post mainMutex O, %d", id);
    sem_post(mainMutex);
    memory->molecules += 1;
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
    sem_post(printMutex);

    sem_wait(compareMutex);
    if (memory->numberOfH <= 0 || memory->numberOfO <= 0) {
        sem_wait(printMutex);
        memory->printCount += 1;
        fprintf(pFile, "%d: H %d: not enough O or H\n", memory->printCount, id);
        sem_post(printMutex);
        sem_post(compareMutex);
        exit(0);
    }

    sem_post(compareMutex);
    debugPrint("sem_wait main mutex H, %d", id);
    sem_wait(mainMutex);
    memory->hydrogenCounter += 1;
    if (memory->hydrogenCounter >= 1 && memory->oxygenCounter >= 0) {
        debugPrint("sem_post hydroQueue H, %d", id);
        sem_post(hydroQueue);
        debugPrint("sem_post hydroQueue H, %d", id);
        sem_post(hydroQueue);
        memory->hydrogenCounter -= 2;
        //  oxyQueue . signal ()
        debugPrint("sem_post oxyQueue H, %d", id);
        sem_post(oxyQueue);
        //  oxygen -= 1
        memory->oxygenCounter -= 1;
    }
    //  else :
    else {
        //  mutex . signal ()
        debugPrint("sem_post mainMutex H, %d", id);
        sem_post(mainMutex);
    }
    //  hydroQueue . wait ()

    debugPrint("sem_wait hydroQueue H, %d", id);
    memory->numberOfH-=1;
    sem_wait(hydroQueue);
    //memory->numberOfH -= 1;
    debugPrint("sem_wait post hydroQueue H, %d", id);
    //  bond ()
    sem_wait(printMutex);
    memory->printCount += 1;
    fprintf(pFile, "%d: H %d: creating molecule %d\n", memory->printCount, id,
        memory->molecules);
    sem_post(printMutex);

    sem_wait(printMutex);
    memory->printCount += 1;
    fprintf(pFile, "%d: H %d: molecule %d created\n", memory->printCount, id,
        memory->molecules);
    sem_post(printMutex);
    //  barrier . wait ()
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

    pDebug = fopen("debug.log", "a");
    if (pDebug == NULL) {
        errors(6);
    }

    setbuf(pFile, NULL);
    setbuf(pDebug, NULL);
    setbuf(stdout, NULL);
    sharedMemory_t* memory = mmap(NULL, sizeof(sharedMemory_t),
        PROT_READ | PROT_WRITE, MAP_SHARED | MAP_ANONYMOUS, -1, 0);
    memory->printCount = 0;
    memory->barrierCounter = 0;
    memory->hydrogenCounter = 0;
    memory->oxygenCounter = 0;
    memory->atoms = 0;
    memory->molecules = 1;
    memory->numberOfH = params.nh;
    memory->numberOfO = params.no;

    semaphoresClear();
    semaphoresInit();
    debugPrint("hello world\n");
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