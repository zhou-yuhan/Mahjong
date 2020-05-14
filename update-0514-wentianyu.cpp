/*
下一步目标：补全下列番种的函数：
1.一色三节高
2.一色三步高
3.花龙
4.三色三同顺
5.三色三步高
*/

#include <iostream>
#include <string>
#include <vector>
#include <algorithm>
#include <map>
#include <set>
#include <unordered_map>
#include <limits>
#include <cstring>
#include <sstream>
#include "MahjongGB/MahjongGB.h"

using namespace std;

/**
 * @brief
 * 全局变量，储存基本牌面信息
 * @param
 * stmp --> string_temp
 * itmp --> int_temp
 * request, response  该回合之前所有信息
 * hand 手牌
 * handset 储存纯数字化的手牌
 * ostringstream istringstream 用于字符串和其他数据类型之间方便转换
 * myPlayerID 应该在吃牌的时候有用
 * quan 圈风
 * TileLeft[4] 四人剩下牌墙数量
 * TING 是否处于听牌状态
 * its --> int to string
 * sti --> string to int
*/
int turnID;
string stmp;
int itmp;
int idtmp;
vector<string> request, response;
vector<string> hand;
vector<pair<string, pair<string, int> > > pack;
multiset<int> handset;
multiset<int> allhandset;
unordered_map<string, int> known;
ostringstream sout;
istringstream sinn;
int myPlayerID, quan;
int TileLeft[4];
bool TING = false;
string its[34] = { "W1","W2","W3","W4","W5","W6","W7","W8","W9",
                   "B1","B2","B3","B4","B5","B6","B7","B8","B9",
                   "T1","T2","T3","T4","T5","T6","T7","T8","T9",
                   "F1","F2","F3","F4","J1","J2","J3" };

map<string, int> sti;

/**
 * 以下是一些标志，判断牌是否需要向该番形发展
 * 问题在于，不同的番形很可能与现有评估函数不兼容
 * 更重要的是，并没有思路怎么去判断番形朝向哪个发展，以及如何控制发展
 * 而且，会不会因为追求番形而打掉有用的牌最后捡芝麻丢西瓜
 *
 * 好在“全求人”至少6番，要求单调将，所以判断pack很多时可以考虑不保留唯一的对子从而单调将
*/

int NewNum(string card) {
    /**
     * @brief
     * 将牌名转换为数字
     * 用11-19表示W1-W9 31-39表示B1-B9 51-59表示T1-T9 F1=65 F2=70 F3=75 F4=80 J1=85 J2=90 J3=95
    */
    char a1 = card[0];
    char a2 = card[1];
    if (a1 == 'W')
        return a2 - '0' + 10;
    if (a1 == 'B')
        return a2 - '0' + 30;
    if (a1 == 'T')
        return a2 - '0' + 50;
    if (a1 == 'F')
        return (a2 - '0') * 5 + 60;
    return (a2 - '0') * 5 + 80;
}

//用来把hand里的string转化成handset里的int储存起来方便评估每张牌的权重
void HandsetInit() {
    handset.clear();
    int len = hand.size();
    for (int i = 0; i < len; ++i) {
        string card = hand[i];
        handset.insert(NewNum(card));
    }
}

int MyMin(int a, int b) {
    return a < b ? a : b;
}

void AddKnown(string card)
{
    auto it = known.find(card);
    if (it == known.end()) {
        known[card] = 1;
    }
    else {
        known[card]++;
    }
}

void AddPack(string TYPE, string MidCard, int from = 0, bool is_BUGANG = false) // is_BUGANG 用于区分暗杠和补杠
{
    pack.push_back(make_pair(TYPE, make_pair(MidCard, from)));
    // 将自己鸣牌加入known, 并把pack从手牌里拿掉
    if (TYPE == "PENG") {
        for (int i = 0; i < 2; ++i) { // 只需要两次，因为该回合request打出的牌已经添加过一次
            AddKnown(MidCard);
            hand.erase(find(hand.begin(), hand.end(), MidCard));
        }
    }
    else if (TYPE == "CHI") {
        int CHIcard_code = from - 2; // 1 2 3 --> -1 0 1
        for (int i = -1; i <= 1; ++i) {
            if (i != CHIcard_code) {
                AddKnown(its[sti[MidCard] + i]);
                hand.erase(find(hand.begin(), hand.end(), its[sti[MidCard] + i]));
            }
        }
    }
    else if (TYPE == "GANG") {
        if (is_BUGANG) {
            AddKnown(MidCard);
            hand.erase(find(hand.begin(), hand.end(), MidCard));
        }
        else {
            if (from == 0) { // 暗杠
                for (int i = 0; i < 4; ++i) {
                    AddKnown(MidCard);
                    hand.erase(find(hand.begin(), hand.end(), MidCard));
                }
            }
            else { // 明杠
                for (int i = 0; i < 3; ++i) {
                    AddKnown(MidCard);
                    hand.erase(find(hand.begin(), hand.end(), MidCard));
                }
            }
        }
    }
}

void AllHandsetInit() {
    for (auto i = handset.begin(); i != handset.end(); ++i) {
        int Card_Num = *i;
        allhandset.insert(Card_Num);
    }
    for (auto i = pack.begin(); i != pack.end(); ++i) {
        if (i->first == "CHI") {
            string mid_card = (i->second).first;
            allhandset.insert(NewNum(mid_card));
            allhandset.insert(NewNum(mid_card) - 1);
            allhandset.insert(NewNum(mid_card) + 1);
        }
        if (i->first == "PENG") {
            string mid_card = (i->second).first;
            for (int j = 0; j < 3; ++j)
                allhandset.insert(NewNum(mid_card));
        }
        if (i->first == "GANG") {
            string mid_card = (i->second).first;
            for (int j = 0; j < 4; ++j)
                allhandset.insert(NewNum(mid_card));
        }
    }
}

