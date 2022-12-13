//(e, N) 公钥加密
#include <iostream>
#include <string>
#include <ctime>
#include <stdio.h>
#include <bitset>
#include <algorithm>
#include <sstream>
#include "base64.h"
#include <iomanip>
#include <vector>
#include "rsa.c"
#include "getINFO.cpp"
using namespace std;
#define NSIZE 8
// there are 64 characters
// static const std::string base64_chars =
//     "ABCDEFGHIJKLMNOPQRSTUVWXYZ"
//     "abcdefghijklmnopqrstuvwxyz"
//     "0123456789+/";
const char *stre = "65537";
const char *strN = "12824161497648806109493170414065219141214450600087525671897833594241376741856905749219583528108728279484061891432209987405902595585674422328287502677354361728699605554502981733825111117296315379303734757007569450989259507253913968419806633596839115832686193690568344292191404931509987998973800727140892044943";
mpz_t mpzte;
mpz_t mpztN;

//从文件取出string
string readFileIntoString(char *filename)
{
    ifstream ifile(filename);
    if (!ifile.is_open())
    {
        cout << "License文件不存在" << endl;
        exit(-1);
    }
    //将文件读入到ostringstream对象buf中
    ostringstream buf;
    char ch;
    while (buf && ifile.get(ch))
    {
        buf.put(ch);
    }
    //返回与流对象buf关联的字符串
    return buf.str();
}

//二进制字符串转化为字符串
string BitStrToStr(string bstr)
{
    string str = "";
    //每八位转化成十进制，然后将数字结果转化成字符
    int sum;
    for (int i = 0; i < bstr.size(); i += 8)
    {
        sum = 0;
        for (int j = 0; j < 8; j++)
            if (bstr[i + j] == '1')
                sum = sum * 2 + 1;
            else
                sum = sum * 2;
        str = str + char(sum);
    }
    return str;
}

//取文件解密
bool valiLicense()
{
    mpz_init_set_str(mpzte, stre, 10);
    mpz_init_set_str(mpztN, strN, 10);
    //----------------------解密---------------------------
    //License文件内容取出
    string licFromFile = readFileIntoString((char *)"License");
    char LC[1000][2 * MODULU_BYTES + 1] = {'\0'};   //定义LC用于储存licence数据
    char M_de[1000][2 * MODULU_BYTES + 1] = {'\0'}; //M_de储存解密后的二进制数据
    int numC = licFromFile.size() / (2 * MODULU_BYTES);
    string de_licenseCode;
    for (int i = 0; i < numC; ++i)
    {
        strncpy(LC[i], licFromFile.substr(i * (2 * MODULU_BYTES), (2 * MODULU_BYTES)).c_str(), 257);
        //(N,e)公钥解密
        RSAES_PKCS1_V1_5_DECRYPT(mpztN, mpzte, (unsigned char *)LC[i], (unsigned char *)M_de[i]);
        string strM_de = M_de[i];
        de_licenseCode += strM_de;
    }

    //二进制转string
    string str_debit = BitStrToStr(de_licenseCode);

    string tmp;
    vector<string> vec_debit;
    stringstream input(str_debit);
    while (getline(input, tmp, ','))
    {
        vec_debit.push_back(tmp);
    }

    //-------------获取机器信息串----------------------------------
    string mcCruInfo = getMcInfo();
    //逗号分隔信息和时间
    vector<string> vec_mcCruInfo;

    stringstream input2(mcCruInfo);
    while (getline(input2, tmp, ','))
    {
        vec_mcCruInfo.push_back(tmp);
    }

    if (vec_mcCruInfo[0] != vec_debit[0])
    {
        cout << "LicenseError : 机器信息不同" << endl;
        return false;
    }
    else
    {
        cout << "机器信息相同" << endl;
    }
    //验证时间
    if (atoi(vec_mcCruInfo[1].c_str()) - atoi(vec_debit[1].c_str()) > 30)
    {
        cout << "license过期" << endl;
        return false;
    }
    else
    {
        cout << "license时间有效！" << endl;
    }
    return true;
}

// int main()
// {
//     //公私钥赋值从char赋值

//     if (valiLicense())
//     {
//         cout << "create Infer successed !" << endl;
//     }
//     else
//     {
//         cout << "create Infer error !" << endl;
//     }
//     return 0;
// }