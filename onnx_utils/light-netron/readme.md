# Light Netron
- This repo enables lightweighting of onnx file by adding the --light parameter, speeding up the efficiency of previewing with a web browser

# Get Start
- 1. Install netron and lnetron
    - `bash install.sh`
- 2. On official netron version
    - `netron demo.onnx --nolight`
- 3. On light netron version
    - `lnetron demo.onnx`

# Example
```bash
$ lnetron ptq-mse.onnx --light
Serving lightweight mode. original length 143.99 MB, lightweight length 115.71 KB
Serving 'ptq-mse.onnx' at http://localhost:8080
```