inline int CalPos(int x)
{
    if (x == myPlayerID)
        return 0;
    int previous = myPlayerID > 0 ? myPlayerID - 1 : 3;
    int opposite = (myPlayerID + 2) % 4;
    int next = (myPlayerID + 1) % 4;
    if (x == previous) {
        return 1;
    }
    else if (x == opposite) {
        return 2;
    }
    else if (x == next) {
        return 3;
    }

    //备选返回值没用
    return -1;
}

inline int CalSupply(string MidCard, string SupplyCard)
{
    return 2 - (sti[MidCard] - sti[SupplyCard]);
}

inline int lastID()
{
    /**
     * @brief
     * 返回上家ID
    */
    return myPlayerID > 0 ? myPlayerID - 1 : 3;
}

inline int NextID(int x) {
    /**
     * @brief
     * 返回x玩家下家id
    */
    return (x + 1) % 4;
}

pair<string, int> Out(int k)
{
    /**
     * @brief
     * 返回k回合打出的牌，如果没有打出，返回NONE
     * 同时也要返回打牌玩家ID, 用于鸣牌时加入
    */
    istringstream ssin;
    int InfoNum;
    int playerID = -1;
    string command;
    string CardName = "NONE";

    ssin.str(request[k]);
    ssin >> InfoNum;
    ssin >> playerID;
    if (InfoNum >= 3) {
        ssin >> command;
        if (command == "PLAY" || command == "PENG") {
            ssin >> CardName;
        }
        else if (command == "CHI") {
            ssin >> CardName >> CardName;
        }
    }
    return make_pair(CardName, playerID);
}

/**
 * @brief
 * 用于计算牌面权值的参数
 * @param
 * params 储存参数
*/

struct node {
    int same = 10;     //相同牌权重
    int adjacent = 8;  //相邻牌权重
    int interval = 6;  //隔牌权重
    int unknown = 2;   //未知牌权重
    int zipai = 2;     //字牌权重
    int samezp = 5;  //相同字牌的权重：因为字牌同数字牌相比不能吃，肯定会损失一部分权重，所以对存在相同牌的字牌权重要增加，相当于放大了有相同牌时的权重增加值
    int num19 = 3;       // 19数字牌权重
    int num28 = 2;       // 28数字牌权重
    int scolordnum = 2;  // same color && different number
                         // 同色不同数字牌的基础权重（每多一张权重增加一倍）
    int snumdcolor = 2;  // same number && different color
                         // 同数字不同颜色牌的基础权重（越多权重越高，呈正比例增长,如果相邻、相隔牌也存在同样情况那么权重增加且有系数）
    int onlycouple = 100;  //保留将牌
    int BigFan = 100;//特殊番种的牌型里的牌加特大权重
    int threshold = 0;
};

node params;

int BadcardLine = 10;  //坏牌分数线，低于该分数的牌称之为坏牌

//返回对子数
int CoupleNum() {
    HandsetInit();
    int cnt = 0;
    for (int i = 11; i < 20; ++i)
        if (handset.count(i) == 2)
            ++cnt;
    for (int i = 31; i < 40; ++i)
        if (handset.count(i) == 2)
            ++cnt;
    for (int i = 51; i < 60; ++i)
        if (handset.count(i) == 2)
            ++cnt;
    for (int i = 65; i < 100; i += 5)
        if (handset.count(i) == 2)
            ++cnt;
    return cnt;
}

//返回暗刻和暗杠数
int KeNum() {
    HandsetInit();
    int cnt = 0;
    for (int i = 11; i < 20; ++i)
        if (handset.count(i) >= 3)
            ++cnt;
    for (int i = 31; i < 40; ++i)
        if (handset.count(i) >= 3)
            ++cnt;
    for (int i = 51; i < 60; ++i)
        if (handset.count(i) >= 3)
            ++cnt;
    for (int i = 65; i < 100; i += 5)
        if (handset.count(i) >= 3)
            ++cnt;
    return cnt;
}

// 全求人 
bool MELDED_HAND = false;
void MeldedHand() {
    if (pack.size() >= 3)
        MELDED_HAND = true;
}

//大三元
bool Big_Three_Dragons() {
    int All_Num = allhandset.size();
    int Num = 0;
    for (auto i = allhandset.begin(); i != allhandset.end(); ++i)
        if (*i > 80)
            ++Num;
    int Others_Num = All_Num - Num;
    return Others_Num <= 5;
}

//小三元
bool Little_Three_Dragons() {
    int All_Num = allhandset.size();
    int Num = 0;
    for (auto i = allhandset.begin(); i != allhandset.end(); ++i)
        if (*i > 80)
            ++Num;
    int Others_Num = All_Num - Num;
    return Others_Num <= 6;
}

//大四喜
bool Big_Four_Winds() {
    int All_Num = allhandset.size();
    int Num = 0;
    for (auto i = allhandset.begin(); i != allhandset.end(); ++i)
        if (*i > 60 && *i <= 80)
            ++Num;
    int Others_Num = All_Num - Num;
    return Others_Num <= 2;
}

//小四喜
bool Little_Four_Winds() {
    int All_Num = allhandset.size();
    int Num = 0;
    for (auto i = allhandset.begin(); i != allhandset.end(); ++i)
        if (*i > 60 && *i <= 80)
            ++Num;
    int Others_Num = All_Num - Num;
    return Others_Num <= 3;
}

//四暗刻
bool Four_Concealed_Pungs() {
    return KeNum() == 4;
}

//三暗刻
bool Three_Concealed_Pungs() {
    return KeNum() == 3;
}

//双暗刻
bool Two_Concealed_Pungs() {
    return KeNum() == 2;
}

