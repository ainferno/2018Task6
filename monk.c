#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <time.h>
#include <signal.h>
#include <sys/sem.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <sys/msg.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/wait.h>

#define CHANCE 10
#define ROPE_LENGTH 10
#define MONK_NUM 10

int sem_id, is_son = 0;
struct sembuf op;

int check()
{
    return (rand() % 100 < CHANCE) ? 1 : 0;
}
void die()
{
    if(is_son)
        _exit(1);
}
//east = 0, weast = 1
//idle = 0, ready = 1, go = 2
void monkey(int pid, int side)
{
    int state = 0, pos = 0;
    while(1)
    {
        switch(state)
        {
            case 0:
                printf("Monkey No.%d spawn on %s side\n", pid, side ?  "west" : "east");
                op.sem_num = 2;
                op.sem_op = 0;
                semop(sem_id, &op, 1);
                state = 1;
            case 1:
                printf("Monkey No.%d idle on %s side\n", pid, side ?  "west" : "east");
                if(check())
                    state = 2;
                break;
            case 2:
                printf("Monkey No.%d ready on %s side\n", pid, side ?  "west" : "east");

                op.sem_num = side;
                op.sem_op = 0;
                semop(sem_id, &op, 1);
                
                printf("Monkey No.%d begin the trip to the %s side\n", pid, !side ?  "west" : "east");

                op.sem_num = !side;
                op.sem_op = 1;
                semop(sem_id, &op, 1);
                state = 3;
                break;
            case 3:
                if(pos >= ROPE_LENGTH)
                {
                    side = !side;
                    
                    op.sem_num = side;
                    op.sem_op = -1;
                    semop(sem_id, &op, 1);
                    
                    pos = 0;
                    state = 1;
                    printf("Monkey No.%d is now on %s side\n", pid, side ?  "west" : "east");
                    break;
                }
                pos++;
                printf("Monkey No.%d climbing the rope to %s side, it is now on %d / %d\n", pid, side ?  "east" : "west", pos, ROPE_LENGTH);
                sleep(1);
                break;
        }
    }
}

int main()
{
    int side = 0;
    sem_id = semget(IPC_PRIVATE, 3, 0666|IPC_CREAT);
    
    semctl(sem_id, 0, SETVAL, 0);
    semctl(sem_id, 1, SETVAL, 0);
    semctl(sem_id, 2, SETVAL, 1);

    void (*old)() = signal(SIGINT, die);
    for(int i = 0;i < MONK_NUM;i++)
    {
        side = (side + 1) % 2;
        if(fork() == 0)
        {
            srand(time(NULL));
            is_son = 1;
            monkey(i+1, side);
        }
        sleep(1);    
    }
    sleep(2);

    op.sem_num = 2;
    op.sem_op = -1;
    semop(sem_id, &op, 1);

    for(int i = 0;i < MONK_NUM;i++)
        wait(NULL);

    signal(SIGINT, old);

    semctl(sem_id, 0, IPC_RMID, NULL);
    return 0;
}