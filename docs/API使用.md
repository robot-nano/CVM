# API 使用

<!-- TOC -->

- [API 使用](#api-使用)
  - [AutoTuning](#autotuning)
  - [Android](#android)
  - [RelayIR量化](#relayir量化)
  - [torch导出ONNX](#torch导出onnx)
  - [读取ONNX](#读取onnx)

<!-- /TOC -->

## AutoTuning
pass转换, eg. tflite模型 NHWC->NCHW
```python
# TFlite framework to Relay parser - Default layout is NHWC
mod, params = relay.frontend.from_tflite(tflite_model,
                                         shape_dict=shape_dict,
                                         dtype_dict=dtype_dict)

# We assume our model's heavily-layout sensitive operators only consist of nn.conv2d
desired_layouts = {'nn.conv2d': ['NCHW', 'default']}

# Convert the layout to NCHW
# RemoveUnunsedFunctions is used to clean up the graph.
seq = tvm.transform.Sequential([relay.transform.RemoveUnusedFunctions(),
                                relay.transform.ConvertLayout(desired_layouts)])
with tvm.transform.PassContext(opt_level=3):
    mod = seq(mod)

# Call relay compilation
with relay.build_config(opt_level=3):
     graph, lib, params = relay.build(mod, target, params=params)


###### layout 可以声明为特定格式
desired_layouts = {'nn.conv2d': ['NCHW', 'OIHW']}
pass = relay.transform.ConvertLayout(desired_layouts)
```

## Android
```python
# AutoTuning or RPC remote test 

# GPU
target = tvm.target.create("opencl -device=mali")
target_host = 'llvm -mtriple=aarch64-linux-gnu'

# CPU
target = "llvm -device=arm-cpu -mtriple=arm64-linux-android"
target_host = None
```

## RelayIR量化
```python
def calibrate_dataset():
    val_data, batch_fn = get_val_data()
    val_data.reset()
    for i, batch in enumerate(val_data):
        if i * batch_size >= calibration_samples:
            break
        data, _ = batch_fn(batch)
        yield {"data": data}

def get_model():
    gluon_model = gluon.model_zoo.vision.get_model(model_name, pretrained=True)
    img_size = 299 if model_name == 'inceptionv3' else 224
    data_shape = (batch_size, 3, img_size, img_size)
    mod, params = relay.frontend.from_mxnet(gluon_model, {"data": data_shape})
    return mod, params


# * 使用数据计算出量化边界进行量化
# * 使用全局值进行量化, 可能会有损失

def quantize(mod, params, data_aware):
    if data_aware:
        with relay.quantize.qconfig(calibrate_mode='kl_divergence', weight_scale='max'):
            mod = relay.quantize.quantize(mod, params, dataset=calibrate_dataset())
    else:
        with relay.quantize.qconfig(calibrate_mode='global_scale', global_scale=8.0):
            mod = relay.quantize.quantize(mod, params)
    return mod

```

## torch导出ONNX
```python
output_onnx = "model.onnx"
input_names = ['data']
output_names = ['output0']
inputs = torch.randn(1, 3, size, size).to(device)
torch_out = torch.onnx.export(net, inputs, output_onnx, export_params=True, verbose=False, input_names=input_names, output_names=output_names, opset_version=11)
```

## 读取ONNX
```python
def get_network(input_shape, output_shape, model_dir, input_name="input0"):
    model = onnx.load(model_dir)
    shape_dict = {input_name: input_shape}
    mod, params = relay.frontend.from_onnx(model, shape=shape_dict, opset=11)
    return mod, params, input_shape, output_shape
```