//一色四同顺
bool Quadruple_Chow(int x) {
    int num20 = allhandset.count(x - 2);
    int num10 = allhandset.count(x - 1);
    int num00 = allhandset.count(x);
    int num01 = allhandset.count(x + 1);
    int num02 = allhandset.count(x + 2);
    if (num20 == 4 && num10 == 4 && num00 == 4)
        return true;
    if (num10 == 4 && num00 == 4 && num01 == 4)
        return true;
    if (num00 == 4 && num01 == 4 && num02 == 4)
        return true;
    return false;
}

//一色三同顺
bool Pure_Triple_Chow(int x) {
    int num20 = allhandset.count(x - 2);
    int num10 = allhandset.count(x - 1);
    int num00 = allhandset.count(x);
    int num01 = allhandset.count(x + 1);
    int num02 = allhandset.count(x + 2);
    if (num20 == 3 && num10 == 3 && num00 == 3)
        return true;
    if (num10 == 3 && num00 == 3 && num01 == 3)
        return true;
    if (num00 == 3 && num01 == 3 && num02 == 3)
        return true;
    return false;
}

//三同刻
bool Triple_Pung(int x) {
    int Num1to9 = x % 10;
    int num1 = allhandset.count(10 + Num1to9);
    int num2 = allhandset.count(30 + Num1to9);
    int num3 = allhandset.count(50 + Num1to9);
    return num1 >= 3 && num2 >= 3 && num3 >= 3;
}

//双同刻
bool Mixed_Double_Pung(int x) {
    int Num1to9 = x % 10;
    int num1 = allhandset.count(10 + Num1to9);
    int num2 = allhandset.count(30 + Num1to9);
    int num3 = allhandset.count(50 + Num1to9);
    int flag = (num1 >= 3) + (num2 >= 3) + (num3 >= 3);
    return flag == 2;
}

//双箭刻
bool Two_Dragon_Pungs() {
    int num1 = allhandset.count(85);
    int num2 = allhandset.count(90);
    int num3 = allhandset.count(95);
    int flag = (num1 >= 3) + (num2 >= 3) + (num3 >= 3);
    return flag == 2;
}

//清龙
bool Pure_Straight(int x) {
    int Ten = (x / 10) * 10;
    int num = 0;
    for (int i = 1; i < 10; ++i)
        if (allhandset.count(Ten + i) > 0)
            ++num;
    return num == 9;
}

//给hand里的每张牌评分
int Marking(string card) {
    int x = NewNum(card);
    int mark = 0;

    int n20 = handset.count(x - 2);
    int n10 = handset.count(x - 1);
    int n00 = handset.count(x);
    int n01 = handset.count(x + 1);
    int n02 = handset.count(x + 2);
    mark += n00 * params.same;//相同牌的权重
    mark += (n10 > 0)* params.adjacent;
    mark += (n01 > 0)* params.adjacent;//相邻牌的权重
    mark += (n20 > 0)* params.interval;
    mark += (n02 > 0)* params.interval;//隔牌的权重

    if (Big_Three_Dragons())// 大三元
        mark += params.BigFan;
    if (Little_Three_Dragons())//小三元
        mark += params.BigFan;
    if (Big_Four_Winds())//大四喜
        mark += params.BigFan;
    if (Little_Four_Winds())//小四喜
        mark += params.BigFan;

    if (n00 >= 3 && Two_Concealed_Pungs())//双暗刻
        mark += params.BigFan;
    if (n00 >= 3 && Three_Concealed_Pungs())//三暗刻
        mark += params.BigFan;
    if (n00 >= 3 && Four_Concealed_Pungs())//四暗刻
        mark += params.BigFan;

    if (x > 60) {
        if (known[card] > 2)//要是牌池里已经出现多于两个该字牌了，就赶紧把这个字牌扔出去吧
            return 0;
        mark += params.zipai; // 非圈风字牌的权重
        mark += params.samezp * (handset.count(x) - 1); //经过对战，发现经常有后期保留单个字牌而打掉有用的牌，最后只能单调将字牌或者不能胡的局面，此处减一使得单个字牌赋分不再很高
        if ((x - 65) / 5 == quan) {//圈风刻
            mark += params.zipai;
            mark += params.samezp * (handset.count(x) - 1);
        }
        if (Two_Dragon_Pungs())//双箭刻
                mark += params.BigFan;
    }

    if (x % 10 == 1 || x % 10 == 9)
        mark -= params.num19;//19数字牌的权重
    if (x % 10 == 2 || x % 10 == 8)
        mark -= params.num28;//28数字牌的权重

    if (x < 60) {//如果是数字牌
        if (handset.count(x - 2) > 0 || handset.count(x - 1) > 0 || handset.count(x + 1) > 0 || handset.count(x + 2) > 0) {
            int temp1 = params.scolordnum;//同色不同数字牌越多，权重越高，且每增加一种数字权重增加值翻倍
            for (int i = (x / 10) * 10 + 1; i < (x / 10 + 1) * 10; ++i)
                if (handset.count(i) > 0) {
                    mark += temp1;
                    temp1 *= 2;
                }
        }
        int Num1to9 = x % 10;//同数字不同色牌越多，权重越高，且呈正比例增加(当然，取该数字在不同色下的数量的最小值比较稳妥）
        for (int i = 10; i <= 50; i += 20)
            if (handset.count(i + Num1to9) > 0 && i + Num1to9 != x) {
                mark += MyMin(handset.count(i + Num1to9), handset.count(x)) * params.snumdcolor;
                if (handset.count(i + Num1to9 + 1) > 0 && handset.count(x + 1) > 0)//相邻右牌也存在同数字不同色时，权重增加
                    mark += MyMin(handset.count(i + Num1to9 + 1), handset.count(x + 1)) * params.snumdcolor * 2;
                if (handset.count(i + Num1to9 - 1) > 0 && handset.count(x - 1) > 0)//相邻左牌也存在同数字不同色时，权重增加
                    mark += MyMin(handset.count(i + Num1to9 - 1), handset.count(x - 1)) * params.snumdcolor * 2;
                if (handset.count(i + Num1to9 + 2) > 0 && handset.count(x + 2) > 0)//相邻右牌也存在同数字不同色时，权重增加，但是要比相邻时低一些
                    mark += MyMin(handset.count(i + Num1to9 + 2), handset.count(x + 2)) * params.snumdcolor;
                if (handset.count(i + Num1to9 - 2) > 0 && handset.count(x - 2) > 0)//相邻左牌也存在同数字不同色时，权重增加，但是要比相邻时低一些
                    mark += MyMin(handset.count(i + Num1to9 - 2), handset.count(x - 2)) * params.snumdcolor;
            }
        if (Triple_Pung(x))//三同刻
            mark += params.BigFan;
        if (Mixed_Double_Pung(x))//双同刻
            mark += params.BigFan;
        if (Pure_Straight(x))//清龙
            mark += params.BigFan;
        if(Quadruple_Chow(x))//一色四同顺
            mark += params.BigFan;;
        if (Pure_Triple_Chow(x))//一色三同顺
            mark += params.BigFan;
    }

    if (x > 60) {//如果是字牌
        
    }

    if (handset.count(x) == 2 && CoupleNum() == 1)//保留将牌
        mark += params.onlycouple;

    // 处理还没有的牌的权重
    int k20 = 0;
    if (sti[card] - 2 >= 0) {
        auto it = known.find(its[sti[card] - 2]);
        if (it != known.end())
            k20 = 4 - known[its[sti[card] - 2]];
        else
            k20 = 4;
    }
    int k10 = 0;
    if (sti[card] - 1 >= 0) {
        auto it = known.find(its[sti[card] - 1]);
        if (it != known.end())
            k10 = 4 - known[its[sti[card] - 1]];
        else
            k10 = 4;
    }
    int k00 = 4 - known[card];
    int k02 = 0;
    if (sti[card] + 2 < 27) {
        auto it = known.find(its[sti[card] + 2]);
        if (it != known.end())
            k02 = 4 - known[its[sti[card] + 2]];
        else
            k02 = 4;
    }
    int k01 = 0;
    if (sti[card] + 1 < 27) {
        auto it = known.find(its[sti[card] + 1]);
        if (it != known.end())
            k01 = 4 - known[its[sti[card] + 1]];
        else
            k01 = 4;
    }
    if (n20 > 0 && n10 == 0)//(x-2)(x)的情况
        mark += k10 * params.unknown;
    if (n10 > 0 && n20 == 0)//(x-1)(x)的情况
        mark += (k20 * params.unknown + k01 * params.unknown);
    if (n02 > 0 && n01 == 0)//(x)(x+2)的情况
        mark += k01 * params.unknown;
    if (n01 > 0 && n02 == 0)//(x)(x+1)的情况
        mark += (k02 * params.unknown + k10 * params.unknown);
    mark += k00 * params.unknown;

    return mark;
}

