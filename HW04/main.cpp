#include <iostream>
#include <pthread.h>
#include <unistd.h>
#include <random>
#include <fstream>
#include <string>
#include <vector>

int result[3][3] = { //таблица для запросов проверки
    -1, -1, -1,
    -1, -1, -1,
    -1, -1, -1
};
pthread_mutex_t mutexGetTask; //мьютекс для корректной работы с очередью заданий
pthread_mutex_t mutexWrite; //мьютекс для корректного вывода
pthread_mutex_t mutexCheckOne; //мьютекс для проверки занятости программиста 1
pthread_mutex_t mutexCheckTwo; //мьютекс для проверки занятости программиста 2
pthread_mutex_t mutexCheckThree; //мьютекс для проверки занятости программиста 3
std::random_device rd;
std::mt19937 mt(rd()); //хороший рандом
bool fileFlag = false; //флаг для вида считывания и вывода
std::vector<std::string> outputStrings = {};

struct task  { //очередь заданий
    int name = 1; //имя задания <=> номер задания
    task *next = nullptr; //указатель на следующий элемент очереди
};

task *head = nullptr; //первый элемент очереди

void checkLock(int p) { //закрыть программиста
    switch (p) {
        case 0:
            pthread_mutex_lock(&mutexCheckOne);
        case 1:
            pthread_mutex_lock(&mutexCheckTwo);
        case 2:
            pthread_mutex_lock(&mutexCheckThree);
    }
}

void checkUnlock(int p) { //открыть программиста
    switch (p) {
        case 0:
            pthread_mutex_unlock(&mutexCheckOne);
        case 1:
            pthread_mutex_unlock(&mutexCheckTwo);
        case 2:
            pthread_mutex_unlock(&mutexCheckThree);
    }
}

void checkCode(int p) { //проверка кода для программиста p
    checkLock(p); //ждём, когда p будет свободен
    for (int i = 0; i < 3; ++i) {
        if (result[i][p] == -2) { //-2 - сигнал, что надо проверить код программиста i
            sleep(1 + (mt() % 2)); //рандомное время проверки кода от 1 до 2 секунд
            result[i][p] = mt() % 2; //рандомный результат удачности проверки 50/50
            if (result[i][p] == 0) { //если неудача
                pthread_mutex_lock(&mutexWrite); //защита для вывода
                std::cout << p + 1 << ": the task for the programmer " << i + 1 << " false!" << '\n';
                if (fileFlag) {
                    outputStrings.push_back(std::to_string(p + 1) + ": the task for the programmer " + std::to_string(i + 1) + " false!\n");
                }
                pthread_mutex_unlock(&mutexWrite);
            } else { //если удача
                pthread_mutex_lock(&mutexWrite); //защита для вывода
                std::cout << p + 1 << ": the task for the programmer " << i + 1 << " true!" << '\n';
                if (fileFlag) {
                    outputStrings.push_back(std::to_string(p + 1) + ": the task for the programmer " + std::to_string(i + 1) + " true!\n");
                }
                pthread_mutex_unlock(&mutexWrite);
            }
        }
    }
    checkUnlock(p); //свободен после проверок
}

void doTask(int p, int whoCheck) { //выполнение задания
    sleep(1 + (mt() % 5)); //время на выполнение от 1 до 5 секунд
    pthread_mutex_lock(&mutexWrite); //защита для вывода
    std::cout << "Trying to do with the programmer " << p + 1 << '\n';
    if (fileFlag) {
        outputStrings.push_back("Trying to do with the programmer " + std::to_string(p + 1) + "\n");
    }
    pthread_mutex_unlock(&mutexWrite);
    checkUnlock(p); //код отправлен на проверку, программист свободен и может проверять
    result[p][whoCheck] = -2; //метка для проверки
    checkCode(whoCheck); //проверка кода
    if (result[p][whoCheck] == 0) { //если код не прошёл проверку, то решаем снова
        checkLock(p); //программист занят
        doTask(p, whoCheck); //решаем снова
    }
}

//стартовая функция потоков-программистов
void* programmer(void *param) {
    int p = *(int *)param; //номер потока (программиста)
    while (head != nullptr) { //пока есть хотя бы 1 задача для решения
        checkLock(p); //программист занят решением
        pthread_mutex_lock(&mutexGetTask); //защита для получения задания
        task *taskNow = head; //задание для программиста
        if (head != nullptr) {
            head = head->next; //в очереди больше нет этого задания
        }
        pthread_mutex_unlock(&mutexGetTask);
        if (taskNow == nullptr) { //если задания кончились
            checkUnlock(p); //программист свободен
            break;
        }
        int taskNum = taskNow->name; //имя задания
        pthread_mutex_lock(&mutexWrite); //защита для вывода
        std::cout << "The programmer " << p + 1 << " took the task " << taskNum << '\n';
        if (fileFlag) {
            outputStrings.push_back("The programmer " + std::to_string(p + 1) + " took the task " + std::to_string(taskNum) + "\n");
        }
        pthread_mutex_unlock(&mutexWrite);
        //whoCheck - проверяющий
        doTask(p, (p + (mt() % 2) + 1) % 3); //выбираем рандомный whoCheck != p
        pthread_mutex_lock(&mutexWrite); //защита для вывода
        std::cout << "Task " << taskNum << " completed!" << '\n';
        if (fileFlag) {
            outputStrings.push_back("Task " + std::to_string(taskNum) + " completed!\n");
        }
        pthread_mutex_unlock(&mutexWrite);
        delete taskNow; //задание выполнено => его можно удалить
    }
    return nullptr;
}

int main(int argc, char *argv[]) {
    std::string fileOut;
    int n; //кол-во задач
    if (argc <= 1) {
        std::cin >> n;
    } else if (argv[1][0] == 'g') {
        n = mt() % 21;
        if (argc > 2) {
            fileOut = argv[2];
            fileFlag = true;
        }
    } else if (argv[1][0] == 'f') {
        std::string fileIn = argv[2];
        std::ifstream input(fileIn);
        fileOut = argv[3];
        fileFlag = true;
        input >> n;
        input.close();
    } else {
        n = atoi(argv[1]);
    }
    if (n > 0) {
        head = new task; //первое задание в очереди
    }
    task *last = head; //последний в очереди
    for (int i = 1; i < n; ++i) {
        task *now = new task; //новое задание
        now->name = i + 1; //имя = номер
        last->next = now; //обновляем очередь
        last = now;
    }
    pthread_t programmers[3]; //дочерние потоки-программисты
    int num[3]; //номера для потоков
    for (int i = 0; i < 3; ++i) {
        num[i] = i;
        pthread_create(&programmers[i], NULL, programmer, (void*)(num + i)); //создание дочерних потоков
    }
    for (int i = 0; i < 3; ++i) { //ожидание завершения дочерних потоков
        pthread_join(programmers[i], NULL);
    }
    std::cout << "All tasks completed!" << '\n'; //конец
    if (fileFlag) {
        std::ofstream output(fileOut);
        for (int i = 0; i < outputStrings.size(); ++i) {
            output << outputStrings[i];
        }
        output << "All tasks completed!" << '\n';
        output.close();
    }
    return 0;
}
