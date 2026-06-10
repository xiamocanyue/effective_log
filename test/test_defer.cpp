#include <iostream>
#include "defer.h"

class Test {
public:
    Test() {
        arr = new int[10];
    }

    void Close() {
        std::cout << "执行删除" << std::endl;
        delete[] arr;
    }

    void Insert(int index, int num) {
        if (index >= 10) {
            std::cout << " 下标超出  " << std::endl;
            return;
        }
        arr[index] = num;
    }

    void Prin() {
        for (int i = 0; i < 10; i++) {
            std::cout << arr[i] << " ";
        }
        std::cout << std::endl;
    }


private:
    int* arr;
};

int main() {
    Test t;
    DEFER{
        t.Close();
    };

    for (int i = 0; i < 10; i++) {
        t.Insert(i, i + 10);
    }

    t.Prin();
}