//计算牌的第二低分（因为第一低分的牌操作后一定会被打出去）
int MinMark() {
    HandsetInit();
    AllHandsetInit();
    int minmark = 100000;
    for (auto i = hand.begin(); i != hand.end(); ++i)
        if (Marking(*i) < minmark)
            minmark = Marking(*i);
    int min2mark = 100000;
    for (auto i = hand.begin(); i != hand.end(); ++i)
        if (Marking(*i) < min2mark && Marking(*i) != minmark)
            min2mark = Marking(*i);
    return min2mark;
}

//计算每张牌评分的平均值
int MarkingAverage() {
    HandsetInit();
    AllHandsetInit();
    int Size = hand.size();
    int TotalMark = 0;
    for (auto i = hand.begin(); i != hand.end(); ++i)
        TotalMark += Marking(*i);
    return TotalMark / Size;
}

//BadRate()返回坏牌占整副牌面的比例
double BadRate() {
    HandsetInit();
    AllHandsetInit();
    int n = 0;
    for (auto i = hand.begin(); i != hand.end(); ++i)
        if (Marking(*i) < BadcardLine)
            ++n;
    return (double)n / hand.size();
}

//前期随便碰，后期有至少一个对子能留下时再碰
bool IfPENG() {
    if (turnID < 50 || handset.size()>9)
        return true;
    return CoupleNum() > 1;
}

/**
 * @brief
 * 以下内容是算法AI部分，可能需要修改
 * 所有函数，CanXXX()判断能否操作，XXX()执行操作
 * 对于CanXXX()函数，参数一律为 要操作的牌、供牌者ID
 * 对于XXX()函数，返回值一律为string，参数一律为要操作的牌、供牌者ID 便于形式统一以及与之后函数(AfterDraw, ActOthers)对接
*/

// 胡部分
bool CanHU(string winTile, bool isZIMO, bool isGANG) {
    MahjongInit();  // 初始化算番库
    if (isZIMO) {
        auto i = find(hand.begin(), hand.end(), winTile);
        hand.erase(i);
    }
    bool isJUEZHANG = (known[winTile] == 3);
    bool isLast = (TileLeft[NextID(myPlayerID)] == 0);
    try {
        vector<pair<int, string> > result = MahjongFanCalculator(
            pack, hand, winTile, 0, isZIMO, isJUEZHANG, isGANG, isLast, myPlayerID, quan
        );
        int FanSum = 0;
        for (auto i = result.begin(); i != result.end(); ++i) {
            FanSum += i->first;
        }
        if (isZIMO)
            hand.push_back(winTile);
        return FanSum >= 8;
    }
    catch (const string & error) {
        if (isZIMO)
            hand.push_back(winTile);
        return false;
    }
}

