#include <string>
#include <iostream>
#include "./thread_pool.h"

int main() {
    //创建线程池对象
    ThreadPool pool(4);
    
    //创建结果列表
    std::vector<std::future<std::string>> results;

    //启动线程任务
    for (int i = 0; i < 8; ++i) {
        results.emplace_back(
            pool.enqueue([i] {
                std::cout<<"working "<<i<<std::endl;
                std::this_thread::sleep_for(std::chrono::seconds(1));
                std::cout<<"working "<<i<<std::endl;
                return std::string("---thread ") + std::to_string(i) + std::string(" finished.---");
            })
        );
    }

    for (auto && result : results) {
        std::cout << result.get() << ' ';
        std::cout<<std::endl;
    }
    return 0;
}