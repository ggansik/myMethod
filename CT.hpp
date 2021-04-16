//
// Created by Hyunsik Yoon on 2021-04-12.
//

#ifndef MYMETHOD_CT_HPP
#define MYMETHOD_CT_HPP
#include <vector>
#include <iostream>
#include <limits>
#include <cmath>

using namespace std;

template<typename K>
class CTIndex{
private:
    struct Segment;                 //세그먼트 구조체
    size_t n;                       //인덱싱된 전체 데이터 개수
    size_t m_epsilon;
    vector<Segment> segments;       //세그먼트들
    vector<size_t> levelSegNum;   //레벨별 세그먼트 수(segment 벡터에서 구분을 위함)
    vector<size_t> levelOffsets;   //레벨별 세그먼트의 시작점(segment 벡터에서 구분을 위함)

    void shrinkingCone(size_t epsilon, vector<K>& data
                        ,vector<Segment>& segments);
    size_t segForKey(size_t level, K key, double predict);
public:
    CTIndex(vector<K>& data, size_t epsilon);
    void insert(K key);
    void lookup(K key);
};

template<typename K>
CTIndex<K>::CTIndex(vector<K>& data, size_t epsilon){
    size_t beforeSegNum = 0;
    m_epsilon = epsilon;
    n = data.size();

    if (n <= 1){
        cout << "data size is 0 or 1\n";
        return;
    }

    for(;data.size()!=1;){
        /*이번 레벨의 시작점 저장*/
        levelOffsets.push_back(segments.size());
        cout << "------------------------------------------" << endl;
        cout << "level " << levelOffsets.size()-1 << endl;
        shrinkingCone(epsilon, data, segments); //레벨 하나 만들기
        levelSegNum.push_back(segments.size() - levelOffsets.back());
        for(size_t i = levelOffsets.back(); i < segments.size(); i++){
/*            for(auto key : segments[i].keys){
                cout << key << " ";
            }*/
            cout << i-levelOffsets.back() << " ";
            cout << "num of keys: " << segments[i].keys.size() << " ";
            cout << "slope: " << segments[i].slope << endl;
        }

        data.clear();
        /*방금 만들어진 세그먼트의 첫번째 key를 모으기*/
        for(auto i = levelOffsets.back(); i < segments.size(); i++){
            data.push_back(segments[i].keys[0]);
            //cout << segments[i].keys[0] << " ";
        }
    }
}

template<typename K>
struct CTIndex<K>::Segment{
    vector<K> keys;                 //포함된 key들
    double slope = -1;              //기울기

};

/*전체 array의 size가 1일때는 동작하지 않음*/
template<typename K>
void CTIndex<K>::shrinkingCone(size_t epsilon, vector<K>& data
                                ,vector<Segment>& segments){
    Segment segment;
    K startingPoint = data[0];
    segment.keys.push_back(startingPoint);
    //nextData.push_back(startingPoint);
    double UB = numeric_limits<size_t>::max();
    double LB = 0;

    double pos = 1;
    auto iter = data.begin();
    iter++;
    for(; iter != data.end(); iter++){ //starting point를 0,0으로 생각
        if ((*iter) == (*(iter-1))){
            continue;
        }
        if(LB > pos/((*iter)-startingPoint) || pos/((*iter)-startingPoint) > UB) {//현재 key가 cone을 벗어나는지 검사
            /*세그먼트 생성 완료*/
            segment.slope = (UB+LB)/2;
            segments.push_back(segment);
            /*세그먼트 변수 초기화*/
            segment.keys.clear();
            segment.slope = -1;
            UB = numeric_limits<size_t>::max();
            LB = 0;
            /*현재 key를 시작으로 새로운 세그먼트 생성 시작*/
            startingPoint = *iter;
            segment.keys.push_back(startingPoint);
            //nextData.push_back(startingPoint);
            pos = 0;

        } else {
            /*세그먼트에 현재 key 추가*/
            segment.keys.push_back(*iter);
            /*UB, LB 업데이트*/

            if (UB > (pos+epsilon)/((*iter)-startingPoint)) {
                UB = (pos+epsilon)/((*iter)-startingPoint);
            }

            if (pos - epsilon > 0) {
                if (LB < (pos-epsilon)/((*iter)-startingPoint)) {
                    LB = (pos-epsilon)/((*iter)-startingPoint);
                }
            }

        }
        pos++;
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

template<typename K>
void CTIndex<K>::insert(K key){
    /*삽입 위치 찾기 - lookup 함수 사용*/
    /*삽입 위치가 원본 세그먼트 기준으로 어딘지 알아내서 correction table에 추가*/
    /*correction table참조할 때 현재 segment의 상태를 만들어 놔야 함*/
    /*inserted: [1,2], [5], [7,9] 이런 식으로*/
    /*그러면 7에 삽입이 들어옴-> 7은 삽입된 key이므로 가장 가깝고 작은 원래 key위치 6을 찾아서
     * 6이 원래 어디에 있었는지 봄 -> 이건 6 앞에 애들 보면 됨 [1,2], [5] 총 3개이므로 위치 는 원래 3
     * 그럼 7에 들어온 삽입은 correction table에 3으로 표기하면 됨*/

}
template<typename K>
void CTIndex<K>::lookup(K key){
    /* 트리 타고 내려가기
     * levelOffset 가장 높은 레벨 부터 순회
     */
    double predict = 0;
    for(size_t level = levelOffsets.size(); level >= 0; level--){
        /*전 레벨의 세그먼트가 예측한 현 레벨 세그먼트의 인덱스를 가지고 정확한 위치를 찾음*/
        size_t segIdx = segForKey(level, key, predict) + levelOffsets[level];
        /* 찾은 세그먼트를 가지고 다음 레벨의 세그먼트 인덱스 예측
         * segment의 첫 번째 key가 0,0이 되도록 했으므로 첫 번째 key를 빼 줘야 함*/
        predict = segments[segIdx].slope * (key - segments[segIdx].keys[0]);
    }
    /*원래 세그먼트내 포지션 기준으로 correction table 참조*/
}

template<typename K>
size_t CTIndex<K>::segForKey(size_t level, K key, double predict){
    /* prediction은 해당 레벨에서의 segment번호 0,1,2...
     * prediction이 정수가 아닌 경우 처리
     * 1. 내림
     * 2. 반올림
     * 3. 올림
     * 4. 정수 아닌거에서 epsilon적용한 범위 내 정수들만 탐색
     *
     * nextPredict기준으로 좌우 epsilon씩 segment안에서 key를 포함하는 segment의 index return 0,1,2..
     */
    for (size_t pos = ceil(predict-m_epsilon); pos < predict+m_epsilon; pos++){

    }
}



#endif //MYMETHOD_CT_HPP