// 听部分
void CanTING() {
    /**
     * @brief
     * 改变TING变量的状态
     * 查找可以听的牌应该查找所有牌，对于known里没有的牌，最应该听但却没有包括
    */
    MahjongInit();  // 初始化算番库
    int TING_num = 0; // 可以听的牌，多于特定数字才决定听牌，减小可以胡的牌全部在别人手里的可能
    for (int i = 0; i < 34; ++i) {
        string card = its[i];
        if (CanHU(card, false, false)) {
            int hand_num = 0;
            for (auto i = hand.begin(); i != hand.end(); ++i) {
                if (*i == card)
                    hand_num++;
            }
            if (known.find(card) != known.end()) {
                TING_num += 4 - known[card] - hand_num; // 要减去手里有的牌，防止自己手持一堆要胡的牌还不知道
            }
            else {
                TING_num += 4 - hand_num;
            }
        }
    }
    int least;
    if (turnID >= 100) // 100回合之后了，快没牌了，就算只有一张也要等
        least = 1;
    else
        least = 2;
    if (TING_num >= least)
        TING = true;
}

// 吃部分
// 1. 吃左牌
bool CanCHILeft(string card, int supplierID) {
    if (supplierID != lastID() || TileLeft[NextID(supplierID)] == 0)
        return false;
    int card_num = NewNum(card);
    return handset.find(card_num + 1) != handset.end() && handset.find(card_num + 2) != handset.end();
}
string CHILeft(string card, int supplierID = -1) { // 吃的supplierID没用
    string mid_card = its[sti[card] + 1];
    AddPack("CHI", mid_card, 1);
    return mid_card;
}

// 2. 吃中间
bool CanCHIMid(string card, int supplierID) {
    if (supplierID != lastID() || TileLeft[NextID(supplierID)] == 0)
        return false;
    int card_num = NewNum(card);
    return handset.find(card_num - 1) != handset.end() && handset.find(card_num + 1) != handset.end();
}
string CHIMid(string card, int supplierID = -1) {
    AddPack("CHI", card, 2);
    return card;
}

// 3. 吃右牌
bool CanCHIRight(string card, int supplierID) {
    if (supplierID != lastID() || TileLeft[NextID(supplierID)] == 0)
        return false;
    int card_num = NewNum(card);
    return handset.find(card_num - 1) != handset.end() && handset.find(card_num - 2) != handset.end();
}
string CHIRight(string card, int supplierID = -1) {
    string mid_card = its[sti[card] - 1];
    AddPack("CHI", mid_card, 3);
    return mid_card;
}

// 碰部分
bool CanPENG(string card, int supplierID) {
    if (TileLeft[NextID(supplierID)] == 0)
        return false;
    int card_num = NewNum(card);
    return handset.count(card_num) == 2;
}
string PENG(string card, int supplierID) {
    AddPack("PENG", card, CalPos(supplierID));
    return card;
}

// 补杠部分
bool CanBUGANG(string card, int supplier = myPlayerID) {
    if (TileLeft[myPlayerID] == 0) // 我以为这里不会有非法操作了，结果刚刚一句真就补杠了最后一张牌
        return false;
    for (auto i = pack.begin(); i != pack.end(); ++i) {
        if (i->first == "PENG" && (*i).second.first == card) {
            return true;
        }
    }
    return false;
}
string BUGANG(string card, int supplierID = myPlayerID) {
    AddPack("GANG", card, 0, true);
    return card;
}

// 杠部分
bool CanGANG(string card, int supplierID) {
    if (TileLeft[myPlayerID] == 0)
        return false;
    if (TileLeft[NextID(supplierID)] == 0 && supplierID != myPlayerID) // 自己摸牌杠不受下家牌荒限制
        return false;
    int card_num = NewNum(card);
    if (supplierID != myPlayerID)
        return handset.count(card_num) == 3;
    else
        return handset.count(card_num) == 4;
}
string GANG(string card, int supplierID) {
    AddPack("GANG", card, CalPos(supplierID));
    return card;
}

// 出牌部分
string PLAY() {
    HandsetInit();
    AllHandsetInit();
    auto result = hand.begin();
    int minMark = numeric_limits<int>::max();
    for (auto i = hand.begin(); i != hand.end(); ++i) {
        int mark = Marking(*i);
        if (mark < minMark) {
            minMark = mark;
            result = i;
        }
    }
    string card = *result;
    hand.erase(result);
    AddKnown(card);
    return card;
}

//确定是否要执行操作
int IfAct(int beforemin, int aftermin, int beforeaverage, int afteraverage, double beforerate, double afterrate) {//六个参数分别为操作前后的第二最小分值、平均分值和坏牌比例
    int flag = 0;
    if (aftermin < beforemin)//操作后第二最小分值更小了，这应该是最不能接受的情况
        ++flag;
    if (beforeaverage - afteraverage < 10)//操作后平均分值降低了，不太能接受，但是考虑到有牌脱离手牌后平均分值降低属于正常情况（连锁效应），所以允许有浮动区间，这个10只是初步设定，需要调整
        ++flag;
    if (afterrate - beforerate > 0.3)//坏牌比例增加了，不太能接受，但是考虑到有牌脱离手牌后坏牌比例上升属于正常情况（连锁效应），所以允许有浮动区间，这个0.3只是初步设定，需要调整
        ++flag;
    if (flag >= 2)//一旦上述三个临界条件均满足，就判定不进行操作，因为后果不可接受
        return 0;
    return 10000 + (beforemin - aftermin) + (beforeaverage - afteraverage) - (afterrate - beforerate) * BadcardLine;//+10000是为了返回一个正数，不影响比较大小
                                                                                                                    //注意：关于怎么把坏牌率的变化值以适当的倍数体现在返回值里需要调整，这里只是预设
}

