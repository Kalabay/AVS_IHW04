#include <iostream>
#include <pthread.h>
#include <unistd.h>
#include <random>

int result[3][3] = {
    -1, -1, -1,
    -1, -1, -1,
    -1, -1, -1
};
pthread_mutex_t mutexGetTask;
pthread_mutex_t mutexWrite;
pthread_mutex_t mutexCheckOne;
pthread_mutex_t mutexCheckTwo;
pthread_mutex_t mutexCheckThree;
std::random_device rd;
std::mt19937 mt(rd());

struct task  { //структура очереди
    int name = 1;  //для записи результата вычислений и дополнительной информации
    task *next = nullptr;  //указатель на следующий элемент очереди
};

task *head = nullptr;

void checkLock(int p) {
    switch (p) {
        case 0:
            pthread_mutex_lock(&mutexCheckOne);
        case 1:
            pthread_mutex_lock(&mutexCheckTwo);
        case 2:
            pthread_mutex_lock(&mutexCheckThree);
    }
}

void checkUnlock(int p) {
    switch (p) {
        case 0:
            pthread_mutex_unlock(&mutexCheckOne);
        case 1:
            pthread_mutex_unlock(&mutexCheckTwo);
        case 2:
            pthread_mutex_unlock(&mutexCheckThree);
    }
}

void checkCode(int p) {
    checkLock(p);
    for (int i = 0; i < 3; ++i) {
        if (result[i][p] == -2) {
            sleep(1 + (mt() % 2));
            result[i][p] = mt() % 2;
            if (result[i][p] == 0) {
                pthread_mutex_lock(&mutexWrite);
                std::cout << p + 1 << ": the task for the programmer " << i + 1 << " false!" << '\n';
                pthread_mutex_unlock(&mutexWrite);
            } else {
                pthread_mutex_lock(&mutexWrite);
                std::cout << p + 1 << ": the task for the programmer " << i + 1 << " true!" << '\n';
                pthread_mutex_unlock(&mutexWrite);
            }
        }
    }
    checkUnlock(p);
}

void doTask(int p, int whoCheck) {
    sleep(1 + (mt() % 5));
    pthread_mutex_lock(&mutexWrite);
    std::cout << "Trying to do with the programmer " << p + 1 << '\n';
    pthread_mutex_unlock(&mutexWrite);
    checkUnlock(p);
    result[p][whoCheck] = -2;
    checkCode(whoCheck);
    if (result[p][whoCheck] == 0) {
        checkLock(p);
        doTask(p, whoCheck);
    }
}

//стартовая функция
void* programmer(void *param) {
    int p = *(int *)param;  //номер потока (программиста)
    while (head != nullptr) {
        checkLock(p);
        pthread_mutex_lock(&mutexGetTask);
        task *taskNow = head;
        if (head != nullptr) {
            head = head->next;
        }
        pthread_mutex_unlock(&mutexGetTask);
        if (taskNow == nullptr) {
            checkUnlock(p);
            break;
        }
        int taskNum = taskNow->name;
        pthread_mutex_lock(&mutexWrite);
        std::cout << "The programmer " << p + 1 << " took the task " << taskNum << '\n';
        pthread_mutex_unlock(&mutexWrite);
        doTask(p, (p + (mt() % 2) + 1) % 3);
        pthread_mutex_lock(&mutexWrite);
        std::cout << "Task " << taskNum << " completed!" << '\n';
        pthread_mutex_unlock(&mutexWrite);
        delete taskNow;
    }
    return nullptr;
}

int main() {
    int n;
    std::cin >> n;
    if (n > 0) {
        head = new task;
    }
    task *last = head;
    for (int i = 1; i < n; ++i) {
        task *now = new task;
        now->name = i + 1;
        last->next = now;
        last = now;
    }
    pthread_t programmers[3];
    int num[3];
    for (int i = 0; i < 3; ++i) {
        num[i] = i;
        pthread_create(&programmers[i], NULL, programmer, (void*)(num + i)) ;
    }
    for (int i = 0; i < 3; ++i) {
        pthread_join(programmers[i], NULL);
    }
    std::cout << "All tasks completed!";
    return 0;
}
