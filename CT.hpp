//
// Created by Hyunsik Yoon on 2021-04-12.
//

#ifndef MYMETHOD_CT_HPP
#define MYMETHOD_CT_HPP
#include <vector>
#include <iostream>
#include <limits>
#include <cmath>
#include <algorithm>
#include <chrono>
#include <map>

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

    void shrinkingCone(size_t epsilon, vector<K>& data, vector<Segment>& segments);
    size_t exactPosForKey(size_t segIdx, K key, pair<size_t, size_t> searchRange); //for leaf
    size_t exactPosForKey(size_t levOffset, K key, double predict); //for internal node

    pair<size_t, size_t> correction(size_t segIdx, double predict, K key);
    size_t findLeafSeg(K key);
    void insertKey(size_t segIdx, K key);

public:
    CTIndex(vector<K>& data, size_t epsilon);
    void insert(K key);
    size_t lookup(K key);
    void size();
};

/*인덱스 생성*/
template<typename K>
CTIndex<K>::CTIndex(vector<K>& data, size_t epsilon){
    chrono::system_clock::time_point start = chrono::system_clock::now();

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

/*        for(size_t i = levelOffsets.back(); i < segments.size(); i++){
            cout << i-levelOffsets.back() << " ";
            cout << "(" << i << ") ";
            cout << "keys: " << segments[i].keys.size() << "     ";
            cout << "slope: " << segments[i].slope << " ";
            cout << "startAbsPos: " << segments[i].startAbsPos << " ||| ";
            for(auto key : segments[i].keys){
                cout << key << " ";
            }
            cout << endl;
        }*/

        data.clear();
        /*방금 만들어진 세그먼트의 첫번째 key를 모으기*/
        for(auto i = levelOffsets.back(); i < segments.size(); i++){
            data.push_back(segments[i].keys[0]);
            //cout << segments[i].keys[0] << " ";
        }
    }
    chrono::system_clock::time_point end = chrono::system_clock::now();
    chrono::nanoseconds nano = end - start;
    cout << "construction time: " << nano.count() / pow(10,9) << "sec(s)" << endl;
}

template<typename K>
struct CTIndex<K>::Segment{
    size_t startAbsPos = -1;           //각 레벨 array기준 시작 key의 position
    vector<K> keys;                 //포함된 key들
    double slope = -1;              //기울기
    bool dirty = false;
    map<size_t, size_t> corTable;
};