pair<string, string> AfterDraw(string card, bool isGANG) {
    /**
     * @brief
     * 处理自己摸牌后的操作 --> PLAY GANG(暗) BUGANG HU
     * @param
     * 返回值：pair<string, string> --> command card
     * command 执行的命令
     * card 打出的牌/暗杠的牌
     *
    */

    //暂时先设定能胡就胡
    if (CanHU(card, true, isGANG))
        return make_pair("HU", "");

    //如果听牌就摸到啥打出啥，不再改变牌面
    if (TING) {
        hand.erase(find(hand.begin(), hand.end(), card));
        return make_pair("PLAY ", card);
    }

    HandsetInit();
    int BeforeMin = MinMark();
    int BeforeAverage = MarkingAverage();
    double BeforeRate = BadRate();

    if (CanGANG(card, myPlayerID)) {
        int BGM = BeforeMin; // BGM=before GANG min 下述命名方式同理，不再注释
        int BGA = BeforeAverage;
        double BGR = BeforeRate;
        int AGM;
        int AGA;
        double AGR;
        for (int i = 0; i < 4; ++i)
            for (auto j = hand.begin(); j != hand.end(); ++j)
                if (*j == card) {
                    hand.erase(j);
                    break;
                }
        AGM = MinMark();
        AGA = MarkingAverage();
        AGR = BadRate();
        for (int i = 0; i < 4; ++i)
            hand.push_back(card);
        if (IfAct(BGM, AGM, BGA, AGA, BGR, AGR) >= params.threshold) {
            GANG(card, 0);
            return make_pair("GANG ", card);
        }
    }

    if (CanBUGANG(card, myPlayerID)) {
        int BBM = BeforeMin;
        int BBA = BeforeAverage;
        double BBR = BeforeRate;
        int ABM;
        int ABA;
        double ABR;
        for (auto i = hand.begin(); i != hand.end(); ++i)
            if (*i == card) {
                hand.erase(i);
                break;
            }
        ABM = MinMark();
        ABA = MarkingAverage();
        ABR = BadRate();
        hand.push_back(card);
        if (IfAct(BBM, ABM, BBA, ABA, BBR, ABR) >= params.threshold) {
            BUGANG(card);
            return make_pair("BUGANG ", card);
        }
    }

    // 啥都不干就打出一张牌
    return make_pair("PLAY ", PLAY());
}

pair<string, pair<string, string> > ActOthers(string card, int supplierID) {
    /**
     * @brief
     * 判断怎么处理别人的request
     * @param
     * 返回值：pair<string, pair<string, string> > --> command card1 card2
     * command 执行的命令
     * card1 吃时为中间牌，其余为空 card2 操作后打出的牌，没有则为空
     * card 别人打出的牌
     * supplierID 供牌方ID
    */

    // 暂时先设定能胡就胡
    if (CanHU(card, false, false))
        return make_pair("HU", make_pair("", ""));

    HandsetInit();
    int BeforeMin = MinMark();
    int BeforeAverage = MarkingAverage();
    double BeforeRate = BadRate();

    enum { CHILeftCompare = 0, CHIMidCompare = 1, CHIRightCompare = 2, PENGCompare = 3, GANGCompare = 4 };
    double compares[5];
    for (int i = 0; i < 5; ++i) compares[i] = -1;

    if (CanCHILeft(card, supplierID)) {
        int BCLM = BeforeMin;
        int BCLA = BeforeAverage;
        double BCLR = BeforeRate;
        int ACLM;
        int ACLA;
        double ACLR;
        for (int i = 0; i < 3; ++i) {
            if (i != 0)
                hand.erase(find(hand.begin(), hand.end(), its[sti[card] + i]));
        }
        ACLM = MinMark();
        ACLA = MarkingAverage();
        ACLR = BadRate();
        for (int i = 0; i < 3; ++i) {
            if (i != 0)
                hand.push_back(its[sti[card] + i]);
        }
        if (IfAct(BCLM, ACLM, BCLA, ACLA, BCLR, ACLR) >= params.threshold)
            compares[CHILeftCompare] = IfAct(BCLM, ACLM, BCLA, ACLA, BCLR, ACLR);
    }

    if (CanCHIMid(card, supplierID)) {
        int BCMM = BeforeMin;
        int BCMA = BeforeAverage;
        double BCMR = BeforeRate;
        int ACMM;
        int ACMA;
        double ACMR;
        for (int i = -1; i < 2; ++i) {
            if (i != 0)
                hand.erase(find(hand.begin(), hand.end(), its[sti[card] + i]));
        }
        ACMM = MinMark();
        ACMA = MarkingAverage();
        ACMR = BadRate();
        for (int i = -1; i < 2; ++i) {
            if (i != 0)
                hand.push_back(its[sti[card] + i]);
        }
        if (IfAct(BCMM, ACMM, BCMA, ACMA, BCMR, ACMR) >= params.threshold)
            compares[CHIMidCompare] = IfAct(BCMM, ACMM, BCMA, ACMA, BCMR, ACMR);
    }

    if (CanCHIRight(card, supplierID)) {
        int BCRM = BeforeMin;
        int BCRA = BeforeAverage;
        double BCRR = BeforeRate;
        int ACRM;
        int ACRA;
        double ACRR;
        for (int i = -2; i < 1; ++i) {
            if (i != 0)
                hand.erase(find(hand.begin(), hand.end(), its[sti[card] + i]));
        }
        ACRM = MinMark();
        ACRA = MarkingAverage();
        ACRR = BadRate();
        for (int i = -2; i < 1; ++i) {
            if (i != 0)
                hand.push_back(its[sti[card] + i]);
        }
        if (IfAct(BCRM, ACRM, BCRA, ACRA, BCRR, ACRR) >= params.threshold)
            compares[CHIRightCompare] = IfAct(BCRM, ACRM, BCRA, ACRA, BCRR, ACRR);
    }

    if (CanPENG(card, supplierID) && IfPENG() && !TING) { // 听牌时不要把将给碰了
        int BPM = BeforeMin;
        int BPA = BeforeAverage;
        double BPR = BeforeRate;
        int APM;
        int APA;
        double APR;
        for (int i = 0; i < 2; ++i)
            hand.erase(find(hand.begin(), hand.end(), card));
        APM = MinMark();
        APA = MarkingAverage();
        APR = BadRate();
        for (int i = 0; i < 2; ++i)
            hand.push_back(card);
        if (IfAct(BPM, APM, BPA, APA, BPR, APR) >= params.threshold)
            compares[PENGCompare] = IfAct(BPM, APM, BPA, APA, BPR, APR);
    }

    if (CanGANG(card, supplierID)) {
        int BGM = BeforeMin;
        int BGA = BeforeAverage;
        double BGR = BeforeRate;
        int AGM;
        int AGA;
        double AGR;
        if (supplierID == myPlayerID) { // 自摸杠
            for (int i = 0; i < 4; ++i)
                hand.erase(find(hand.begin(), hand.end(), card));
        }
        else { // 别人供牌
            for (int i = 0; i < 3; ++i)
                hand.erase(find(hand.begin(), hand.end(), card));
        }

        AGM = MinMark();
        AGA = MarkingAverage();
        AGR = BadRate();

        if (supplierID == myPlayerID) { // 自摸杠
            for (int i = 0; i < 4; ++i)
                hand.push_back(card);
        }
        else { // 别人供牌
            for (int i = 0; i < 3; ++i)
                hand.push_back(card);
        }
        if (IfAct(BGM, AGM, BGA, AGA, BGR, AGR) >= params.threshold)
            compares[GANGCompare] = IfAct(BGM, AGM, BGA, AGA, BGR, AGR);
    }

    // 不能鸣牌
    bool doNothing = true;
    for (int i = 0; i < 5; ++i) {
        if (compares[i] != -1) {
            doNothing = false;
            break;
        }
    }
    if (doNothing)
        return make_pair("PASS", make_pair("", ""));

    // 选取最佳选择
    double max = -1;
    int choice = 0;
    for (int i = 0; i < 5; ++i) {
        if (compares[i] > max) {
            max = compares[i];
            choice = i;
        }
    }

    string midCard, outCard;
    switch (choice) {
    case CHILeftCompare:
        midCard = CHILeft(card); outCard = PLAY();
        return make_pair("CHI ", make_pair(midCard + " ", outCard));
    case CHIMidCompare:
        midCard = CHIMid(card); outCard = PLAY();
        return make_pair("CHI ", make_pair(midCard + " ", outCard));
    case CHIRightCompare:
        midCard = CHIRight(card); outCard = PLAY();
        return make_pair("CHI ", make_pair(midCard + " ", outCard));
    case PENGCompare:
        PENG(card, supplierID);
        return make_pair("PENG ", make_pair("", PLAY()));
    case GANGCompare:
        GANG(card, supplierID);
        return make_pair("GANG", make_pair("", ""));
    }

    //下面这个备选返回值没用
    return make_pair("", make_pair("", ""));
}

