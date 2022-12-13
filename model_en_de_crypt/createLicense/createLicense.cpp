/*收到客户发来的base64编码的code
    base解码得到机器信息（mac ip cpuid diskid）串,
    把串用私钥加密,生成密文写入license文件
*/
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

using namespace std;
#define NSIZE 8
// there are 64 characters
// static const std::string base64_chars =
//     "ABCDEFGHIJKLMNOPQRSTUVWXYZ"
//     "abcdefghijklmnopqrstuvwxyz"
//     "0123456789+/";
const char *stre = "65537";
const char *strd = "3800070437834197699717066228865321203631301869992519470654072179076972341224973826233186018827097718656338891490509451995401504589373899959036014800711377726346356463261816979674320434955501714996720884390940433176592221351101598452732509992277762099251207596480601869257058307638143555271800582808417517953";
const char *strN = "12824161497648806109493170414065219141214450600087525671897833594241376741856905749219583528108728279484061891432209987405902595585674422328287502677354361728699605554502981733825111117296315379303734757007569450989259507253913968419806633596839115832686193690568344292191404931509987998973800727140892044943";
mpz_t mpzte;
mpz_t mpztd;
mpz_t mpztN;
static inline bool is_base64(unsigned char c)
{
    return (isalnum(c) || (c == '+') || (c == '/'));
}

std::string base64_decode(std::string &encoded_string)
{
    size_t in_len = encoded_string.size();
    int i = 0;
    int j = 0;
    int in_ = 0;
    unsigned char char_array_4[4], char_array_3[3];
    std::string ret;

    while (in_len-- && (encoded_string[in_] != '=') && is_base64(encoded_string[in_]))
    {
        char_array_4[i++] = encoded_string[in_];
        in_++;
        if (i == 4)
        {
            for (i = 0; i < 4; i++)
                char_array_4[i] = base64_chars.find(char_array_4[i]) & 0xff;

            char_array_3[0] = (char_array_4[0] << 2) + ((char_array_4[1] & 0x30) >> 4);
            char_array_3[1] = ((char_array_4[1] & 0xf) << 4) + ((char_array_4[2] & 0x3c) >> 2);
            char_array_3[2] = ((char_array_4[2] & 0x3) << 6) + char_array_4[3];

            for (i = 0; (i < 3); i++)
                ret += char_array_3[i];
            i = 0;
        }
    }

    if (i)
    {
        for (j = 0; j < i; j++)
            char_array_4[j] = base64_chars.find(char_array_4[j]) & 0xff;

        char_array_3[0] = (char_array_4[0] << 2) + ((char_array_4[1] & 0x30) >> 4);
        char_array_3[1] = ((char_array_4[1] & 0xf) << 4) + ((char_array_4[2] & 0x3c) >> 2);

        for (j = 0; (j < i - 1); j++)
            ret += char_array_3[j];
    }

    return ret;
}

