/*
 * @Author: Xia Hanyu
 * @Date:   2020-05-05 10:55:15
 * @Last Modified by:   Wen Tianyu
 * @Last Modified time: 2020-05-09 01:46:00
 */
 /**20200509 Wen Tianyu Update
  ----------------------------------------
  * Marking()函数现在有了新功能：
    （1）字牌加权重
    （2）19/28数字牌减权重（减的幅度不同）
    （3）存在同色不同数字牌时加权重（指数倍增加）
    （4）存在同数字不同色牌时加权重（取最小值正比例增加）、相邻牌和相隔牌存在同数字不同色牌是加权重（加的幅度不同，取最小值正比例增加）
    具体的权重参数及细节请在params结构中查阅注释
  * 删除了原LowRate()函数，这个比例太过模糊，意义不大，舍弃
  * 新的比例用BadRate()函数求值，求坏牌占整副牌的比例，坏牌的判定由新的变量BadcardLine（环牌分数线）唯一决定  ----> 注意：本条内参数需要优化设定值
  * 完全更改了IfAct()函数，现在函数有六个参数，参数细节和判定规则见注释                                       ----> 注意：本条内参数需要优化设定值
  * 根据判定规则的修改，AfterDraw()和ActOthers()函数也进行了修改，并完成了对IfAct()函数的调用匹配
  ----------------------------------------
  * 后续优化重点：
  * 下列参数需要优化设定值：
    （1）params结构体中设定的权重（可以参考CSDN上的那篇资料，权重值的设定是重中之重，其他参数都要以此为基准设定）
    （2）BadcardLine坏牌分数线
    （3）IfAct()函数中的判定条件（主要是可接受改变值的设定）
  * 根据番种再考虑一下增加Marking()函数的职能，让评估更加准确
  * 对整副牌还可以怎样进行评估需要一些奇思妙想（现在只有第二最小值、平均值和坏牌比例三个判定量，比较笼统）
  */

#include <iostream>
#include <string>
#include <sstream>
#include <vector>
#include <algorithm>
#include <map>
#include <set>
#include <unordered_map>
#include <limits>
#include <cstring>
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
 * lastout 储存上一回合打出了什么牌
 * its --> int to string
 * sti --> string to int
*/
int turnID;
string stmp;
int itmp;
int idtmp;
vector<string> request, response;
vector<string> hand;
multiset<int> handset;
ostringstream sout;
istringstream sin;
string lastout; // 这个东西好像没用过？？？
int myPlayerID, quan;
int TileLeft[4];
string its[34] = { "W1","W2","W3","W4","W5","W6","W7","W8","W9",
                   "B1","B2","B3","B4","B5","B6","B7","B8","B9",
                   "T1","T2","T3","T4","T5","T6","T7","T8","T9",
                   "F1","F2","F3","F4","J1","J2","J3" };

map<string, int> sti;

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
    int len = hand.size();
    for (int i = 0; i < len; ++i) {
        string card = hand[i];
        handset.insert(NewNum(card));
    }
}

int MyMin(int a, int b) {
    return a < b ? a : b;
}

/**
 * @brief
 * 以下内容用于储存已经出现的牌的出现次数
 * 1. 打出的牌，无论是否被吃、碰，都已不会再变为未知
 * 2. 打出的牌若是被吃、碰、杠，则该顺/刻就永远已知
 * @param
 * known 储存已知牌的哈希表
 * @function
 * AddKnown 将新出现的明牌加入known
*/
unordered_map<string, int> known;
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

/**
 * @brief
 * 用于计算牌面权值的参数
 * @param
 * params 储存参数
*/
struct node
{
    int same;//相同牌权重
    int adjacent;//相邻牌权重
    int interval;//隔牌权重
    int unknown;//未知牌权重
    int zipai;//字牌权重
    int samezp;//相同字牌的权重：因为字牌同数字牌相比不能吃，肯定会损失一部分权重，所以对存在相同牌的字牌权重要增加，相当于放大了有相同牌时的权重增加值
    int num19;//19数字牌权重
    int num28;//28数字牌权重
    int scolordnum;//same color && different number 同色不同数字牌的基础权重（每多一张权重增加一倍）
    int snumdcolor;//same number && different color 同数字不同颜色牌的基础权重（越多权重越高，呈正比例增长,如果相邻、相隔牌也存在同样情况那么权重增加且有系数）
};

node params;