void ProcessKnown()
{
    /**
     * @brief
     * 可以处理除了HU之外的所有信息
     * 1. 将输入进来的所有信息中的明牌加入known哈希表
     * 包括 别人和自己打出的牌、别人的鸣牌、自己的鸣牌，总之已经被固定
     * 2. 将自己的鸣牌放入鸣牌库
     * 3. 顺便处理摸牌
    */
    for (int i = 0; i <= turnID; ++i) {
        sinn.clear();
        string CardName;
        // 1. 所有request出现的明牌
        sinn.str(request[i]);
        sinn >> itmp;
        if (itmp == 2) { // 摸牌
            sinn >> stmp;
            hand.push_back(stmp);
            TileLeft[myPlayerID]--;
        }
        if (itmp == 3) {
            sinn >> idtmp >> stmp;
            if (idtmp != myPlayerID) { // 自身request除不处理
                if (stmp == "DRAW")
                    TileLeft[idtmp]--;
                if (stmp == "PLAY") {
                    sinn >> CardName;
                    AddKnown(CardName);
                }
                else if (stmp == "PENG") {
                    sinn >> CardName;
                    AddKnown(CardName);
                    AddKnown(Out(i - 1).first);
                    AddKnown(Out(i - 1).first);
                }
                else if (stmp == "CHI") {
                    sinn >> CardName;
                    // 避免把上回合打出的牌重复加入Known
                    string CHIcard = Out(i - 1).first;
                    for (int i = -1; i <= 1; ++i) {
                        if (sti[CardName] + i != sti[CHIcard])
                            AddKnown(its[sti[CardName] + i]);
                    }
                    sinn >> CardName;
                    AddKnown(CardName);
                }
                else if (stmp == "BUGANG") {
                    sinn >> CardName;
                    AddKnown(CardName);
                }
                else if (stmp == "GANG") {
                    // 只处理明杠，因为暗杠也不知道杠的是啥
                    if (Out(i - 1).first != "NONE") {
                        for (int j = 0; j < 4; ++j) AddKnown(Out(i - 1).first);
                    }
                }
            }
        }
        // 处理response
        if (i == turnID)
            break;
        sinn.clear();
        sinn.str(response[i]);
        sinn >> stmp;
        istringstream sssin;
        sssin.str(request[i + 1]);
        int iddtmp;
        string sstmp;
        sssin >> iddtmp >> iddtmp >> sstmp;
        if (stmp == "PLAY") {
            sinn >> CardName;
            hand.erase(find(hand.begin(), hand.end(), CardName));
        }
        else if (stmp == "PENG" && iddtmp == myPlayerID && sstmp == "PENG") {
            sinn >> CardName; // 这是碰后打出的牌
            hand.erase(find(hand.begin(), hand.end(), CardName));
            AddKnown(CardName);
            AddPack("PENG", Out(i).first, CalPos(Out(i).second));
        }
        else if (stmp == "GANG" && iddtmp == myPlayerID && sstmp == "GANG") {
            // 1. 暗杠
            if (Out(i).first == "NONE") {
                sinn >> CardName;
                AddPack("GANG", CardName, 0); // 算番库说明没有写暗杠第三个参数应该是什么，先写0吧
            }
            else {
                // 2. 明杠
                AddPack("GANG", Out(i).first, CalPos(Out(i).second));
            }
        }
        else if (stmp == "BUGANG" && iddtmp == myPlayerID && sstmp == "BUGANG") {
            sinn >> CardName; // 补杠的牌
            AddPack("GANG", CardName, 0, true);
        }
        else if (stmp == "CHI" && iddtmp == myPlayerID && sstmp == "CHI") {
            sinn >> CardName; // 得到的顺子的中间
            AddPack("CHI", CardName, CalSupply(CardName, Out(i).first));
            sinn >> CardName; // 打出牌
            AddKnown(CardName);
            hand.erase(find(hand.begin(), hand.end(), CardName));
        }
    }
    if (turnID >= 40)
        CanTING();
}

