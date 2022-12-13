/*get cpuid、mac、ip、diskID*/
#include <iostream>
#include <cstdio>
#include <cstring>
#include <cstdlib>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <string.h>
#include <string>
#include <fstream>
#include <stdlib.h>
#include <stdio.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <errno.h>
//使用 ifconf结构体和ioctl函数时需要用到该头文件
#include <net/if.h>
//使用ifaddrs结构体时需要用到该头文件
#include <ifaddrs.h>
#include <cctype>
#include <fcntl.h>
#include <scsi/sg.h>
#include <sys/ioctl.h>
#include <linux/hdreg.h>
#include "ctime"
#define PJ_MAX_HOSTNAME (128)
#define RUN_SUCCESS 0
#define RUN_FAIL -1

using namespace std;
static const std::string base64_chars =
    "ABCDEFGHIJKLMNOPQRSTUVWXYZ"
    "abcdefghijklmnopqrstuvwxyz"
    "0123456789+/";

//------------------------------getMAC:------------------------------------
bool get_mac_address_by_ioctl1(std::string &mac_address)
{
    mac_address.clear();

    int sock = socket(AF_INET, SOCK_STREAM, 0);
    if (sock < 0)
    {
        return (false);
    }

    struct ifreq ifr = {0};
    strncpy(ifr.ifr_name, "eth0", sizeof(ifr.ifr_name) - 1);
    bool ret = (ioctl(sock, SIOCGIFHWADDR, &ifr) >= 0);

    close(sock);

    const char hex[] =
        {
            '0', '1', '2', '3', '4', '5', '6', '7',
            '8', '9', 'a', 'b', 'c', 'd', 'e', 'f'};
    char mac[16] = {0};
    for (int index = 0; index < 6; ++index)
    {
        size_t value = ifr.ifr_hwaddr.sa_data[index] & 0xFF;
        mac[2 * index + 0] = hex[value / 16];
        mac[2 * index + 1] = hex[value % 16];
    }
    std::string(mac).swap(mac_address);

    return (ret);
}

bool get_mac_address_by_ioctl2(std::string &mac_address)
{
    mac_address.clear();

    int sock = socket(AF_INET, SOCK_STREAM, 0);
    if (sock < 0)
    {
        return (false);
    }

    struct ifreq ifr = {0};
    strncpy(ifr.ifr_name, "em1", sizeof(ifr.ifr_name) - 1);
    bool ret = (ioctl(sock, SIOCGIFHWADDR, &ifr) >= 0);

    close(sock);

    const char hex[] =
        {
            '0', '1', '2', '3', '4', '5', '6', '7',
            '8', '9', 'a', 'b', 'c', 'd', 'e', 'f'};
    char mac[16] = {0};
    for (int index = 0; index < 6; ++index)
    {
        size_t value = ifr.ifr_hwaddr.sa_data[index] & 0xFF;
        mac[2 * index + 0] = hex[value / 16];
        mac[2 * index + 1] = hex[value % 16];
    }
    std::string(mac).swap(mac_address);

    return (ret);
}

static void parse_mac_address(const char *file_name, const char *match_words, std::string &mac_address)
{
    mac_address.c_str();

    std::ifstream ifs(file_name, std::ios::binary);
    if (!ifs.is_open())
    {
        return;
    }

    char line[4096] = {0};
    while (!ifs.eof())
    {
        ifs.getline(line, sizeof(line));
        if (!ifs.good())
        {
            break;
        }

        const char *mac = strstr(line, match_words);
        if (NULL == mac)
        {
            continue;
        }
        mac += strlen(match_words);

        while ('\0' != mac[0])
        {
            if (' ' != mac[0] && ':' != mac[0])
            {
                mac_address.push_back(mac[0]);
            }
            ++mac;
        }

        if (!mac_address.empty())
        {
            break;
        }
    }

    ifs.close();
}

