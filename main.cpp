#include <iostream>
#include <vector>
#include <algorithm>
#include "CT.hpp"

int main() {
    std::vector<int> data(1000);
    std::generate(data.begin(), data.end(), std::rand);
    for(auto& i : data)
    {
        i = i%100000;
    }
    std::sort(data.begin(), data.end());
/*    for(auto i : data)
    {
        cout << i << " ";
    }*/

/*
    int temp = 0;
    //
    for(auto& i : data)
    {
        i = temp;
        temp += 1;
    }*/

     /*
    for(auto i : data)
    {
        cout << i << " ";
    }
     */
    size_t epsilon = 3;
    CTIndex<int> index(data, epsilon);
    index.lookup(8888);

    return 0;
}