void Input()
{
    /**
     * @brief
     * 输入到本回合为止所有信息
    */

    cin >> turnID;
    turnID--;
    getline(cin, stmp);
    // 将之前所有回合输入、输出信息存入
    for (int i = 0; i < turnID; i++) {
        getline(cin, stmp);
        request.push_back(stmp);
        getline(cin, stmp);
        response.push_back(stmp);
    }
    // 本回合输入
    getline(cin, stmp);
    request.push_back(stmp);
}

void Initialize()
{
    /**
     * @brief
     * 初始化工作
    */

    //不求人
    MeldedHand();

    // 初始化参数
    params.same = 10;     //相同牌权重
    params.adjacent = 8;  //相邻牌权重
    params.interval = 6;  //隔牌权重
    params.unknown = 2;   //未知牌权重
    params.zipai = 2;     //字牌权重
    params.samezp = 5;  //相同字牌的权重：因为字牌同数字牌相比不能吃，肯定会损失一部分权重，所以对存在相同牌的字牌权重要增加，相当于放大了有相同牌时的权重增加值
    params.num19 = 3;       // 19数字牌权重
    params.num28 = 2;       // 28数字牌权重
    params.scolordnum = 2;  // same color && different number
                         // 同色不同数字牌的基础权重（每多一张权重增加一倍）
    params.snumdcolor = 2;  // same number && different color
                         // 同数字不同颜色牌的基础权重（越多权重越高，呈正比例增长,如果相邻、相隔牌也存在同样情况那么权重增加且有系数）
    params.onlycouple = MELDED_HAND ? 0 : 100;  //保留将牌
    params.threshold = 0;

    // 初始化map sti
    for (int i = 0; i < 34; ++i)
        sti[its[i]] = i;
    // 初始化TileLeft
    for (int i = 0; i < 4; ++i) {
        TileLeft[i] = 21; // 34 - 13
    }
    // 初始化手牌
    if (turnID < 2) {
        sinn.clear();
        return; // 在之后的回合再处理
    }
    else {
        // 存入初始信息
        sinn.str(request[0]); // 第一回合
        sinn >> itmp >> myPlayerID >> quan;
        sinn.clear();
        sinn.str(request[1]); // 第二回合
        sinn >> itmp; // 编号
        sinn >> itmp >> itmp >> itmp >> itmp; // 花牌数全为0
        for (int j = 0; j < 13; j++) { // 手牌
            sinn >> stmp;
            hand.push_back(stmp);
        }
    }
}

void Act()
{
    /**
     * @brief
     * 做出决策
     * 决策取决于本回合输入是什么(编号为2/3)
    */
    sinn.clear();
    if (turnID < 2) {
        sout << "PASS";
    }
    else {
        sinn.str(request[turnID]); // 本回合输入信息
        sinn >> itmp;
        // 1. 摸牌 -> PLAY GANG BUGANG HU
        if (itmp == 2) {
            sinn >> stmp;
            auto act = AfterDraw(stmp, response[turnID - 1][0] == 'G' || response[turnID - 1][0] == 'B'); // 上回合是否杠牌
            sout << act.first << act.second;
        }
        // 2. itmp == 3 他人操作
        else {
            sinn >> idtmp;
            sinn >> stmp;
            if (idtmp == myPlayerID || stmp == "BUHUA" || stmp == "DRAW" || stmp == "GANG") {// 补花、摸牌、杠或request来自自己 -> PASS
                sout << "PASS";
            }
            else { // 他人 PLAY PENG CHI BUGANG -> 判断进行操作
                string card; // 除了BUGANG外都会打出一张牌, BUGANG时card是补杠的牌
                pair<string, pair<string, string> > act;// 做出的反应统一用act储存 command, card1, card2
                if (stmp == "PLAY") {//别人打牌
                    sinn >> card;
                    act = ActOthers(card, idtmp);
                }
                if (stmp == "BUGANG") {//别人补杠(先默认能抢杠和就抢杠和)
                    sinn >> card;
                    if (CanHU(card, false, false))
                        act = make_pair("HU", make_pair("", ""));
                    else
                        act = make_pair("PASS", make_pair("", ""));
                }
                if (stmp == "PENG") { //别人碰牌
                    sinn >> card;
                    act = ActOthers(card, idtmp);
                }
                if (stmp == "CHI") { // 别人吃牌
                    sinn >> card >> card;
                    act = ActOthers(card, idtmp);
                }
                sout << act.first << act.second.first << act.second.second;
            }
        }
    }
    response.push_back(sout.str());
    cout << response[turnID] << endl;
}

int main()
{
    Input();
    Initialize();
    ProcessKnown();
    Act();
    system("pause");
    return 0;
}