static bool get_mac_address_by_system(std::string &mac_address)
{
    mac_address.c_str();

    const char *lshw_result = ".lshw_result.txt";
    char command[512] = {0};
    snprintf(command, sizeof(command), "lshw -c network | grep serial | head -n 1 > %s", lshw_result);

    if (0 == system(command))
    {
        parse_mac_address(lshw_result, "serial:", mac_address);
    }

    unlink(lshw_result);

    return (!mac_address.empty());
}

static bool get_mac_address(std::string &mac_address)
{
    if (get_mac_address_by_ioctl1(mac_address))
    {
        return (true);
    }

    if (get_mac_address_by_ioctl2(mac_address))
    {
        return (true);
    }

    if (get_mac_address_by_system(mac_address))
    {
        return (true);
    }
    return (false);
}

//-------------------------------getIP------------------------------------
/*可以检测ip4也可以检测ip6，但是需要ifaddrs.h，某些Android系统上没有该头文件（可自己实现该头文件所带内容）
该方法较为强大，可以通过网卡名（ifAddrStruct->ifa_name）获取IP. 
*/
int get_local_ip_using_ifaddrs(char *str_ip)
{
    struct ifaddrs *ifAddrStruct = NULL;
    void *tmpAddrPtr = NULL;
    int status = RUN_FAIL;

    getifaddrs(&ifAddrStruct);

    while (ifAddrStruct != NULL)
    {
        if (ifAddrStruct->ifa_addr->sa_family == AF_INET) // check it is IP4
        {
            tmpAddrPtr = &((struct sockaddr_in *)ifAddrStruct->ifa_addr)->sin_addr;
            if (inet_ntop(AF_INET, tmpAddrPtr, str_ip, INET_ADDRSTRLEN))
            {
                status = RUN_SUCCESS;
                if (strcmp("127.0.0.1", str_ip))
                {
                    break;
                }
            }
        }
        else if (ifAddrStruct->ifa_addr->sa_family == AF_INET6)
        {
            //可以添加IP6相应代码
        }
        ifAddrStruct = ifAddrStruct->ifa_next;
    }
    return status;
}
//----------------------------getDiskID----------------------------------

static bool get_disk_name(std::string &disk_name)
{
    disk_name.c_str();

    std::ifstream ifs("/etc/mtab", std::ios::binary);
    if (!ifs.is_open())
    {
        return (false);
    }

    char line[4096] = {0};
    while (!ifs.eof())
    {
        ifs.getline(line, sizeof(line));
        if (!ifs.good())
        {
            break;
        }

        const char *disk = line;
        while (isspace(disk[0]))
        {
            ++disk;
        }

        const char *space = strchr(disk, ' ');
        if (NULL == space)
        {
            continue;
        }

        const char *mount = space + 1;
        while (isspace(mount[0]))
        {
            ++mount;
        }
        if ('/' != mount[0] || ' ' != mount[1])
        {
            continue;
        }

        while (space > disk && isdigit(space[-1]))
        {
            --space;
        }

        if (space > disk)
        {
            std::string(disk, space).swap(disk_name);
            break;
        }
    }

    ifs.close();

    return (!disk_name.empty());
}

static void trim_serial(const void *serial, size_t serial_len, std::string &serial_no)
{
    const char *serial_s = static_cast<const char *>(serial);
    const char *serial_e = serial_s + serial_len;
    while (serial_s < serial_e)
    {
        if (isspace(serial_s[0]))
        {
            ++serial_s;
        }
        else if ('\0' == serial_e[-1] || isspace(serial_e[-1]))
        {
            --serial_e;
        }
        else
        {
            break;
        }
    }

    if (serial_s < serial_e)
    {
        std::string(serial_s, serial_e).swap(serial_no);
    }
}

