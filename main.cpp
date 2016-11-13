

#include <stdio.h>
#include <iostream>
#include <string>
#include <map>
#include "VON_RegEx.h"

using namespace std;

int main(){
    VON_RegEx parser;//初始化引擎
    string strRegEx;//输入的正则表达式
    string strText;//搜索的文本
    int nPos=0;
    cin>>strRegEx;
    cin>>strText;
    if(parser.SetRegEx(strRegEx)){
        if(parser.FindFirst(strText,nPos, strRegEx)){
            while (parser.FindNext(nPos, strRegEx)) {
            }
        }
        else cout<<"not found"<<endl;
    }
    else{
        cout<<"not supported or error"<<endl;
    }
    parser.show();//展示所有找到的匹配的句子
    return 0;
}
