#!/bin/bash

# install depends
pip install netron onnx

python_path=`python -c "import sys;print(sys.executable)"`

# 修改下 不用系统路径
bin_path=`python -c "import sys;print(sys.exec_prefix)"`/bin
cp lnetron ${bin_path}
sed -i "1i#!${python_path}" ${bin_path}/lnetron
chmod +x ${bin_path}/lnetron

echo Install to ${bin_path}/lnetron, Done. You can try it using \'lnetron xxx.onnx --light\'