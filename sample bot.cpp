#include <iostream>
#include <string>
#include <sstream>
#include <vector>
#include <algorithm>

/* 这些好像不需要
#ifdef _BOTZONE_ONLINE
#include "jsoncpp/json.h"
#else
#include <json/json.h>
#endif

#define SIMPLEIO 0
//由玩家自己定义，0表示JSON交互，1表示简单交互。

*/

using namespace std;

vector<string> request, response; // 该回合之前所有信息
vector<string> hand; // 手牌

int main()
{
    int turnID;
    string stmp;
//#if SIMPLEIO
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
/* 不需要
#else
    Json::Value inputJSON;
    cin >> inputJSON;
    turnID = inputJSON["responses"].size();
    for(int i = 0; i < turnID; i++) {
        request.push_back(inputJSON["requests"][i].asString());
        response.push_back(inputJSON["responses"][i].asString());
    }
    request.push_back(inputJSON["requests"][turnID].asString());
#endif
*/
    if(turnID < 2) { // 第一、二回合，直接输出pass
        response.push_back("PASS");
    } else {
        int itmp, myPlayerID, quan;
        /**
         * ostringstream istringstream 用于字符串和其他数据类型之间方便转换
        */
        ostringstream sout;
        istringstream sin;
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
        for(int i = 2; i < turnID; i++) { // 一二之后每回合的输入输出(包括本回合)
            sin.clear();
            sin.str(request[i]);
            sin >> itmp; // 输入指令编号
            if(itmp == 2) { // 摸牌
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
        /**
        * 样例只处理输入编号为2（摸牌）情况并随机出牌
        * 此处可以在else处修改，根据本轮输入决定是否吃、碰、杠
        */
        sin.clear();
        sin.str(request[turnID]); // 本回合输入信息
        sin >> itmp;
        if(itmp == 2) { // 摸牌(已经加入手牌)
            random_shuffle(hand.begin(), hand.end());
            sout << "PLAY " << *hand.rbegin();
            hand.pop_back();
        } else {
            sout << "PASS"; // 样例除了打牌什么也不做
        }
        /**
         * else if(...){
         * 算法ai...
         * ...
         * }
        */
        response.push_back(sout.str());
    }

// #if SIMPLEIO
    cout << response[turnID] << endl;
/*
#else
    Json::Value outputJSON;
    outputJSON["response"] = response[turnID];
    cout << outputJSON << endl;
#endif
*/
    return 0;
}