std::string IntToString(int &i)
{
    std::string s;
    stringstream ss(s);
    ss << i;
    return ss.str();
}
//string 转2进制(先转int[]再转string)
std::string StrToBitStr(string str)
{
    int size8 = str.size() * 8;
    int bstr[size8];
    for (int i = 0; i < str.size(); i++)
    {
        bitset<8> bits = bitset<8>(str[i]);
        for (int j = 0; j < 8; j++)
        {
            bstr[i * 8 + j] = bits[7 - j];
        }
    }
    int arrayLength = sizeof(bstr) / sizeof(bstr[0]);
    string s;
    for (int i = 0; i < arrayLength; i++)
    {
        int &temp = bstr[i];
        s += IntToString(temp);
    }

    //添加一个翻转操作
    //reverse(begin(s), end(s));
    return s;
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

//string写入文件
void write_txt(std::string str)
{
    std::string write_file_name = "License";
    ofstream os;                                //创建一个文件输出流对象
    os.open(write_file_name.c_str(), ios::out); //将对象与文件关联
    cout << "向License文件中中写入内容..." << endl;
    os << str; //将输入的内容放入txt文件中
    os.close();
}

//从文件取出string
string readFileIntoString(char *filename)
{
    ifstream ifile(filename);
    //将文件读入到ostringstream对象buf中
    ostringstream buf;
    char ch;
    while (buf && ifile.get(ch))
        buf.put(ch);
    //返回与流对象buf关联的字符串
    return buf.str();
}

//取文件解密(暂时注释掉)
// bool valiLicense()
// {
//     //----------------------解密---------------------------
//     //License文件内容取出
//     string licFromFile = readFileIntoString((char *)"License");
//     char LC[1000][2 * MODULU_BYTES + 1] = {'\0'};   //定义LC用于储存licence数据
//     char M_de[1000][2 * MODULU_BYTES + 1] = {'\0'}; //M_de储存解密后的二进制数据
//     int numC = licFromFile.size() / (2 * MODULU_BYTES);
//     string de_licenseCode;
//     for (int i = 0; i < numC; ++i)
//     {
//         strncpy(LC[i], licFromFile.substr(i * (2 * MODULU_BYTES), (2 * MODULU_BYTES)).c_str(), 257);
//         //(N,e)公钥解密
//         RSAES_PKCS1_V1_5_DECRYPT(mpztN, mpzte, (unsigned char *)LC[i], (unsigned char *)M_de[i]);
//         string strM_de = M_de[i];
//         de_licenseCode += strM_de;
//     }
//     cout << "解密后：" << de_licenseCode << endl;
//     //二进制转string
//     string str_debit = BitStrToStr(de_licenseCode);
//     cout << "解二进制：" << str_debit << "   " << str_debit.size() << endl;
//     string tmp;
//     vector<string> vec_debit;
//     stringstream input(str_debit);
//     while (getline(input, tmp, ','))
//     {
//         vec_debit.push_back(tmp);
//     }

//     cout << "转vector: " << vec_debit[0] << "           " << vec_debit[1] << endl;
//     ;

//     //-------------获取机器信息串----------------------------------
//     string mcCruInfo = getMcInfo();
//     //逗号分隔信息和时间
//     vector<string> vec_mcCruInfo;

//     stringstream input2(mcCruInfo);
//     while (getline(input2, tmp, ','))
//     {
//         vec_mcCruInfo.push_back(tmp);
//     }
//     cout << "重新getINFO转vector: " << vec_mcCruInfo[0] << "           " << vec_mcCruInfo[1] << endl;
//     ;
//     if (vec_mcCruInfo[0] != vec_debit[0])
//     {
//         cout << "face_car.SO : 机器信息不同" << endl;
//         return false;
//     }
//     else
//     {
//         cout << "机器信息相同" << endl;
//     }
//     //验证时间
//     if (atoi(vec_mcCruInfo[1].c_str()) - atoi(vec_debit[1].c_str()) > 15)
//     {
//         cout << "license过期" << endl;
//         return false;
//     }
//     else
//     {
//         cout << "license时间有效！" << endl;
//     }
//     return true;
// }

int main()
{
    cout << "Please enter machineCode：" << endl;

    std::string mcInfo64;
    cin >> mcInfo64;
    //base64解码
    std::string mcInfo = base64_decode(mcInfo64);
    //string转2进制
    std::string mcInfoBit = StrToBitStr(mcInfo);
    // cout << "BITSET" << endl;
    // cout << mcInfoBit << endl;
    //cout << BitStrToStr(mcInfoBit) << endl;
    //公私钥赋值从char赋值
    mpz_init_set_str(mpzte, stre, 10);
    mpz_init_set_str(mpztd, strd, 10);
    mpz_init_set_str(mpztN, strN, 10);
    //cout << mcInfoBit.size() << endl;
    char M[1000][2 * MODULU_BYTES];
    int num = mcInfoBit.size() / (2 * (MODULU_BYTES - 11));
    int val = mcInfoBit.size() / (2 * (MODULU_BYTES - 11));
    int index = 0;
    //将二进制string 每取234个元素就追加到M[i]里：
    if (num == 0)
    {
        strncpy(M[0], mcInfoBit.c_str(), mcInfoBit.length() + 1);
        index = 1;
    }
    else
    {
        for (int i = 0; i < num; ++i)
        {
            strncpy(M[i], mcInfoBit.substr(i * (2 * (MODULU_BYTES - 11)), (2 * (MODULU_BYTES - 11))).c_str(), 2 * (MODULU_BYTES - 11) + 1);
        }
        strncpy(M[num], mcInfoBit.substr(num * (2 * (MODULU_BYTES - 11)), mcInfoBit.size() - (num * (2 * (MODULU_BYTES - 11)))).c_str(), mcInfoBit.substr(num * (2 * (MODULU_BYTES - 11)), mcInfoBit.size() - (num * (2 * (MODULU_BYTES - 11)))).size() + 1);
        index = num + 1;
    }

    char C[1000][2 * MODULU_BYTES + 1] = {'\0'};
    string licenseCode;
    for (int i = 0; i < index; ++i)
    {
        //(N,)
        RSAES_PKCS1_V1_5_ENCRYPT(mpztN, mpztd, (const unsigned char *)M[i], (unsigned char *)C[i]);
        // printf("M[%d] is: %s\n", i, M[i]);
        // printf("strlen(M[%d]) is: %d\n", i, strlen(M[i]));
        // cout << num << endl;
        // printf("C[%d] is: %s\n", i, C[i]);
        string strC = C[i];
        licenseCode += strC;
        //printf("strlen(C[%d]) is: %d\n", i, strlen(C[i]));
    }
    write_txt(licenseCode);
    //cout << "生成license密文 :" << licenseCode << endl;
    cout << "已在本目录下生成License文件" << endl;

    //valiLicense();

    mpz_clear(mpztN);
    mpz_clear(mpztd);
    mpz_clear(mpzte);
    return 0;
}