static bool scsi_io(
    int fd, unsigned char *cdb,
    unsigned char cdb_size, int xfer_dir,
    unsigned char *data, unsigned int data_size,
    unsigned char *sense, unsigned int sense_len)
{
    sg_io_hdr_t io_hdr = {0};
    io_hdr.interface_id = 'S';
    io_hdr.cmdp = cdb;
    io_hdr.cmd_len = cdb_size;
    io_hdr.sbp = sense;
    io_hdr.mx_sb_len = sense_len;
    io_hdr.dxfer_direction = xfer_dir;
    io_hdr.dxferp = data;
    io_hdr.dxfer_len = data_size;
    io_hdr.timeout = 5000;

    if (ioctl(fd, SG_IO, &io_hdr) < 0)
    {
        return (false);
    }

    if (SG_INFO_OK != (io_hdr.info & SG_INFO_OK_MASK) && io_hdr.sb_len_wr > 0)
    {
        return (false);
    }

    if (io_hdr.masked_status || io_hdr.host_status || io_hdr.driver_status)
    {
        return (false);
    }

    return (true);
}

static bool get_disk_serial_by_way_2(const std::string &disk_name, std::string &serial_no)
{
    serial_no.clear();

    int fd = open(disk_name.c_str(), O_RDONLY);
    if (-1 == fd)
    {
        return (false);
    }

    int version = 0;
    if (ioctl(fd, SG_GET_VERSION_NUM, &version) < 0 || version < 30000)
    {
        close(fd);
        return (false);
    }

    const unsigned int data_size = 0x00ff;
    unsigned char data[data_size] = {0};
    const unsigned int sense_len = 32;
    unsigned char sense[sense_len] = {0};
    unsigned char cdb[] = {0x12, 0x01, 0x80, 0x00, 0x00, 0x00};
    cdb[3] = (data_size >> 8) & 0xff;
    cdb[4] = (data_size & 0xff);

    if (scsi_io(fd, cdb, sizeof(cdb), SG_DXFER_FROM_DEV, data, data_size, sense, sense_len))
    {
        int page_len = data[3];
        trim_serial(data + 4, page_len, serial_no);
    }

    close(fd);

    return (!serial_no.empty());
}

static bool parse_serial(const char *line, int line_size, const char *match_words, std::string &serial_no)
{
    const char *serial_s = strstr(line, match_words);
    if (NULL == serial_s)
    {
        return (false);
    }
    serial_s += strlen(match_words);
    while (isspace(serial_s[0]))
    {
        ++serial_s;
    }

    const char *serial_e = line + line_size;
    const char *comma = strchr(serial_s, ',');
    if (NULL != comma)
    {
        serial_e = comma;
    }

    while (serial_e > serial_s && isspace(serial_e[-1]))
    {
        --serial_e;
    }

    if (serial_e <= serial_s)
    {
        return (false);
    }

    std::string(serial_s, serial_e).swap(serial_no);

    return (true);
}

static void get_serial(const char *file_name, const char *match_words, std::string &serial_no)
{
    serial_no.c_str();

    std::ifstream ifs(file_name, std::ios::binary);
    if (!ifs.is_open())
    {
        return;
    }

    char line[4096] = {0};
    while (!ifs.eof())
    {
        ifs.getline(line, sizeof(line));
        if (!ifs.good())
        {
            break;
        }

        if (0 == ifs.gcount())
        {
            continue;
        }

        if (parse_serial(line, ifs.gcount() - 1, match_words, serial_no))
        {
            break;
        }
    }

    ifs.close();
}

//--------------------------getCPUID------------------------------------

static bool get_cpu_id_by_asm(std::string &cpu_id)
{
    cpu_id.clear();

    unsigned int s1 = 0;
    unsigned int s2 = 0;
    asm volatile(
        "movl $0x01, %%eax; \n\t"
        "xorl %%edx, %%edx; \n\t"
        "cpuid; \n\t"
        "movl %%edx, %0; \n\t"
        "movl %%eax, %1; \n\t"
        : "=m"(s1), "=m"(s2));

    if (0 == s1 && 0 == s2)
    {
        return (false);
    }

    char cpu[32] = {0};
    snprintf(cpu, sizeof(cpu), "%08X%08X", htonl(s2), htonl(s1));
    std::string(cpu).swap(cpu_id);

    return (true);
}