int BadcardLine;//坏牌分数线，低于该分数的牌称之为坏牌

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

    if (x > 60) {
        mark += params.zipai;//字牌的权重
        mark += params.samezp * handset.count(x);//相同字牌的权重
    }
        
    if (x % 10 == 1 || x % 10 == 9)
        mark -= params.num19;//19数字牌的权重
    if (x % 10 == 2 || x % 10 == 8)
        mark -= params.num28;//28数字牌的权重

    if (x < 60) {//如果是数字牌
        int temp1 = params.scolordnum;//同色不同数字牌越多，权重越高，且每增加一种数字权重增加值翻倍
        for (int i = (x / 10) * 10 + 1; i < (x / 10 + 1) * 10; ++i)
            if (handset.count(i) > 0) {
                mark += temp1;
                temp1 *= 2;
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
    }

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
    if (n00 == 2)//(x)(x)的情况
        mark += k00 * params.unknown;
    return mark;
}

//计算牌的第二低分（因为第一低分的牌操作后一定会被打出去）
int MinMark() {
    HandsetInit();
    int Size = hand.size();
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
    int Size = hand.size();
    int TotalMark = 0;
    for (auto i = hand.begin(); i != hand.end(); ++i)
        TotalMark += Marking(*i);
    return TotalMark / Size;
}

//BadRate()返回坏牌占整副牌面的比例
double BadRate() {
    int n = 0;
    for (auto i = hand.begin(); i != hand.end(); ++i)
        if (Marking(*i) < BadcardLine)
            ++n;
    return (double)n / hand.size();
}

vector<string>::iterator CalMinTile() {
    /**
     * @brief
     * 返回最小权值牌的迭代器
    */
    auto result = hand.begin();
    int minMark = numeric_limits<int>::max();
    for (auto i = hand.begin(); i != hand.end(); ++i) {
        int mark = Marking(*i);
        if (mark < minMark) {
            minMark = mark;
            result = i;
        }
    }
    return result;
}

