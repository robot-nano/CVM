# TVM Runtime

<!-- TOC -->

- [TVM Runtime](#tvm-runtime)
  - [两个主要模块](#两个主要模块)
  - [获取ONNX 模型](#获取onnx-模型)
  - [RPC AutoTuning (Android, ios ?)](#rpc-autotuning-android-ios-)

<!-- /TOC -->

---
## 两个主要模块
$\begin{cases}
 TVM \ compiler, \ compilation \ and \ optimization \\
 TVM \ runtime , \ runs on the target device
\end{cases}$  

所以, 不需要编译所有TVM代码部署到设备上. 只需要在桌面机上构建TVM环境, 使用交叉编译将模型部署到设备上.

---
## 获取ONNX 模型
```python
def get_network(input_shape, output_shape, target,
                model_dir,
                input_name="input0"):
    input_shape = (batch_size, 3, 320, 320)
    output_shape = [(batch_size, 4200, 2), (batch_size, 4200, 4), (batch_size, 4200, 10)]
    model = onnx.load(model_dir)
    shape_dict = {input_name: input_shape}
    mod, params = relay.frontend.from_onnx(model, shape=shape_dict, opset=11)
    return mod, params, input_shape, output_shape
```

---
## RPC AutoTuning (Android, ios ?)
```bash
1. 编译TVM RPC Android 软件, 参考 '编译.md'
2. 运行 rpc
# 主机 开启rpc-tracker 和 查询
python -m tvm.exec.rpc_tracker --host=0.0.0.0 --port=9190
python -m tvm.exec.query_rpc_tracker --host=0.0.0.0 --port=9190
# app上运行 tvm-rpc-tracker, 指定主机ip,端口 和 AutoTuning程序中要用到的 name
```

RPC Remote AutoTuning OpenCL:  

```python
# Note: cpu差不多, target改一下. 可以参看tvm tutorials  
# Note: 需要指定TVM ndk编译 export TVM_NDK_CC=/opt/android-toolchain-arm64/bin/aarch64-linux-android-g++


"""
Auto-tuning a convolutional network for Mobile GPU
==================================================
**Author**: `Lianmin Zheng <https://github.com/merrymercy>`_, `Eddie Yan <https://github.com/eqy>`_

Auto-tuning for a specific device is critical for getting the best
performance. This is a tutorial about how to tune a whole convolutional
network.

The operator implementation for Mobile GPU in TVM is written in template form.
The template has many tunable knobs (tile factor, vectorization, unrolling, etc).
We will tune all convolution, depthwise convolution and dense operators
in the neural network. After tuning, we produce a log file which stores
the best knob values for all required operators. When the TVM compiler compiles
these operators, it will query this log file to get the best knob values.

We also released pre-tuned parameters for some arm devices. You can go to
`Mobile GPU Benchmark <https://github.com/apache/incubator-tvm/wiki/Benchmark#mobile-gpu>`_
to see the results.
"""

######################################################################
# Install dependencies
# --------------------
# To use the autotvm package in tvm, we need to install some extra dependencies.
# (change "3" to "2" if you use python2):
#
# .. code-block:: bash
#
#   pip3 install --user psutil xgboost tornado
#
# To make TVM run faster during tuning, it is recommended to use cython
# as FFI of tvm. In the root directory of tvm, execute
# (change "3" to "2" if you use python2):
#
# .. code-block:: bash
#
#   pip3 install --user cython
#   sudo make cython3
#
# Now return to python code. Import packages.

import os

import numpy as np

import tvm
from tvm import te
from tvm import autotvm
from tvm import relay
import tvm.relay.testing
from tvm.autotvm.tuner import XGBTuner, GATuner, RandomTuner, GridSearchTuner
from tvm.contrib.util import tempdir
import tvm.contrib.graph_runtime as runtime

#################################################################
# Define network
# --------------
# First we need to define the network in relay frontend API.
# We can load some pre-defined network from :code:`relay.testing`.
# We can also load models from MXNet, ONNX and TensorFlow.

def get_network(name, batch_size):
    """Get the symbol definition and random weight of a network"""
    input_shape = (batch_size, 3, 224, 224)
    output_shape = (batch_size, 1000)

    if "resnet" in name:
        n_layer = int(name.split('-')[1])
        mod, params = relay.testing.resnet.get_workload(num_layers=n_layer, batch_size=batch_size, dtype=dtype)
    elif "vgg" in name:
        n_layer = int(name.split('-')[1])
        mod, params = relay.testing.vgg.get_workload(num_layers=n_layer, batch_size=batch_size, dtype=dtype)
    elif name == 'mobilenet':
        mod, params = relay.testing.mobilenet.get_workload(batch_size=batch_size, dtype=dtype)
    elif name == 'squeezenet_v1.1':
        mod, params = relay.testing.squeezenet.get_workload(batch_size=batch_size, version='1.1', dtype=dtype)
    elif name == 'inception_v3':
        input_shape = (1, 3, 299, 299)
        mod, params = relay.testing.inception_v3.get_workload(batch_size=batch_size, dtype=dtype)
    elif name == 'mxnet':
        # an example for mxnet model
        from mxnet.gluon.model_zoo.vision import get_model
        block = get_model('resnet18_v1', pretrained=True)
        mod, params = relay.frontend.from_mxnet(block, shape={'data': input_shape}, dtype=dtype)
        net = mod["main"]
        net = relay.Function(net.params, relay.nn.softmax(net.body), None, net.type_params, net.attrs)
        mod = tvm.IRModule.from_expr(net)
    else:
        raise ValueError("Unsupported network: " + name)

    return mod, params, input_shape, output_shape


#################################################################
# Start RPC Tracker
# -----------------
# TVM uses RPC session to communicate with ARM boards.
# During tuning, the tuner will send the generated code to the board and
# measure the speed of code on the board.
#
# To scale up the tuning, TVM uses RPC Tracker to manage distributed devices.
# The RPC Tracker is a centralized master node. We can register all devices to
# the tracker. For example, if we have 10 phones, we can register all of them
# to the tracker, and run 10 measurements in parallel, accelerating the tuning process.
#
# To start an RPC tracker, run this command on the host machine. The tracker is
# required during the whole tuning process, so we need to open a new terminal for
# this command:
#
# .. code-block:: bash
#
#   python -m tvm.exec.rpc_tracker --host=0.0.0.0 --port=9190
#
# The expected output is
#
# .. code-block:: bash
#
#   INFO:RPCTracker:bind to 0.0.0.0:9190

#################################################################
# Register devices to RPC Tracker
# -----------------------------------
# Now we can register our devices to the tracker. The first step is to
# build the TVM runtime for the ARM devices.
#
# * For Linux:
#   Follow this section :ref:`build-tvm-runtime-on-device` to build
#   the TVM runtime on the device. Then register the device to tracker by
#
#   .. code-block:: bash
#
#     python -m tvm.exec.rpc_server --tracker=[HOST_IP]:9190 --key=rk3399
#
#   (replace :code:`[HOST_IP]` with the IP address of your host machine)
#
# * For Android:
#   Follow this `readme page <https://github.com/apache/incubator-tvm/tree/master/apps/android_rpc>`_ to
#   install TVM RPC APK on the android device. Make sure you can pass the android RPC test.
#   Then you have already registered your device. During tuning, you have to go to developer option
#   and enable "Keep screen awake during changing" and charge your phone to make it stable.
#
# After registering devices, we can confirm it by querying rpc_tracker
#
# .. code-block:: bash
#
#   python -m tvm.exec.query_rpc_tracker --host=0.0.0.0 --port=9190
#
# For example, if we have 2 Huawei mate10 pro, 11 Raspberry Pi 3B and 2 rk3399,
# the output can be
#
# .. code-block:: bash
#
#    Queue Status
#    ----------------------------------
#    key          total  free  pending
#    ----------------------------------
#    mate10pro    2      2     0
#    rk3399       2      2     0
#    rpi3b        11     11    0
#    ----------------------------------
#
# You can register multiple devices to the tracker to accelerate the measurement in tuning.

###########################################
# Set Tuning Options
# ------------------
# Before tuning, we should apply some configurations. Here I use an RK3399 board
# as example. In your setting, you should modify the target and device_key accordingly.
# set :code:`use_android` to True if you use android phone.

#### DEVICE CONFIG ####

target = tvm.target.create('opencl -device=mali')

# Replace "aarch64-linux-gnu" with the correct target of your board.
# This target host is used for cross compilation. You can query it by :code:`gcc -v` on your device.
target_host = 'llvm -mtriple=aarch64-linux-gnu'

# Also replace this with the device key in your tracker
device_key = 'android'

# Set this to True if you use android phone
use_android = True

#### TUNING OPTION ####
network = 'resnet-18'
log_file = "./%s.%s.log" % (device_key, network)
dtype = 'float32'

tuning_option = {
    'log_filename': log_file,

    'tuner': 'xgb',
    'n_trial': 1000,
    'early_stopping': 450,

    'measure_option': autotvm.measure_option(
        builder=autotvm.LocalBuilder(
            build_func='ndk' if use_android else 'default'),
        runner=autotvm.RPCRunner(
            device_key, host='0.0.0.0', port=9190,
            number=10,
            timeout=5,
        ),
    ),
}

####################################################################
#
# .. note:: How to set tuning options
#
#   In general, the default values provided here work well.
#   If you have enough time budget, you can set :code:`n_trial`, :code:`early_stopping` larger,
#   which makes the tuning run longer.
#   If your device runs very slow or your conv2d operators have many GFLOPs, considering to
#   set timeout larger.
#

###################################################################
# Begin Tuning
# ------------
# Now we can extract tuning tasks from the network and begin tuning.
# Here, we provide a simple utility function to tune a list of tasks.
# This function is just an initial implementation which tunes them in sequential order.
# We will introduce a more sophisticated tuning scheduler in the future.

# You can skip the implementation of this function for this tutorial.
def tune_tasks(tasks,
               measure_option,
               tuner='xgb',
               n_trial=1000,
               early_stopping=None,
               log_filename='tuning.log',
               use_transfer_learning=True):
    # create tmp log file
    tmp_log_file = log_filename + ".tmp"
    if os.path.exists(tmp_log_file):
        os.remove(tmp_log_file)

    for i, tsk in enumerate(reversed(tasks)):
        prefix = "[Task %2d/%2d] " % (i+1, len(tasks))

        # create tuner
        if tuner == 'xgb' or tuner == 'xgb-rank':
            tuner_obj = XGBTuner(tsk, loss_type='rank')
        elif tuner == 'ga':
            tuner_obj = GATuner(tsk, pop_size=50)
        elif tuner == 'random':
            tuner_obj = RandomTuner(tsk)
        elif tuner == 'gridsearch':
            tuner_obj = GridSearchTuner(tsk)
        else:
            raise ValueError("Invalid tuner: " + tuner)

        if use_transfer_learning:
            if os.path.isfile(tmp_log_file):
                tuner_obj.load_history(autotvm.record.load_from_file(tmp_log_file))

        # do tuning
        tsk_trial = min(n_trial, len(tsk.config_space))
        tuner_obj.tune(n_trial=tsk_trial,
                       early_stopping=early_stopping,
                       measure_option=measure_option,
                       callbacks=[
                           autotvm.callback.progress_bar(tsk_trial, prefix=prefix),
                           autotvm.callback.log_to_file(tmp_log_file)
                       ])

    # pick best records to a cache file
    autotvm.record.pick_best(tmp_log_file, log_filename)
    os.remove(tmp_log_file)


########################################################################
# Finally, we launch tuning jobs and evaluate the end-to-end performance.

def tune_and_evaluate(tuning_opt):
    # extract workloads from relay program
    print("Extract tasks...")
    mod, params, input_shape, _ = get_network(network, batch_size=1)
    tasks = autotvm.task.extract_from_program(mod["main"],
                                              target=target,
                                              target_host=target_host,
                                              params=params,
                                              ops=(relay.op.get("nn.conv2d"),))

    # run tuning tasks
    print("Tuning...")
    tune_tasks(tasks, **tuning_opt)

    # compile kernels with history best records
    with autotvm.apply_history_best(log_file):
        print("Compile...")
        with tvm.transform.PassContext(opt_level=3):
            graph, lib, params = relay.build_module.build(
                mod, target=target, params=params, target_host=target_host)
        # export library
        tmp = tempdir()
        if use_android:
            from tvm.contrib import ndk
            filename = "./net.so"
            lib.export_library(filename, ndk.create_shared)
        else:
            filename = "./net.tar"
            lib.export_library(filename)

        # upload module to device
        print("Upload...")
        remote = autotvm.measure.request_remote(device_key, '0.0.0.0', 9190,
                                                timeout=10000)
        remote.upload(filename)
        rlib = remote.load_module(filename)

        # upload parameters to device
        ctx = remote.context(str(target), 0)
        module = runtime.create(graph, rlib, ctx)
        data_tvm = tvm.nd.array((np.random.uniform(size=input_shape)).astype(dtype))
        module.set_input('data', data_tvm)
        module.set_input(**params)

        # evaluate
        print("Evaluate inference time cost...")
        ftimer = module.module.time_evaluator("run", ctx, number=1, repeat=30)
        prof_res = np.array(ftimer().results) * 1000  # convert to millisecond
        print("Mean inference time (std dev): %.2f ms (%.2f ms)" %
              (np.mean(prof_res), np.std(prof_res)))


tune_and_evaluate(tuning_option)

```