//
//  main.cpp
//  SNL
//
//  Created by apple on 16/5/26.
//  Copyright © 2016年 Tony_LB. All rights reserved.
//  仅仅进行词法分析，语法分析（并不生成语法树）

#include <iostream>
#include <vector>
#include <list>
#include <tuple>
#include <map>
#include <fstream>
#include <stdlib.h>
#include <regex>
#include <boost/algorithm/string.hpp>

//只用了单一命名空间
using namespace std;

typedef tuple<int, string, string> token;
ifstream codeFile;
enum state{START, INASSIGN, INCOMMENT, INNUM, INID, INCHAR, INRANGE, DONE};
vector<string> reserWord;
vector<token> tokenList;
//分析表
map<pair<string, string>, int> analysisTable;
//语法表
vector< vector<string> > grammaTable;
//记录Predict集
vector<vector<string>> predicts;
//记录行号，列号
int lines = 1, row = 0;

//初始化函数
void init(ifstream & codeFile)
{
    char buffer[256];
    
    //这个文件里放保留字，每一行放一个
    ifstream in("/Users/apple/Documents/SNL/reserWord.txt");
    if (! in.is_open())
    {
        cout << "CANT FIND reserWord.txt";
        exit(-1);
    }
    //初始化保留字表
    while (!in.eof() )
    {
        in.getline(buffer,256);
        reserWord.push_back(buffer);

    }
    in.close();
    
    string fileName;
    /*cout << "请输入文件名";
    cin >> fileName;*/
    fileName = "/Users/apple/Documents/SNL/SCAN.txt";
    codeFile.open(fileName);
    
    if (! codeFile.is_open())
    {
        cout <<  "cant find "+ fileName << endl;
        exit(-1);
    }
    in.close();
    //初始化语法分析需要的
    in.open("/Users/apple/Documents/SNL/gramma.txt");
    if (! in.is_open())
    {
        cout << "CANT FIND gramma.txt";
        exit(-1);
    }
    string gramma;
    regex re(".* ::= .*\\s");
    vector<string> icon;
    string befCom;
    while (!in.eof() )
    {
        getline(in, gramma);
        if(gramma.size() <= 0)
            continue;
        boost::split(icon, gramma, boost::is_any_of("   "));
        //带有::=
        if(regex_match(gramma, re))
        {
            icon.erase(icon.begin());
            befCom = icon[0];
        }
        //不带有::=
        else
        {
            icon.erase(icon.begin(), icon.begin() + 2);
            icon.insert(icon.begin(), "::=");
            icon.insert(icon.begin(), befCom);
        }
        string::iterator it = icon[icon.size() - 1].end() - 1;
        if(*it == '\r')
        {
            icon[icon.size() - 1].erase(it);
        }
        grammaTable.push_back(icon);
        icon.clear();
    }
    in.close();
    //处理predict集合
    in.open("/Users/apple/Documents/SNL/predict.txt");
    if (! in.is_open())
    {
        cout << "CANT FIND predict.txt";
        exit(-1);
    }
    string line;
    vector<string> predict;
    re.assign(".*\\{.*");
    while ( !in.eof() )
    {
        getline(in, line);
        if( !regex_match(line, re))
            continue;
        boost::split(predict, line, boost::is_any_of(" ,{}  "));
        vector<string>::iterator it = predict.begin();
        while( it != predict.end() )
        {
            if ( it->size() == 0)
                it = predict.erase(it);
            else
                ++it;
        }
        predicts.push_back(predict);
    }
    //初始化分析表
    for (int i = 1; i <= 104; ++i)
    {
        for (vector<string>::iterator it = predicts[i - 1].begin(); it != predicts[i - 1].end(); ++it)
        {
            analysisTable[make_pair( *grammaTable[i - 1].begin(), *it)] = i;
        }
    }
}

//显示当前token序列
void showTokenList()
{
    for(vector<token>::iterator it = tokenList.begin(); it != tokenList.end(); ++it)
        cout << get<1>(*it) << "  " << get<2>(*it) << endl;
    for(vector<string>::iterator it = reserWord.begin(); it != reserWord.end(); ++it)
        cout << *it << endl;
}

//读取一个char
inline void getChar(char & ch)
{
    ch = codeFile.get();
    ++row;
}

//是否是数字
inline bool isNum( const char & ch)
{
    if(ch <= '9' && ch >= '0')
        return  true;
    else
        return false;
}

//是否是字母
inline bool isChar(const char & ch)
{
    if( (ch <= 'z' && ch >= 'a') || ( ch <= 'Z' && ch >= 'A' ) )
        return true;
    else
        return false;
}

//验证标识符是否为保留字,是保留字为真，否则为假(书中提起的函数，其实可以放进setID中)
bool reservedlookup(const string & word)
{
    for(vector<string>::iterator it = reserWord.begin(); it != reserWord.end(); ++it)
        if(word == *it)
            return true;
    return false;
}

