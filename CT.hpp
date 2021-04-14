//
// Created by Hyunsik Yoon on 2021-04-12.
//

#ifndef MYMETHOD_CT_HPP
#define MYMETHOD_CT_HPP
#include <vector>
#include <iostream>
#include <limits>

using namespace std;

template<typename K>
class CTIndex{
private:
    struct Segment;                 //세그먼트 구조체
    size_t n;                       //인덱싱된 전체 데이터 개수
    size_t epsilon;                 //허용 오차
    vector<Segment> segments;       //세그먼트들
    //vector<size_t> levelSegNum;   //레벨별 세그먼트 수(segment 벡터에서 구분을 위함)
    vector<size_t> levelOffsets;   //레벨별 세그먼트의 시작점(segment 벡터에서 구분을 위함)

    template<typename Iter>
    void shrinkingCone(size_t epsilon, Iter cursor, Iter end
                        ,vector<Segment>& segments, vector<K>& nextData);
public:
    CTIndex(vector<K>& data, size_t epsilon);
};

template<typename K>
CTIndex<K>::CTIndex(vector<K>& data, size_t epsilon){
    auto begin = data.begin();
    auto end = data.end();
    size_t beforeSegNum = 0;

    n = distance(begin, end);

    if (n <= 1){
        cout << "data size is 0 or 1\n";
        /*
        for(auto seg : segments){
            for(auto key : seg.keys){
                cout << key << " ";
            }
            cout << "slope: " << seg.slope << endl << endl;
        }
         */
        return;
    }

    levelOffsets.push_back(0);
    for(;data.size()==1;){
        shrinkingCone(epsilon, begin, end, segments); //레벨 하나 만들기
        data.clear();
        /*방금 만들어진 세그먼트의 첫번째 key를 모으기*/
        for(auto i = levelOffsets.back(); i < segments.size(); i++){
            data.push_back(segments[i].keys[0]);
        }
        /*다음 레벨의 시작점 저장*/
        levelOffsets.push_back(levelOffsets.back());
        begin = data.begin();
        end = data.end();
    }


    levelOffsets.push_back(0); //leaf level의 시작점



}

template<typename K>
struct CTIndex<K>::Segment{
    vector<K> keys;                 //포함된 key들
    double slope = -1;              //기울기

};

/*전체 array의 size가 1일때는 동작하지 않음*/
template<typename K>
template<typename Iter>
void CTIndex<K>::shrinkingCone(size_t epsilon, Iter cursor, Iter end
                                ,vector<Segment>& segments, vector<K>& nextData){
    Segment segment;
    K startingPoint = *cursor;
    segment.keys.push_back(startingPoint);
    //nextData.push_back(startingPoint);
    double UB = numeric_limits<size_t>::max();
    double LB = 0;

    ++cursor;
    double pos = 1;
    for(; cursor != end; ++cursor, ++pos){ //starting point를 0,0으로 생각
        if(LB > ((*cursor)-startingPoint)/pos || ((*cursor)-startingPoint)/pos > UB) {//현재 key가 cone을 벗어나는지 검사

            /*세그먼트 생성 완료*/
            segment.slope = (UB+LB)/2;
            segments.push_back(segment);
            /*세그먼트 변수 초기화*/
            segment.keys.clear();
            segment.slope = -1;
            UB = numeric_limits<size_t>::max();
            LB = 0;
            /*현재 key를 시작으로 새로운 세그먼트 생성 시작*/
            startingPoint = *cursor;
            segment.keys.push_back(startingPoint);
            //nextData.push_back(startingPoint);
            pos = 0;

        } else {
            /*세그먼트에 현재 key 추가*/
            segment.keys.push_back(*cursor);
            /*UB, LB 업데이트*/
            if (pos - epsilon > 0) {
                if (UB > ((*cursor)-startingPoint) / (pos - epsilon)) {
                    UB = ((*cursor)-startingPoint) / (pos - epsilon);
                }
            }
            if (LB < ((*cursor)-startingPoint) / (pos + epsilon)) {
                LB = ((*cursor)-startingPoint) / (pos + epsilon);
            }

        }
    }
    /*마지막 세그먼트 처리*/
    if(!segment.keys.empty()){
        if(segment.keys.size() == 1){
            segment.slope = 0; //첫 번째 key는 원점이므로 pos가 0이면 됨 pos=ax 에서 a=0이면 괜찮을듯?
        }else{
            segment.slope = (UB+LB)/2;
        }
        segments.push_back(segment);
        segment.keys.clear();
        segment.slope = -1;
    }


};

#endif //MYMETHOD_CT_HPP
