#include <iostream>
#include <vector>
#include <algorithm>
#include <fstream>
#include <iterator>
#include <random>
#include "CT.hpp"
#include "zipf.h"

void getSearchKey(std::vector<int>& data, std::vector<int>& output, size_t totalKey, size_t batchSize);
void getZipfSearchKey(std::vector<int>& data, std::vector<int>& output, size_t totalKey, size_t batchSize);

int main() {

/*  std::vector<int> data(100);
    std::generate(data.begin(), data.end(), std::rand);
    *//*RAND_MAX == 32767*//*
    for(auto& i : data)
    {
        i = i%100000;
    }
    std::sort(data.begin(), data.end());*/

    std::vector<int> data;
    cout << data.max_size() << endl;
/*    for(int i = 0; i < 67108867; i++){
        data.push_back(i);
    }
    return 0;*/

    ifstream file("D:\\LearnedIndex\\dataset\\OSM\\osm_800M_int_9digit.csv");
    size_t count = 0;
    while (file.good()) {
        string str;
        getline(file, str);
        try {
            data.push_back(stod(str));
            count++;
        } catch (exception e) {
            break;
        }
        if (count == 100000000) {
            break;
        }
    }
    file.close();
/*    auto offset = data.back();
    for(auto d : data){
        try{
            data.push_back(d+offset);
        }catch(exception e){
            cout << d+offset << endl;
            break;
        }
    }*/
    cout << data[0] << endl;
    cout << data[89897] << endl;
    cout << data[89898] << endl;
    cout << "number of keys: " << data.size() << endl;

    /* bulk loading 할 key random으로 고르기 - 나머지 key는 insert를 위한 key*/
    /* bulk loading 된 key 중에서 lookup할 key 고르기 (zipf)*/
    std::vector<int> keysForInit;
    std::vector<int> keysForLookup;
    std::vector<int> keysForInsert;
    size_t totalKey = 80000000;

    for (size_t i = 0; i < totalKey; i++) {
        keysForInit.push_back(data[i]);
    }
    // bulk loading 할 key 정렬
    std::sort(keysForInit.begin(), keysForInit.end());
    size_t epsilon = 128;
    CTIndex<int> index(keysForInit, epsilon);

    /* initialize the index using 100M keys (random sampled)
     * perform the following workloads during 60secs.
     *  read only    : sequential lookup for a key
     *  read heavy   : 19 lookup,1 insertion - repeat this cycle
     *  write heavy  : 1 lookup,1 insertion - repeat this cycle
     *  write only   : sequential insertion for a key
     * */

    //data안에 들어있는 key들 랜덤으로 batchSize만큼 선택하기(lookup으로 사용할 Key 선택하는 것)
    getSearchKey(data, keysForLookup, totalKey, 1);
    //getZipfSearchKey(data, keysForLookup, totalKey, 1);

    //insert 한번 할 때 마다 totalKey 증가시켜야함!!!!!!
    //insert 할 Key는 data[totalKey]로 집어넣기기
    cout << index.lookup(320856465) << endl;
    index.insert(323012605);


    return 0;
}

void getZipfSearchKey(std::vector<int>& data, std::vector<int>& output, size_t totalKey, size_t batchSize){
    ScrambledZipfianGenerator zipfGen(totalKey);
    for(size_t i = 0; i < batchSize; i++){
        size_t pos = zipfGen.nextValue();
        output.push_back(data[pos]);
    }
}
void getSearchKey(std::vector<int>& data, std::vector<int>& output, size_t totalKey, size_t batchSize){
    std::mt19937_64 gen(std::random_device{}());
    std::uniform_int_distribution<int> dis(0, totalKey-1);

    for(size_t i = 0; i < batchSize; i++){
        size_t pos = dis(gen);
        output.push_back(data[pos]);
    }
}