'''
@file  model_en_de_crypt.py
@brief 对pytorch训练生成pt权重 或 onnx 模型进行加解密处理
@date  

'''

import io
import torch
import onnx
from cryptography.fernet import Fernet

def read_license(license_pth):
    
    '''
    读取密钥 license
    '''
    
    with open(license_pth, 'rb') as fr:
        key = fr.read()
    return key

def model_encrtpy(license_pth, encrypt_pth, pt_pth=False, onnx_pth=False):
    
    '''
    利用cryptography库 对pt 或 onnx 模型文件 加密
    
    '''
    
    key = read_license(license_pth)
        
    if pt_pth:
        pt_model = torch.load(pt_pth)
        bytes_= io.BytesIO()
        torch.save(pt_model, bytes_)
        bytes_.seek(0)
        
        model_bytes = bytes_.read()
        
        encrypt_data = Fernet(key).encrypt(model_bytes)
        
        with open(encrypt_pth, 'wb') as fw:
            fw.write(encrypt_data)
        
    if onnx_pth:
        pt_model = onnx.load(onnx_pth)
        bytes_= io.BytesIO()
        onnx.save(pt_model, bytes_)
        bytes_.seek(0)
        
        model_bytes = bytes_.read()
        
        encrypt_data = Fernet(key).encrypt(model_bytes)
        
        with open(encrypt_pth, 'wb') as fw:
            fw.write(encrypt_data)
    

def model_decrypt(license_pth, encrypt_pth, pt_pth=False, onnx_pth=False):
    
    '''
    利用cryptography库 对pt 或 onnx 加密模型文件 解密
    
    '''
    with open(encrypt_pth, 'rb') as fr:
        encrypt_data = fr.read()
        
    key = read_license(license_pth)
    
    decrypt_data = Fernet(key).decrypt(encrypt_data)
    
    bytes_  = io.BytesIO(decrypt_data)
    bytes_.seek(0)
    
    if pt_pth:
        
        torch.save(bytes_, pt_pth)
        
    if onnx_pth:
        model = onnx.load(bytes_)
        onnx.save(model, onnx_pth)
    

if __name__=="__main__":
    
    
    # model_encrtpy("license", encrypt_pth="yolov7_encrypt.onnx", pt_pth=False, onnx_pth="yolov7.onnx")
    
    # model_decrypt("license",  encrypt_pth="yolov7_encrypt.onnx", pt_pth=False, onnx_pth="yolov7_decrypt.onnx")
    
    # model_encrtpy("license", encrypt_pth="yolov7_encrypt.pt", pt_pth="yolov7.pt", onnx_pth=False)
    # print("-------------------------------加密完成！！！---------------------------------------")
    
    model_decrypt("license",  encrypt_pth="yolov7_encrypt.pt", pt_pth="yolov7_decrypt.pt", onnx_pth=False)
    
    print("-------------------------------解密完成！！！---------------------------------------")
    