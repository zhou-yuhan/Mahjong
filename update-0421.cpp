/**20200421 Wen-Tianyu Update
 ----------------------------------------
 * 增加lastID()函数返回上家ID（判断是否吃的基础）
 * 增加Out()函数返回上一回合打出的牌名（能确保只有上一回合打出过牌时才会被调用）
 * Input()函数加入统计明牌部分
 * Act()函数进行了处理其他输入的补充
 ----------------------------------------
 *下一步的计划：
 *写一下抢杠胡的大框架（无AI部分）
 *定义容器存放己方的吃、碰、明杠、暗杠牌，并实现能把牌加入这些容器
 */

#include <iostream>
#include <string>
#include <sstream>
#include <vector>
#include <algorithm>
#include <map>
#include <unordered_map>

using namespace std;

//its=int to string
string its[34] = { "W1","W2","W3","W4","W5","W6","W7","W8","W9",
                   "B1","B2","B3","B4","B5","B6","B7","B8","B9",
                   "T1","T2","T3","T4","T5","T6","T7","T8","T9",
                   "F1","F2","F3","F4","J1","J2","J3" };

//sti=string to int (Input函数前两行完成初始化)
map<string, int> sti;

/**
 * @brief
 * 全局变量，储存基本牌面信息
 * @param
 * stmp --> string_temp
 * itmp --> int_temp
 * request, response  该回合之前所有信息
 * hand 手牌
 * ostringstream istringstream 用于字符串和其他数据类型之间方便转换
 * myPlayerID 应该在吃牌的时候有用
 * quan 不知道有啥用
 * lastout 储存上一回合打出了什么牌
*/
int turnID;
string stmp;
int itmp;
int idtmp;
vector<string> request, response;
vector<string> hand;
ostringstream sout;
istringstream sin;
string lastout;
int myPlayerID, quan;

int lastID() {
    return (myPlayerID + 1) % 4;
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

//返回该回合打出的牌名
string Out(int k) {
    istringstream ssin;
    ssin.str(request[k]);
    int iitmp;
    int iidtmp;
    string sstmp;
    ssin >> iitmp;
    ssin >> iidtmp;
    ssin >> sstmp;
    if (sstmp == "CHI")
        ssin >> sstmp;
    ssin >> sstmp;
    return sstmp;
}

/**
 * @brief
 * 输入到本回合为止所有信息
*/
void Input()
{
    for (int i = 0; i < 34; ++i)
        sti[its[i]] = i;
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
    //统计明牌
    for (int i = 0; i <= turnID; ++i) {
        sin.clear();
        sin.str(request[i]);
        sin >> itmp;
        if (itmp == 3) {
            sin >> idtmp;
            sin >> stmp;
            string sstmp;//读牌名
            if (stmp == "PLAY") {
                sin >> sstmp;
                AddKnown(sstmp);
            }
            else if (stmp == "PENG") {
                sin >> sstmp;
                AddKnown(sstmp);
                AddKnown(Out(i - 1));
                AddKnown(Out(i - 1));
            }
            else if (stmp == "CHI") {
                sin >> sstmp;
                //为了避免把上回合打出的牌重复加入Known中，暂时先分三种情况处理一下...
                if (Out(i - 1) == its[sti[sstmp] - 1]) {
                    AddKnown(sstmp);
                    AddKnown(its[sti[sstmp] + 1]);
                }
                else if (Out(i - 1) == its[sti[sstmp] + 1]) {
                    AddKnown(its[sti[sstmp] - 1]);
                    AddKnown(sstmp);
                }
                else if (Out(i - 1) == sstmp) {
                    AddKnown(its[sti[sstmp] - 1]);
                    AddKnown(its[sti[sstmp] + 1]);
                }
                sin >> sstmp;
                AddKnown(sstmp);
            }
            else if (stmp == "BUGANG") {
                sin >> sstmp;
                AddKnown(sstmp);
            }
        }
        if (i == turnID)
            break;
        sin.clear();
        sin.str(response[i]);
        sin >> stmp;
        //如果处理request里的GANG语句无法快捷地分辨是明杠还是暗杠，先出此下策处理response里的GANG语句
        if (stmp == "GANG") {
            string LastOne = Out(i);
            AddKnown(LastOne);
            AddKnown(LastOne);
            AddKnown(LastOne);
            AddKnown(LastOne);
        }
    }
}

/**
 * @brief
 * 处理之前回合的信息
*/
void PreviousInfo()
{
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
        for (int j = 0; j < 5; j++) sin >> itmp; // 编号 四个玩家花牌数
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
            /**
             * 可以在此处理其他编号输入
             * 储存其他玩家打过的牌、杠、碰、吃的牌
            */
        }
        sin.clear();
    }
}

/**
 * @brief
 * 做出决策
 * 决策取决于本回合输入是什么(编号为2/3)
*/
void Act()
{
    sin.clear();
    if (turnID < 2) {
        sout << "PASS";
    }
    else {
        sin.str(request[turnID]); // 本回合输入信息
        sin >> itmp;
        /**
         * 考虑把以下决策内容封装为函数 Draw() Peng() Chi()...
         * 目前功能简单，先这么写吧
        */
        if (itmp == 2) { // 摸牌
            sin >> stmp;
            hand.push_back(stmp);
            /**
             * 此处添加算法
             * 暂时是傻傻的随机出牌
            */
            random_shuffle(hand.begin(), hand.end());
            sout << "PLAY " << *hand.rbegin();
            hand.pop_back();
        }
        else {
            sin >> idtmp;
            sin >> stmp;
            //下面一行是必定会PASS的情况（补花、摸牌、杠或request来自自己）
            if (idtmp == myPlayerID || stmp == "BUHUA" || stmp == "DRAW" || stmp == "GANG")
                sout << "PASS";
            else {
                if (stmp == "BUGANG") {
                    //判断是否能够抢杠胡、是否抢杠胡
                }
                string ThisOut = Out(turnID);
                if (idtmp == lastID()) {
                    //判断是否能吃和是否要吃
                }
                //判断是否能碰、杠、胡和是否要胡
            }
        }
    }
    response.push_back(sout.str());
    cout << response[turnID] << endl;
}

int main()
{
    Input();
    PreviousInfo();
    Act();
    system("pause");
    return 0;
}