/**
 * @brief
 * 以下内容存储自己鸣牌（吃、碰、杠得到的牌）
 * @param
 * pack 容器，为了与算番库匹配，定义为 vector<pair<string, pair<string, int> > > 详见算番库介绍
 * @function
 * AddPack 添加鸣牌,并自动加入known
 * CalPos 计算玩家ID相对于自己是上家、对家、下家
*/
vector<pair<string, pair<string, int> > > pack;
void AddPack(string TYPE, string MidCard, int from = 0, bool is_BUGANG = false) // is_BUGANG 用于区分暗杠和补杠
{
    pack.push_back(make_pair(TYPE, make_pair(MidCard, from)));
    // 将自己鸣牌加入known, 并把pack从手牌里拿掉
    if (TYPE == "PENG") {
        for (int i = 0; i < 2; ++i) { // 只需要两次，因为该回合request打出的牌已经添加过一次
            AddKnown(MidCard);
            // vector没有find()，很伤心
            for (auto j = hand.begin(); j != hand.end(); ++j) {
                if (*j == MidCard) {
                    hand.erase(j); break;
                }
            }
        }
    }
    else if (TYPE == "CHI") {
        int CHIcard_code = from - 2; // 1 2 3 --> -1 0 1
        for (int i = -1; i <= 1; ++i) {
            if (i != CHIcard_code) {
                AddKnown(its[sti[MidCard] + i]);
                for (auto j = hand.begin(); j != hand.end(); ++j) {
                    if (*j == its[sti[MidCard] + i]) {
                        hand.erase(j); break;
                    }
                }
            }
        }
    }
    else if (TYPE == "GANG") {
        if (is_BUGANG) {
            AddKnown(MidCard);
            for (auto j = hand.begin(); j != hand.end(); ++j) {
                if (*j == MidCard) {
                    hand.erase(j); break;
                }
            }
        }
        else {
            if (from == 0) { // 暗杠
                for (int i = 0; i < 4; ++i) {
                    AddKnown(MidCard);
                    for (auto j = hand.begin(); j != hand.end(); ++j) {
                        if (*j == MidCard) {
                            hand.erase(j); break;
                        }
                    }
                }
            }
            else { // 明杠
                for (int i = 0; i < 3; ++i) {
                    AddKnown(MidCard);
                    for (auto j = hand.begin(); j != hand.end(); ++j) {
                        if (*j == MidCard) {
                            hand.erase(j); break;
                        }
                    }
                }
            }
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
 * 以下内容是算法AI部分，可能需要修改
 * 所有函数，CanXXX()判断能否操作，XXX()执行操作
 * 对于CanXXX()函数，参数一律为 要操作的牌、供牌者ID
 * 对于XXX()函数，返回值一律为string，参数一律为要操作的牌、供牌者ID 便于形式统一以及与之后函数(AfterDraw, ActOthers)对接
*/

// 胡部分
bool CanHU(string winTile, bool isZIMO, bool isGANG) {
    MahjongInit(); // 初始化算番库
    bool isJUEZHANG = (known[winTile] == 3);
    bool isLast = false;
    for (int i = 0; i < 4; ++i) {
        if (TileLeft[i] == 0) {
            isLast = true;
            break;
        }
    }
    vector<pair<int, string> > result = MahjongFanCalculator(
        pack, hand, winTile, 0, isZIMO, isJUEZHANG, isGANG, isLast, myPlayerID, quan
    );
    int FanSum = 0;
    for (auto i = result.begin(); i != result.end(); ++i) {
        FanSum += i->first;
    }
    return FanSum >= 8;
}

// 吃部分
// 1. 吃左牌
bool CanCHILeft(string card, int supplierID) {
    if (supplierID != lastID() || TileLeft[NextID(supplierID)] == 0)
        return false;
    int card_num = NewNum(card);
    return (handset.find(card_num + 1) != handset.end() && handset.find(card_num + 2) != handset.end());
}
string CHILeft(string card, int supplierID = -1) { // 吃的supplierID没用
    int card_num = NewNum(card);
    string mid_card = its[sti[card] + 1];
    AddPack("CHI", mid_card, 1);
    return mid_card;
}

// 2. 吃中间
bool CanCHIMid(string card, int supplierID) {
    if (supplierID != lastID() || TileLeft[NextID(supplierID)] == 0)
        return false;
    int card_num = NewNum(card);
    return (handset.find(card_num - 1) != handset.end() && handset.find(card_num + 1) != handset.end());
}
string CHIMid(string card, int supplierID = -1) {
    int card_num = NewNum(card);
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
    int card_num = NewNum(card);
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
    if (TileLeft[NextID(supplierID)] == 0 && supplierID != myPlayerID) // 自己摸牌杠不受下家牌荒限制
        return false;
    int card_num = NewNum(card);
    return handset.count(card_num) == 3;
}
string GANG(string card, int supplierID) {
    AddPack("GANG", card, CalPos(supplierID));
    return card;
}

// 出牌部分
string PLAY() {
    vector<string>::iterator out = CalMinTile();
    string card = *out;
    hand.erase(out);
    AddKnown(card);
    return card;
}

//确定是否要执行操作
int IfAct(int beforemin, int aftermin,int beforeaverage,int afteraverage ,double beforerate, double afterrate) {//六个参数分别为操作前后的第二最小分值、平均分值和坏牌比例
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

    int BeforeMin = MinMark();
    int BeforeAverage = MarkingAverage();
    double BeforeRate = BadRate();

    if (CanGANG(card, myPlayerID)) {
        int BGM = BeforeMin;//BGM=before GANG min 下述命名方式同理，不再注释
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
        if (IfAct(BGM, AGM, BGA, AGA, BGR, AGR)) {
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
        if (IfAct(BBM, ABM, BBA, ABA, BBR, ABR)) {
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

    //暂时先设定能胡就胡
    if (CanHU(card, false, false))
        return make_pair("HU", make_pair("", ""));

    int BeforeMin = MinMark();
    int BeforeAverage = MarkingAverage();
    double BeforeRate = BadRate();

    enum { CHILeftCompare = 0, CHIMidCompare = 1, CHIRightCompare = 2, PENGCompare = 3, GANGCompare = 4 };
    double compares[5];
    memset(compares, -1, sizeof(compares));

    if (CanCHILeft(card, supplierID)) {
        int BCLM = BeforeMin;
        int BCLA = BeforeAverage;
        double BCLR = BeforeRate;
        int ACLM;
        int ACLA;
        double ACLR;
        for (int i = 0; i < 3; ++i)
            for (auto j = hand.begin(); j != hand.end(); ++j)
                if (*j == its[sti[card] + i]) {
                    hand.erase(j);
                    break;
                }
        ACLM = MinMark();
        ACLA = MarkingAverage();
        ACLR = BadRate();
        for (int i = 0; i < 3; ++i)
            hand.push_back(its[sti[card] + i]);
        if (IfAct(BCLM, ACLM, BCLA, ACLA, BCLR, ACLR))
            compares[CHILeftCompare] = IfAct(BCLM, ACLM, BCLA, ACLA, BCLR, ACLR);
    }

    if (CanCHIMid(card, supplierID)) {
        int BCMM = BeforeMin;
        int BCMA = BeforeAverage;
        double BCMR = BeforeRate;
        int ACMM;
        int ACMA;
        double ACMR;
        for (int i = -1; i < 2; ++i)
            for (auto j = hand.begin(); j != hand.end(); ++j)
                if (*j == its[sti[card] + i]) {
                    hand.erase(j);
                    break;
                }
        ACMM = MinMark();
        ACMA = MarkingAverage();
        ACMR = BadRate();
        for (int i = -1; i < 2; ++i)
            hand.push_back(its[sti[card] + i]);
        if (IfAct(BCMM, ACMM, BCMA, ACMA, BCMR, ACMR))
            compares[CHIMidCompare] = IfAct(BCMM, ACMM, BCMA, ACMA, BCMR, ACMR);
    }

    if (CanCHIRight(card, supplierID)) {
        int BCRM = BeforeMin;
        int BCRA = BeforeAverage;
        double BCRR = BeforeRate;
        int ACRM;
        int ACRA;
        double ACRR;
        for (int i = -2; i < 1; ++i)
            for (auto j = hand.begin(); j != hand.end(); ++j)
                if (*j == its[sti[card] + i]) {
                    hand.erase(j);
                    break;
                }
        ACRM = MinMark();
        ACRA = MarkingAverage();
        ACRR = BadRate();
        for (int i = -2; i < 1; ++i)
            hand.push_back(its[sti[card] + i]);
        if (IfAct(BCRM, ACRM, BCRA, ACRA, BCRR, ACRR))
            compares[CHIRightCompare] = IfAct(BCRM, ACRM, BCRA, ACRA, BCRR, ACRR);
    }

    if (CanPENG(card, supplierID)) {
        int BPM = BeforeMin;
        int BPA = BeforeAverage;
        double BPR = BeforeRate;
        int APM;
        int APA;
        double APR;
        for (int i = 0; i < 3; ++i)
            for (auto j = hand.begin(); j != hand.end(); ++j)
                if (*j == card) {
                    hand.erase(j);
                    break;
                }
        APM = MinMark();
        APA = MarkingAverage();
        APR = BadRate();
        for (int i = 0; i < 3; ++i)
            hand.push_back(card);
        if (IfAct(BPM, APM, BPA, APA, BPR, APR))
            compares[PENGCompare] = IfAct(BPM, APM, BPA, APA, BPR, APR);
    }

    if (CanGANG(card, supplierID)) {
        int BGM = BeforeMin;
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
        if (IfAct(BGM, AGM, BGA, AGA, BGR, AGR))
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
    switch (choice) {
    case CHILeftCompare: return make_pair("CHI", make_pair(CHILeft(card), PLAY()));
    case CHIMidCompare: return make_pair("CHI", make_pair(CHIMid(card), PLAY()));
    case CHIRightCompare: return make_pair("CHI", make_pair(CHIRight(card), PLAY()));
    case PENGCompare:
        PENG(card, supplierID);
        return make_pair("PENG", make_pair("", PLAY()));
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
    */
    for (int i = 0; i <= turnID; ++i) {
        sin.clear();
        string CardName;
        // 1. 所有request出现的明牌
        sin.str(request[i]);
        sin >> itmp;
        if (itmp == 3) {
            sin >> idtmp >> stmp;
            if (stmp == "DRAW")
                TileLeft[idtmp]--;
            if (idtmp == myPlayerID)
                continue;
            if (stmp == "PLAY") {
                sin >> CardName;
                AddKnown(CardName);
            }
            else if (stmp == "PENG") {
                sin >> CardName;
                AddKnown(CardName);
                AddKnown(Out(i - 1).first);
                AddKnown(Out(i - 1).first);
            }
            else if (stmp == "CHI") {
                sin >> CardName;
                // 避免把上回合打出的牌重复加入Known
                string CHIcard = Out(i - 1).first;
                for (int i = -1; i <= 1; ++i) {
                    if (sti[CardName] + i != sti[CHIcard])
                        AddKnown(its[sti[CardName] + i]);
                }
                sin >> CardName;
                AddKnown(CardName);
            }
            else if (stmp == "BUGANG") {
                sin >> CardName;
                AddKnown(CardName);
            }
            else if (stmp == "GANG") {
                // 只处理明杠，因为暗杠也不知道杠的是啥
                if (Out(i - 1).first != "NONE") {
                    for (int j = 0; j < 4; ++j)
                        AddKnown(Out(i - 1).first);
                }
            }
        }
        // 处理response
        if (i == turnID)
            break;
        sin.clear();
        sin.str(response[i]);
        sin >> stmp;
        if (stmp == "PLAY") {
            sin >> CardName;
            AddKnown(CardName);
        }
        else if (stmp == "PENG") {
            sin >> CardName; // 这是碰后打出的牌
            AddKnown(CardName);
            AddPack("PENG", Out(i).first, CalPos(Out(i).second));
        }
        else if (stmp == "GANG") {
            // 1. 暗杠
            if (Out(i).first == "NONE") {
                sin >> CardName;
                AddPack("GANG", CardName, 0); // 算番库说明没有写暗杠第三个参数应该是什么，先写0吧
            }
            else {
                // 2. 明杠
                AddPack("GANG", Out(i).first, CalPos(Out(i).second));
            }
        }
        else if (stmp == "BUGANG") {
            sin >> CardName; // 补杠的牌
            AddPack("GANG", CardName, 0, true);
        }
        else if (stmp == "CHI") {
            sin >> CardName; // 得到的顺子的中间
            AddPack("CHI", CardName, CalSupply(CardName, Out(i).first));
            sin >> CardName; // 打出牌
            AddKnown(CardName);
        }
    }
    // 此时将手牌变为纯数字形式
    HandsetInit();
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

    // 初始化map sti
    for (int i = 0; i < 34; ++i)
        sti[its[i]] = i;
    // 初始化TileLeft
    for (int i = 0; i < 4; ++i) {
        TileLeft[i] = 21; // 34 - 13
    }
    // 初始化手牌
    if (turnID < 2) {
        sin.clear();
        return; // 在之后的回合再处理
    }
    else {
        // 存入初始信息
        sin.str(request[0]); // 第一回合
        sin >> itmp >> myPlayerID >> quan;
        sin.clear();
        sin.str(request[1]); // 第二回合
        sin >> itmp; // 编号
        for (int j = 0; j < 13; j++) { // 手牌
            sin >> stmp;
            hand.push_back(stmp);
        }
        for (int i = 2; i < turnID; i++) { // 一二之后每回合的输入输出(不包括本回合)
            sin.clear();
            sin.str(request[i]);
            sin >> itmp; // 输入指令编号
            if (itmp == 2) { // 摸牌
                sin >> stmp;
                hand.push_back(stmp);
                sin.clear();
                sin.str(response[i]);
                sin >> stmp >> stmp;
                hand.erase(find(hand.begin(), hand.end(), stmp));
            }
        }
        sin.clear();
    }
}

void Act()
{
    /**
     * @brief
     * 做出决策
     * 决策取决于本回合输入是什么(编号为2/3)
    */
    sin.clear();
    if (turnID < 2) {
        sout << "PASS";
    }
    else {
        sin.str(request[turnID]); // 本回合输入信息
        sin >> itmp;
        // 1. 摸牌 -> PLAY GANG BUGANG HU
        if (itmp == 2) {
            sin >> stmp;
            hand.push_back(stmp);
            auto act = AfterDraw(stmp, response[turnID - 1][0] == 'G' || response[turnID - 1][0] == 'B'); // 上回合是否杠牌
            sout << act.first << act.second;
        }
        // 2. itmp == 3 他人操作
        else {
            sin >> idtmp;
            sin >> stmp;
            if (idtmp == myPlayerID || stmp == "BUHUA" || stmp == "DRAW" || stmp == "GANG") {// 补花、摸牌、杠或request来自自己 -> PASS
                sout << "PASS";
            }
            else { // 他人 PLAY PENG CHI BUGANG -> 判断进行操作
                string card; // 除了BUGANG外都会打出一张牌, BUGANG时card是补杠的牌
                pair<string, pair<string, string> > act;// 做出的反应统一用act储存 command, card1, card2
                if (stmp == "PLAY") {//别人打牌
                    sin >> card;
                    act = ActOthers(card, idtmp);
                }
                if (stmp == "BUGANG") {//别人补杠(先默认能抢杠和就抢杠和)
                    sin >> card;
                    if (CanHU(card, false, true))
                        act = make_pair("HU", make_pair("", ""));
                }
                if (stmp == "PENG") { //别人碰牌
                    sin >> card;
                    act = ActOthers(card, idtmp);
                }
                if (stmp == "CHI") { // 别人吃牌
                    sin >> card >> card;
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
