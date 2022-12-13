'''
@file  generate_key.py
@brief 生成模型加密密钥
@date  

'''

import uuid
import socket
from cryptography.fernet import Fernet


def generate_key_cryptography(license_pth):
    
    '''
    利用Cyptography密码库, 生成密钥
    Cryptography密码库实现了一个集成的对称密码函数, 称之为Fernet
    它可以保证信息无法被篡改和破解
    '''
    
    key = Fernet.generate_key()

    with open(license_pth, 'wb') as fw:
        fw.write(key)

    print(key) 
    
    return key.decode()

def generate_key_mac_ip(license_pth):
    
    '''
    利用mac 地址 或 IP 地址，生成密钥
    可以理解为 对硬件进行了绑定
    '''
    mac = uuid.UUID(int=uuid.getnode()).hex[-12:]
    mac_key = ':'.join([mac[e:e + 2] for e in range(0, 11, 2)])
    
    print("mac: ", mac_key)

    hostname = socket.gethostname()
    address = socket.gethostbyname(hostname)

    print("ip: ", address)
    
    # str→bytes：encode()
    # bytes→str：decode()
    with open(license_pth, 'wb') as fw:
        fw.write(mac_key.encode("utf-8"))

    print(mac_key) 
    
    return mac_key
    
def generate_key_cryptography_mac(license_pth):
    
    '''
    生成密钥与mac地址结合 
    生成新的密钥license
    '''
    key_cryptography = generate_key_cryptography(license_pth)
    key_mac = generate_key_mac_ip(license_pth)
    
    key = key_cryptography + key_mac
    
    with open(license_pth, 'wb') as fw:
        fw.write(key.encode("utf-8"))

    print(key) 
    
    
if __name__=="__main__":
    
    license_pth = "license"
    # generate_key_cryptography(license_pth)
    
    # generate_key_mac_ip(license_pth)
    
    generate_key_cryptography_mac(license_pth)