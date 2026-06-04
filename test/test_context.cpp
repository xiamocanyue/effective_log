#include "../context/context.h"

#include <iostream>

int main() {
    auto worker = ADD_TASK_RUNNER(1);

    POST_TASK(worker, []() {
        std::cout << "task A" << std::endl;
    });

    POST_TASK(worker, []() {
        std::cout << "task B" << std::endl;
    });

    WAIT_TASK_IDLE(worker);

    auto ret = POST_AND_RETURN(worker, []() -> std::string {
        return "done";
    });

    if (ret) {
        std::cout << ret->get() << std::endl;
    }

    auto repeat_id = POST_REPEATED_TASK(
        2,
        std::chrono::seconds(2),
        worker,
        []() {
            std::cout << "heartbeat" << std::endl;
        }
    );

    std::cout << "repeat id: " << repeat_id << std::endl;
    std::this_thread::sleep_for(std::chrono::seconds(5));
}