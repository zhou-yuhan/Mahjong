#include <iostream>
#include <string>
#include <sstream>
#include <vector>
#include <algorithm>
#include<unordered_map>

using namespace std;

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
*/
int turnID;
string stmp;
int itmp;
vector<string> request, response;
vector<string> hand;
ostringstream sout;
istringstream sin;
int myPlayerID, quan;

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
    if(it == known.end()){
        known[card] = 1;
    }else{
        known[card]++;
    }
}

/**
 * @brief
 * 输入到本回合为止所有信息
*/
void Input()
{
    cin >> turnID;
    turnID--;
    getline(cin, stmp);
    // 将之前所有回合输入、输出信息存入
    for(int i = 0; i < turnID; i++) {
        getline(cin, stmp);
        request.push_back(stmp);
        getline(cin, stmp);
        response.push_back(stmp);
    }
    // 本回合输入
    getline(cin, stmp); 
    request.push_back(stmp);
}

/**
 * @brief
 * 处理之前回合的信息
*/
void PreviousInfo()
{
    if(turnID < 2){
        sin.clear();
        return; // 在之后的回合再处理
    }else{
        // 存入初始信息
        sin.str(request[0]); // 第一回合
        sin >> itmp >> myPlayerID >> quan;
        sin.clear(); 
        sin.str(request[1]); // 第二回合
        for(int j = 0; j < 5; j++) sin >> itmp; // 编号 四个玩家花牌数
        for(int j = 0; j < 13; j++) { // 手牌
            sin >> stmp;
            hand.push_back(stmp);
        }
        for(int i = 2; i < turnID; i++){ // 一二之后每回合的输入输出(不包括本回合)
            sin.clear();
            sin.str(request[i]);
            sin >> itmp; // 输入指令编号
            if(itmp == 2){ // 摸牌
                sin >> stmp;
                hand.push_back(stmp);
                sin.clear();
                sin.str(response[i]); // 输出 PLAY/GANG/BUGANG/HU Card1 本回合尚未填入
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
    if(turnID < 2){
        sout << "PASS";
    }else{
        sin.str(request[turnID]); // 本回合输入信息
        sin >> itmp;
        /**
         * 考虑把以下决策内容封装为函数 Draw() Peng() Chi()...
         * 目前功能简单，先这么写吧
        */
        if(itmp == 2){ // 摸牌
            sin >> stmp;
            hand.push_back(stmp);
            /**
             * 此处添加将本回合摸的牌加入手牌， 经过botzone测试，可以正常使用
             * 注：原样例没有此步骤也是可以正常运行的，因为本回合摸的牌会在下一回合request序列里加入手牌
             * 只不过本回合的出牌、杠、胡等操作不会在手牌里考虑这张牌
             * 
            */

            /**
             * 此处添加算法
             * 暂时是傻傻的随机出牌
            */
            random_shuffle(hand.begin(), hand.end());
            sout << "PLAY " << *hand.rbegin();
            hand.pop_back();
        }
        /*
        else if(其他输入(GANG/CHI/PENG)){
            ...
        }
        */
        else{
            sout << "PASS"; // 样例只能处理摸牌，其余就PASS
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