//将标识符放入token序列
void setID(int lines, string  word)
{
    
    if( reservedlookup(word) )
    {
        //神奇的类型转换
        for(string::iterator it = word.begin(); it != word.end(); ++it)
            *it -= 32;
        tokenList.push_back(make_tuple(lines, word, ""));
    }
    else
    {
        tokenList.push_back(make_tuple(lines, "ID", word));
    }
}

//显示语法错误,inline提高效率
inline void showWordError(const int & line, const int & row)
{
    cout << "在" << line << "行" << row << "列有词法错误" << endl;
    //showTokenList();
    exit(-1);
}

//词法分析
void scanner(ifstream & codeFile)
{
    state now = START;
    //读取到的词
    string word = "";
    //进入循环
    char ch = ' ';
    //记录行数,列数
    getChar(ch);
    
    while( ch != EOF && now != DONE)
    {
        switch (now) {
            case START:
            {
                if(ch == '.')
                {
                    getChar(ch);
                    if(ch == '.')
                    {
                        tokenList.push_back(make_tuple(lines, "..", ""));
                        break;
                    }
                    else
                    tokenList.push_back(make_tuple(lines, ".", ""));
                    now = DONE;
                    codeFile.seekg(-1, ios::cur);
                    --row;
                    break;
                }
                if(ch == '\r')
                {
                    // \r\n为回车，直接读到下一行
                    getChar(ch);
                    ++lines;
                    row = 0;
                    break;
                }
                if(ch == '\n')
                {
                    //系统不同，有时单独'\n'为回车
                    ++lines;
                    row = 0;
                    break;
                }
                if(ch == ' ' || ch == '\t')
                {
                    break;
                }
                if( isChar(ch) )
                {
                    now = INID;
                    word += ch;
                    break;
                }
                if(ch == '\'')
                {
                    now = INCHAR;
                    break;
                }
                if( isNum(ch) )
                {   now = INNUM;
                    word += ch;
                    break;
                }
                if(ch == '{')
                {
                    now = INCOMMENT;
                    break;
                }
                if(ch == ':')
                {
                    now = INASSIGN;
                    break;
                }
                if(ch == '[')
                {
                    tokenList.push_back(make_tuple(lines, "[", ""));
                    break;
                }
                if(ch == ']')
                {
                    tokenList.push_back(make_tuple(lines, "]", ""));
                    break;
                }
                if(ch == ';')
                {
                    tokenList.push_back(make_tuple(lines, ";", ""));
                    break;
                }
                if(ch == '(')
                {
                    tokenList.push_back(make_tuple(lines, "(", ""));
                    break;
                }
                if(ch == ')')
                {
                    tokenList.push_back(make_tuple(lines, ")", ""));
                    break;
                }
                if(ch == '=')
                {
                    tokenList.push_back(make_tuple(lines, "=", ""));
                    break;
                }
                if(ch == '<')
                {
                    tokenList.push_back(make_tuple(lines, "<", ""));
                    break;
                }
                if(ch == '>')
                {
                    tokenList.push_back(make_tuple(lines, ">", ""));
                    break;
                }
                if(ch == '+')
                {
                    tokenList.push_back(make_tuple(lines, "+", ""));
                    break;
                }
                if(ch == '-')
                {
                    tokenList.push_back(make_tuple(lines, "-", ""));
                    break;
                }
                if(ch == '*')
                {
                    tokenList.push_back(make_tuple(lines, "*", ""));
                    break;
                }
                if(ch == '/')
                {
                    tokenList.push_back(make_tuple(lines, "/", ""));
                    break;
                }
                //验证‘，’后面跟随的是语法分析的事情
                if(ch == ',')
                {
                    tokenList.push_back(make_tuple(lines, "COMMA", ""));
                    break;
                }
                
                //其他的状况全是错误
                showWordError(lines, row);
            }
            
            case INID:
                //标识符结束
                if(  isNum(ch) || isChar(ch)  )
                    word += ch;
                else
                {
                    if(ch == '.')
                    {
                        getChar(ch);
                        if( isChar(ch) )
                        {
                            word += ".";
                            word += ch;
                            break;
                        }
                        if(ch == EOF)
                        {
                            regex re("[a-z|A-Z|_][a-z|A-Z|0-9|.]*");
                            if(regex_match(word, re))
                            {
                                setID(lines, word);
                                now = START;
                                word = "";
                            }
                            else
                            {
                                cout << endl << word <<  "不符合正则表达式!" << endl;
                                showWordError(lines, row);
                            }
                            tokenList.push_back(make_tuple(lines, ".", ""));
                            now = DONE;
                            break;
                        }
                        codeFile.seekg(-1, ios::cur);
                        --row;
                    }
                    //使用正则测试标识符是否符合规则
                    regex re("[a-z|A-Z|_][a-z|A-Z|0-9|.]*");
                    if(regex_match(word, re))
                    {
                        setID(lines, word);
                        now = START;
                        word = "";
                    }
                    else
                    {
                        cout << endl << word <<  "不符合正则表达式!" << endl;
                        showWordError(lines, row);
                    }
                    codeFile.seekg(-1, ios::cur);
                    --row;
                }
                
                break;
                
            case INCHAR:
                if(ch != '\'')
                {
                    word += ch;
                }
                else
                {
                    tokenList.push_back(make_tuple(lines, "CHARC", word));
                    now = START;
                    word = "";
                }
                break;
                
            case INNUM:
                if( ch > '9' || ch < '0')
                {
                    tokenList.push_back(make_tuple(lines, "INTC", word));
                    now =START;
                    word = "";
                    codeFile.seekg(-1, ios::cur);
                }
                else
                    word += ch;
                break;
                
            case INCOMMENT:
                if(ch == '}')
                {
                    now = START;
                }
                else
                {
                    if(ch == '\r')
                    {
                        // \r\n为回车，直接读到下一行
                        getChar(ch);
                        ++lines;
                        row = 0;
                        break;
                    }
                }
                break;

            case INASSIGN:
                if(ch != '=')
                {
                    showWordError(lines, row);
                }
                else
                {
                    tokenList.push_back(make_tuple(lines, ":=", ""));
                    now = START;
                }
                break;
                
            case INRANGE:
            {
                string num1, num2;
                //看读的下一个是否是数字
                if( !isNum(ch))
                    showWordError(lines, row);
                //读出所有数字
                while( isNum(ch) )
                {
                    num1 += ch;
                    getChar(ch);
                    continue;
                }
                //检测中间是否是".."形式
                if( ch == '.')
                {
                    getChar(ch);
                    if(ch != '.')
                        showWordError(lines, row);
                }
                else
                {
                    showWordError(lines, row);
                }
                //检测后面是否是数字
                getChar(ch);
                if( !isNum(ch) )
                    showWordError(lines, row);
                while( isNum(ch) )
                {
                    num2 += ch;
                    getChar(ch);
                    continue;
                }
                if( ch != ']')
                    showWordError(lines, row);
                tokenList.push_back(make_tuple(lines, "ARRAY", ""));
                tokenList.push_back(make_tuple(lines, "[", ""));
                tokenList.push_back(make_tuple(lines, "INTC", num1));
                tokenList.push_back(make_tuple(lines, "..", ""));
                tokenList.push_back(make_tuple(lines, "INTC", num2));
                tokenList.push_back(make_tuple(lines, "]", ""));
                now = START;
                break;
            }
                
            default:
                break;
        }
        getChar(ch);
    }
    if(now != DONE)
    {
        showWordError(lines, row);
    }

}
//词法分析结束



