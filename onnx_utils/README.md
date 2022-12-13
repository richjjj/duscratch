
## 1. ONNX 模型 shape 推理
onnx模型如果缺少中间节点的shape信息，可以用onnx_infer_shape 进行shape推理，此脚本来自 [onnxruntime](https://github.com/microsoft/onnxruntime/blob/main/onnxruntime/python/tools/symbolic_shape_infer.py)。

```
python onnx_infer_shape.py --input model.onnx --output new_model.onnx
```

## 2. 剪裁ONNX模型
若只需要模型的一部分，可用prune_onnx_model剪裁模型。如只需要模型中的输出`x`和`y`及其之间的节点：
```
python prune_onnx_model.py --model model.onnx --output_names x y --save_file new_model.onnx
```
其中`output_names`用于指定最终模型的输出tensor，可以指定多个

## 3. 修改模型中间节点命名（包含输入、输出重命名）

```
python rename_onnx_model.py --model model.onnx --origin_names x y z --new_names x1 y1 z1 --save_file new_model.onnx
```

其中`origin_names`和`new_names`，前者表示原模型中各个命名（可指定多个），后者表示新命名，两个参数指定的命名个数需要相同

## 3. onnx模型额外信息保存和解析

- 读取extr信息

    reference：[metadata_extractor.cpp](https://gist.github.com/tomdol/8dae97218a8a9e56cb12919dd3e026d4)

- 将一些imagesize 字典等信息保存到onnx模型
``` python
# 添加meta信息
import onnx

model = onnx.load_model('/path/to/model.onnx')
meta = model.metadata_props.add()
meta.key = 'dictionary'
meta.value = open('/path/to/ppocr_keys_v1.txt', 'r', -1, 'u8').read()

meta = model.metadata_props.add()
meta.key = 'shape'
meta.value = '[3,48,320]'

onnx.save_model(model, '/path/to/model.onnx')

# 获取meta信息
import json
import onnxruntime as ort

sess = ort.InferenceSession('/path/to/model.onnx')
metamap = sess.get_modelmeta().custom_metadata_map
chars = metamap['dictionary'].splitlines()
input_shape = json.loads(metamap['shape'])
```