static void parse_cpu_id(const char *file_name, const char *match_words, std::string &cpu_id)
{
    cpu_id.c_str();

    std::ifstream ifs(file_name, std::ios::binary);
    if (!ifs.is_open())
    {
        return;
    }

    char line[4096] = {0};
    while (!ifs.eof())
    {
        ifs.getline(line, sizeof(line));
        if (!ifs.good())
        {
            break;
        }

        const char *cpu = strstr(line, match_words);
        if (NULL == cpu)
        {
            continue;
        }
        cpu += strlen(match_words);

        while ('\0' != cpu[0])
        {
            if (' ' != cpu[0])
            {
                cpu_id.push_back(cpu[0]);
            }
            ++cpu;
        }

        if (!cpu_id.empty())
        {
            break;
        }
    }

    ifs.close();
}

std::string base64_encode(const char *bytes_to_encode, unsigned int in_len)
{
    std::string ret;
    int i = 0;
    int j = 0;
    unsigned char char_array_3[3]; // store 3 byte of bytes_to_encode
    unsigned char char_array_4[4]; // store encoded character to 4 bytes

    while (in_len--)
    {
        char_array_3[i++] = *(bytes_to_encode++); // get three bytes (24 bits)
        if (i == 3)
        {
            // eg. we have 3 bytes as ( 0100 1101, 0110 0001, 0110 1110) --> (010011, 010110, 000101, 101110)
            char_array_4[0] = (char_array_3[0] & 0xfc) >> 2;                                     // get first 6 bits of first byte,
            char_array_4[1] = ((char_array_3[0] & 0x03) << 4) + ((char_array_3[1] & 0xf0) >> 4); // get last 2 bits of first byte and first 4 bit of second byte
            char_array_4[2] = ((char_array_3[1] & 0x0f) << 2) + ((char_array_3[2] & 0xc0) >> 6); // get last 4 bits of second byte and first 2 bits of third byte
            char_array_4[3] = char_array_3[2] & 0x3f;                                            // get last 6 bits of third byte

            for (i = 0; (i < 4); i++)
                ret += base64_chars[char_array_4[i]];
            i = 0;
        }
    }

    if (i)
    {
        for (j = i; j < 3; j++)
            char_array_3[j] = '\0';

        char_array_4[0] = (char_array_3[0] & 0xfc) >> 2;
        char_array_4[1] = ((char_array_3[0] & 0x03) << 4) + ((char_array_3[1] & 0xf0) >> 4);
        char_array_4[2] = ((char_array_3[1] & 0x0f) << 2) + ((char_array_3[2] & 0xc0) >> 6);

        for (j = 0; (j < i + 1); j++)
            ret += base64_chars[char_array_4[j]];

        while ((i++ < 3))
            ret += '=';
    }

    return ret;
}
//组合getInfo
string getMcInfo()
{
    std::string mac_address;

    if (!get_mac_address(mac_address))

    {
        printf("XX#$%01ma\n");
        exit(-1);
    }

    //ip
    std::string ipAddress;
    char local_ip4[INET_ADDRSTRLEN] = {0};
    if (get_local_ip_using_ifaddrs(local_ip4) == RUN_SUCCESS)
    {
        ipAddress = local_ip4;
    }
    else
    {
        printf("XX#$%02i\n");
        exit(-1);
    }
    //hardDisk
    std::string disk_name;
    std::string disk_id;
    if (get_disk_name(disk_name))
    {

        if (!get_disk_serial_by_way_2(disk_name, disk_id))
        {
            printf("XX#$%03di\n");
        }
    }
    else
    {
        //printf("can't get disk name \n");
        printf("XX#$%02di\n");
    }

    //cpu
    std::string cpu_id;
    if (!get_cpu_id_by_asm(cpu_id))
    {
        printf("XX#$%04cp\n");
        exit(-1);
    }
    //time
    time_t seconds;
    long secondnum = time(&seconds);
    string licenceTime = to_string(secondnum / (3600 * 24));
    string mcInfo = mac_address + ipAddress + disk_id + cpu_id + "," + licenceTime;
    return mcInfo;
}