//LL(1)自顶向下语法分析,因为符号栈是按照逆序压栈，所以相当于FIFO，用list即可
void parseLL1()
{
    //初始化符号栈
    list<string> symbol;
    symbol.push_front("Program");
    map<pair<string, string>, int>::iterator mapIt;
    vector<token>::iterator tokIt = tokenList.begin();
    while ( symbol.size() != 0 && tokenList.size() != 0)
    {
        //match
        int LINE = get<0>(*tokIt);
        if(symbol.front() == get<1>(*tokIt))
        {
            symbol.erase(symbol.begin());
            tokenList.erase(tokenList.begin());
            tokIt = tokenList.begin();
            continue;
        }
        mapIt = analysisTable.find(make_pair(symbol.front(), get<1>(*tokIt)));
        if(mapIt == analysisTable.end())
        {
            cout << "在" << get<0>(*tokIt) << "行有语法错误" << endl;
            exit(-1);
        }
        symbol.erase(symbol.begin());
        vector<string>::iterator gramma = grammaTable[mapIt->second - 1].end() - 1;
        while( *gramma != "::=")
        {
            if(*gramma != "ε")
                symbol.push_front( * gramma );
            --gramma;
        }
    }
    if(symbol.size() !=0 || tokenList.size() != 0)
    {
        if(mapIt != analysisTable.end())
        {
            cout << "在" << get<0>(*tokIt) << "行有语法错误" << endl;
            exit(-1);
        }
        else
        {
            cout << "有语法错误" << endl;
            exit(-1);
        }
    }
}

int main(int argc, const char * argv[]) {
    
    //初始化状态
    init( codeFile );
    //词法分析
    scanner( codeFile );
    
    parseLL1();
    codeFile.close();
    cout << "Hello" << endl;
    return 0;
}