/*전체 array의 size가 1일때는 동작하지 않음*/
template<typename K>
void CTIndex<K>::shrinkingCone(size_t epsilon, vector<K>& data, vector<Segment>& segments){
    size_t absPos = 0;
    Segment segment;
    K startingPoint = data[0];
    segment.keys.push_back(startingPoint);
    segment.startAbsPos = absPos;
    //nextData.push_back(startingPoint);
    double UB = numeric_limits<size_t>::max();
    double LB = 0;

    double pos = 1;
    auto iter = data.begin();
    iter++;
    for(; iter != data.end(); iter++){ //starting point를 0,0으로 생각
        /* 중복값 제거
         * 이거 잘생각해야함!!!!
         */
        if ((*iter) == (*(iter-1))){
            continue;
        }
        absPos++;
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
            segment.startAbsPos = absPos;
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
size_t CTIndex<K>::lookup(K key){
    chrono::steady_clock::time_point start = chrono::steady_clock::now();
    /*제일 작은 key보다 작거나 제일 큰 Key 보다 큰 경우*/
    if (key<segments[0].keys[0] || segments[levelSegNum[0]-1].keys.back() < key){
        cout << "key out of range" << endl;
        return -1;
    }
    auto segIdx = findLeafSeg(key);
    double predict = segments[segIdx].slope * (key - segments[segIdx].keys[0]);

    /* relative position */
    pair<size_t, size_t> searchRange;
    if (segments[segIdx].dirty) {
        searchRange = correction(segIdx, predict, key);
    }else{
        searchRange = make_pair(max((double)0,ceil(predict-m_epsilon)), floor(predict+m_epsilon));
    }
    /* 수정된 위치 기준으로 Local search */
    /* keys 중에서 고르는 것*/
    /* startAbsPos는 전체 array기준으로 위치를 알려줘야 하므로 더하는것*/
    size_t exactPos = exactPosForKey(segIdx,key,searchRange) + segments[segIdx].startAbsPos;
    chrono::steady_clock::time_point end = chrono::steady_clock::now();
    chrono::nanoseconds nano = end - start;
    cout << "lookup time: " << nano.count() << "ns" << endl;
    return exactPos;
}

/*현재 레벨에서의 포지션을 리턴, 전체 segments어레이 아님*/
template<typename K>
size_t CTIndex<K>::exactPosForKey(size_t segIdx, K key, pair<size_t, size_t> searchRange){
    /* prediction은 해당 레벨에서의 segment번호 0,1,2...
     * prediction이 정수가 아닌 경우 처리
     * 1. 내림
     * 2. 반올림
     * 3. 올림
     * 4. 정수 아닌거에서 epsilon적용한 범위 내 정수들만 탐색
     * 이걸 바꿀거면 insertKey도 바꿔야함!!!!
     * predict기준으로 좌우 epsilon씩 segment안에서 key를 포함하는 segment의 index return 0,1,2..
     */

    /* binary search를 하던 뭘 하던 해서 찾기 */
    /* 정수 pos만 보기 위한 조건들 */
    size_t pos = searchRange.first;
    for (; pos <= searchRange.second; pos++){
        /*segment별 첫번째 key를 모아놓는다?*/
        if (segments[segIdx].keys[pos] < key){
            continue;
        } else if (segments[segIdx].keys[pos] == key){
            return pos;
        } else{
            cout << "there is no " << key << endl;
            return 99999999;
        }
    }
}

/*for seg segment.keys[0]를 참조해야 하므로 좀 다르다*/
template<typename K>
size_t CTIndex<K>::exactPosForKey(size_t levOffset, K key, double predict){
    /* prediction은 해당 레벨에서의 segment번호 0,1,2...
     * prediction이 정수가 아닌 경우 처리
     * 1. 내림
     * 2. 반올림
     * 3. 올림
     * 4. 정수 아닌거에서 epsilon적용한 범위 내 정수들만 탐색
     * 이걸 바꿀거면 insertKey도 바꿔야함!!!!
     * predict기준으로 좌우 epsilon씩 segment안에서 key를 포함하는 segment의 index return 0,1,2..
     */

    /*binary search를 하던 뭘 하던 해서 찾기*/
    /*정수 pos만 보기 위한 조건들*/
    size_t pos = max((double)levelOffsets[levOffset], ceil(predict-m_epsilon+levelOffsets[levOffset]));
    size_t UB = min((double)levelOffsets[levOffset+1]-1, predict+m_epsilon+levelOffsets[levOffset]);
    for (; pos <= UB; pos++){
        /*segment별 첫번째 key를 모아놓는다?*/
        if (segments[pos].keys[0] <= key){
            continue;
        } else{
            return pos-1;
        }
    }
    /*for 문 안에서 안나온거면 범위 내 제일 마지막 세그먼트*/
    return pos-1;
}

template<typename K>
void CTIndex<K>::insert(K key){
    chrono::steady_clock::time_point start = chrono::steady_clock::now();
    /* 삽입 위치 찾기 - lookup 함수 사용*/

    /*해당 key가 삽입되어야 하는 segment의 segments 벡터상 idx찾기*/
    size_t segIdx = findLeafSeg(key);

    /* 계속 dirty bit을 쓴다 vs 검사 후 쓴다*/
    /* segIdx를 이용해서 insert하고 CTEntry 생성*/
    insertKey(segIdx, key);
    chrono::steady_clock::time_point end = chrono::steady_clock::now();
    chrono::nanoseconds nano = end - start;
    cout << "insertion time: " << nano.count() << "ns" << endl;
}

template<typename K>
size_t CTIndex<K>::findLeafSeg(K key){
    /* 트리 타고 내려가기
     * levelOffset 가장 높은 레벨 부터 순회
     */
    /* predict는 현 레벨 segment들만 가지고 index를 매긴 것
     * 루트는 그냥 참조하면 됨*/
    size_t segIdx = levelOffsets.back();
    double predict = -1;
    for(int levOffset = levelOffsets.size()-2; levOffset >= 0; levOffset--){
        /* 다음 레벨의 세그먼트 인덱스 예측
        * segment의 첫 번째 key가 0,0이 되도록 했으므로 첫 번째 key를 빼 줘야 함*/
        predict = segments[segIdx].slope * (key - segments[segIdx].keys[0]) + segments[segIdx].startAbsPos;
        /* 전 레벨의 세그먼트가 예측한 현 레벨 세그먼트의 인덱스를 가지고 정확한 위치를 찾음*/
        segIdx = exactPosForKey(levOffset, key, predict);
    }
    return segIdx;
}

template<typename K>
void CTIndex<K>::insertKey(size_t segIdx, K key){

    double predict = segments[segIdx].slope * (key - segments[segIdx].keys[0]);

    pair<size_t, size_t> searchRange;
    if (segments[segIdx].dirty) {
        searchRange = correction(segIdx, predict, key);
    }else{
        searchRange = make_pair(max((double)0,ceil(predict-m_epsilon)),
                min(floor(predict+m_epsilon), (double)segments[segIdx].keys.size()-1));
    }

    size_t insertPos = -1;
    size_t pos = searchRange.first;
    for (; pos <= searchRange.second; pos++){
        /*segment별 첫번째 key를 모아놓는다?*/
        if (segments[segIdx].keys[pos] < key){
            continue;
        } else if (segments[segIdx].keys[pos] == key){ //중복값 처리
            cout << "key duplicated!" << endl;
            return;
        }
        else{
            break;
        }
    }
    /*for 문 안에서 안나온거면 범위 내 제일 마지막 position*/
    insertPos = pos;
    segments[segIdx].keys.insert(segments[segIdx].keys.begin()+insertPos, key);
    /*
    for(size_t i = levelOffsets.front(); i < levelOffsets[1]; i++){
        for(auto key : segments[i].keys){
            cout << key << " ";
        }
        cout << endl;
    }
     */
    /* insertPos(삽입된 상태 기준)가지고 correction table 봐서 correction table entry 생성 */
    /* 삽입된게 하나도 없는 경우에는 안봐도 됨? -> segment.dirty로 판단*/

    if (!segments[segIdx].dirty){
        segments[segIdx].corTable[insertPos] = 1;
        segments[segIdx].dirty = true;
    }else {
        /* binary search로 pos+cumInserts가 insertPos보다 크지 않은 마지막 엔트리 찾기*/
        /* pos = insertPos - cumInsert 에 삽입하면 됨 */
        /* Entry는 pair(pos+cumInserts, cumInserts) */
        /* pos 값 말고 CT의 index로 binary search */
        /* lower bound는 값으로 하는거 아닌가? 흠... */
        size_t cInsPos;
        /* insertPos보다 큰 첫번째 entry를 반환*/
        auto entryIter = lower_bound(segments[segIdx].corTable.begin(), segments[segIdx].corTable.end(), insertPos,
                [](pair<size_t, size_t> entry, size_t value)
                -> bool { return  entry.first + entry.second <= value;});

        /* 전부 다 insertPos 보다 같거나 큰 경우*/
        if (entryIter == segments[segIdx].corTable.begin()){
            cInsPos = insertPos;
        } else{ //insertPos이거나 InsertPos보다 최초로 작은 entry의 cumInsert를 가지고 보정
            entryIter--;
            cInsPos = insertPos - entryIter->second;
        }
        /* 빈 entry 새로 만드는 경우 */
        if (!segments[segIdx].corTable.count(cInsPos)){
            segments[segIdx].corTable[cInsPos] = 0;
            cout << "i'm in!!" << endl;
        }
        /* 업데이트된 엔트리부터 이후 모든 엔트리 +1 */
        for (;entryIter != segments[segIdx].corTable.end();entryIter++){
            (*entryIter).second += 1;
        }
    }
}

template<typename K>
pair<size_t, size_t> CTIndex<K>::correction(size_t segIdx, double predict, K key) {
    /* 상대적 position 사용 */
    /* searchRange안에 insert 숫자 파악
     * [0, predict) 에 발생한 insert는 predict값에 영향을 줌 i_1
     * [predict,predict+eps) 에 발생한 insert는 predict값에 영향 없음 i_2
     * [LB, UB] = [predict-m_epsilon, predict+m_epsilon+i_1+i_2]
     * c_predict = predict+i_1
     *
     * seg.keys[corrected predict] <= key(=query key) 이면  c_predict 포함 오른쪽 서치
     * 아니면 c_predict 왼쪽 서치
     */

    /* predict보다 최초로 크거나 같은 entry 찾기*/
    auto entryIter = lower_bound(segments[segIdx].corTable.begin(), segments[segIdx].corTable.end(), predict,
                                 [](pair<size_t, size_t> entry, size_t value)
                                         -> bool { return  entry.first + entry.second < value;});


    /* predict 보다 최초로 작은 entry의 cumInsert가 leftIns */
    size_t leftIns;
    if (entryIter == segments[segIdx].corTable.begin()){
        leftIns = 0;
    } else{
        leftIns= (--entryIter)->second;
        entryIter++;
    }

    /* predict+eps 보다 최초로 작은 값의 cumInsert-leftIns 하면 rightIns */
    /* 여기서 binary search 또 한번 해야하나....*/
    /* predict부터 시작해서 entry 하나씩 보면서 가기 일단 시도*/

    /* 이게맞나 */
    size_t rightIns = 0;
    for (;entryIter != segments[segIdx].corTable.end(); entryIter++) {
        if(entryIter->first >= predict + m_epsilon){
            break;
        }
        rightIns = (entryIter)->second - leftIns;
    }


    /* 올림을 해야하나 내림을 해야하나 반올림? */
    /* relative position */
    size_t cPredict = round(predict+leftIns);
    if (segments[segIdx].keys[cPredict] <= key) {
        return make_pair(cPredict,
                min(floor(cPredict+m_epsilon+rightIns),(double)segments[segIdx].keys.size()-1));
    } else {
        return make_pair(max((double)0,ceil(predict-m_epsilon)), cPredict);
    }
}

template<typename K>
void CTIndex<K>::size(){
    /* segment
     *      startAbsPos - int
     *      keys        - K 제외!!!!
     *      slope       - double
     *      dirty bit   - bool
     *      corTable    - map<size_t, size_t>
     * levelSegNum
     * levelOffsets
     * m_epsilon
     * n
     */
    size_t total = 0;
    size_t startAbsPosTotal = 0;
    size_t slopeTotal = 0;
    size_t dirtyTotal = 0;
    size_t corTableTotal = 0;
    size_t dataSize = 0;
    size_t numKeyExceptDups = 0;
    for(auto seg : segments){
        startAbsPosTotal += sizeof(seg.startAbsPos);
        slopeTotal += sizeof(seg.slope);
        dirtyTotal += sizeof(seg.dirty);
        if(seg.corTable.size() > 0){
            corTableTotal += sizeof((*seg.corTable.begin())) * seg.corTable.size();
        }
        dataSize += sizeof(seg.keys[0]) * seg.keys.size();
        numKeyExceptDups += seg.keys.size();
        /*for(auto ent : seg.corTable){
            corTableTotal += sizeof(sizeof(ent));
        }*/
    }
    total = startAbsPosTotal
            + slopeTotal
            + dirtyTotal
            + corTableTotal
            + sizeof(levelSegNum)
            + sizeof(levelOffsets)
            + sizeof(m_epsilon)
            + sizeof(n);


    cout << "===========================================================" << endl;
    cout << "index size only    " << endl;
    cout << "total:             " << (double)total/(double)1000000 << "MB(s)" << endl;
    cout << "startAbsPos:       " << startAbsPosTotal << "byte(s)" << endl;
    cout << "slope:             " << slopeTotal << "byte(s)" << endl;
    cout << "dirty:             " << dirtyTotal << "byte(s)" << endl;
    cout << "corTable:          " << corTableTotal << "byte(s)" << endl;
    cout << "levelSegNum:       " << sizeof(levelSegNum) << "byte(s)" << endl;
    cout << "levelOffsets:      " << sizeof(levelOffsets) << "byte(s)" << endl;
    cout << "epsilon:           " << sizeof(m_epsilon) << "byte(s)" << endl;
    cout << "n:                 " << sizeof(n) << "byte(s)" << endl;
    cout << "-----------------------------------------------------------" << endl;
    cout << "data size:         " << (double)dataSize/(double)1000000 << "MB(s)" << endl;
    cout << "# of keys(no dup): " << numKeyExceptDups << endl;
    cout << "-----------------------------------------------------------" << endl;
    cout << "Total size:        " << (double)(dataSize + total)/(double)1000000 << "MB(s)" << endl;
    cout << "===========================================================" << endl;



}
#endif //MYMETHOD_CT_HPP
