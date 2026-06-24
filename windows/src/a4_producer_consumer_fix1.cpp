#include <windows.h>
#include <iostream>

const unsigned short SIZE_OF_BUFFER = 2;
unsigned short ProductID = 0;
unsigned short ConsumeID = 0;
unsigned short in = 0;
unsigned short out = 0;

int buffer[SIZE_OF_BUFFER] = {0};
bool p_ccontinue = true;
HANDLE Mutex;
HANDLE FullSemaphore;
HANDLE EmptySemaphore;

DWORD WINAPI Producer(LPVOID);
DWORD WINAPI Consumer(LPVOID);

void Produce() {
    std::cout << std::endl << "Producing " << ++ProductID << " ... ";
    std::cout << "Succeed" << std::endl;
}

void Append() {
    std::cerr << "Appending a product ... ";
    buffer[in] = ProductID;
    in = (in + 1) % SIZE_OF_BUFFER;
    std::cerr << "Succeed" << std::endl;

    for (int i = 0; i < SIZE_OF_BUFFER; ++i) {
        std::cout << i << ": " << buffer[i];
        if (i == in) {
            std::cout << " <-- 生产";
        }
        if (i == out) {
            std::cout << " <-- 消费";
        }
        std::cout << std::endl;
    }
}

void Take() {
    std::cerr << "Taking a product ... ";
    ConsumeID = buffer[out];
    buffer[out] = 0;
    out = (out + 1) % SIZE_OF_BUFFER;
    std::cerr << "Succeed" << std::endl;

    for (int i = 0; i < SIZE_OF_BUFFER; ++i) {
        std::cout << i << ": " << buffer[i];
        if (i == in) {
            std::cout << " <-- 生产";
        }
        if (i == out) {
            std::cout << " <-- 消费";
        }
        std::cout << std::endl;
    }
}

void Consume() {
    std::cout << "Consuming " << ConsumeID << " ... ";
    std::cout << "Succeed" << std::endl;
}

DWORD WINAPI Producer(LPVOID lpPara) {
    (void)lpPara;

    while (p_ccontinue) {
        WaitForSingleObject(EmptySemaphore, INFINITE);
        WaitForSingleObject(Mutex, INFINITE);
        Produce();
        Append();
        Sleep(1500);
        ReleaseMutex(Mutex);
        ReleaseSemaphore(FullSemaphore, 1, NULL);
    }

    return 0;
}

DWORD WINAPI Consumer(LPVOID lpPara) {
    (void)lpPara;

    while (p_ccontinue) {
        WaitForSingleObject(FullSemaphore, INFINITE);
        WaitForSingleObject(Mutex, INFINITE);
        Take();
        Consume();
        Sleep(1500);
        ReleaseMutex(Mutex);
        ReleaseSemaphore(EmptySemaphore, 1, NULL);
    }

    return 0;
}

int main() {
    Mutex = CreateMutex(NULL, FALSE, NULL);
    EmptySemaphore = CreateSemaphore(NULL, SIZE_OF_BUFFER, SIZE_OF_BUFFER, NULL);
    FullSemaphore = CreateSemaphore(NULL, 0, SIZE_OF_BUFFER, NULL);

    /* 修改 1：让消费者数量大于生产者数量。 */
    const unsigned short PRODUCERS_COUNT = 1;
    const unsigned short CONSUMERS_COUNT = 3;
    const unsigned short THREADS_COUNT = PRODUCERS_COUNT + CONSUMERS_COUNT;

    HANDLE hThreads[THREADS_COUNT];
    DWORD producerID[PRODUCERS_COUNT];
    DWORD consumerID[CONSUMERS_COUNT];

    for (unsigned short i = 0; i < PRODUCERS_COUNT; ++i) {
        hThreads[i] = CreateThread(NULL, 0, Producer, NULL, 0, &producerID[i]);
        if (hThreads[i] == NULL) {
            return -1;
        }
    }

    for (unsigned short i = 0; i < CONSUMERS_COUNT; ++i) {
        hThreads[PRODUCERS_COUNT + i] =
            CreateThread(NULL, 0, Consumer, NULL, 0, &consumerID[i]);
        if (hThreads[PRODUCERS_COUNT + i] == NULL) {
            return -1;
        }
    }

    std::cout << "Press Enter to stop..." << std::endl;
    while (p_ccontinue) {
        if (std::cin.get()) {
            p_ccontinue = false;
        }
    }

    return 0;
}
