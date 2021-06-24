# RelayIR

<!-- TOC -->

- [RelayIR](#relayir)
  - [设计目标](#设计目标)
  - [与当前计算图IR 术语不同](#与当前计算图ir-术语不同)
  - [Module: Support Multiple Functions (Graphs)](#module-support-multiple-functions-graphs)
  - [Implication of IR Transformations](#implication-of-ir-transformations)
  - [Relay Operator Strategy](#relay-operator-strategy)
    - [Operator Strategy Design](#operator-strategy-design)
    - [写一个 Strategy Function](#写一个-strategy-function)
    - [注册 Strategy Function 到算子中](#注册-strategy-function-到算子中)
    - [新的target注册Strategy](#新的target注册strategy)

<!-- /TOC -->

## 设计目标
* 支持传统数据流模式编程和转换
* 支持functional-style scoping, let-binding 并使其对不同语言功能齐全
* 允许用户混合两种编程方式

---
## 与当前计算图IR 术语不同
* 已知架构使用 graph & subgraph
* Relay 使用 function -- eg.`fn(%x)`, 指明graph

`Each dataflow node is a CallNode in Rely.`

---
## Module: Support Multiple Functions (Graphs)
Rely 相比 NNVM 改进:
* 简洁的文本格式, 易于简化书写过程调试
* First-class support for subgraphs-functions, in a joint module, this enables futher chances of `joint module` such as `inlining` and calling `convention specification`
* Native front-end language interop, for example, all the data structure can be visited in Python, which allows quick prototyping of optimizations in Python and mixing them with C++ code.

---
## Implication of IR Transformations
对编写`passes`影响:  

```待定```

---

## Relay Operator Strategy
*<font color=red>Relay Operator 在TOPI库中实现, 每个Relay Operator 都需要注册 `compute` and `schedule` 函数. 但是针对不同硬件, 每个算子通常有不同的实现方法, 即使对相同硬件, 每个算子也可能有多种算法和实现方式. 针对这种复杂情况, 采用 `operator strategy` 允许用户定义每个算子和不同硬件的底层测略</font>*

### Operator Strategy Design 

Operator strategy 中基本元素是一个`OpImplementation`. 它包含一对`compute and schedule` 函数/名字/优先级(优先级用于从 Op Strategy 选择Implementation)


`OpStrategy` 包含 a list of `OpSpecialization`. 每个 `OpSpecialization` 包含 a list of `OpImplementaion` 关联一个 `SpecializedCondition`, `SepcializedCondtition` 可以为 null, 表示 implementations are generally applicable; 否则, 只有 implementations 只有在特殊条件满足时才被考虑. `SpecializedCondition` consits of a list of clauses defined in Tensor Expression in conjunctive normal form (CNF) and only support conditions on tensor shapes.

最后, a strategy function, or `FTVMStrategy`, 决定哪一对 compute and schedule 函数被用在workload中, 以及需要被注册在 Relay operator中. `FTVMStrategy` 是一个通用函数(include/tvm/target/generic_func.h), 可以被每个目标设备重写, 函数签名是
```cpp
OpStrategy(const Attrs& attrs, const Array<Tensor>& inputs, const Type& out_type, const Target& target)
```

### 写一个 Strategy Function

建议在TOPI中使用Python定义:
1. 定义Compute过程
2. 定义schedule过程
3. 定义包装函数, 以符合函数签名 (`FTVMCompute` and `FTVMSchedule` in `include/tvm/relay/op_attr_types.h`)
<font color=red>通常情况下需要对每一个算子写一个用户定义的 **compute wrapper function**, 以从op attributes 获取不同字段 </font>
```python
# 定义 strategy (已经定义 compute/schedule/wrap function)
# add to python/tvm/relay/op/strategy/generic.py

@override_native_generic_func("topk_strategy")
def topk_strategy(attrs, inputs, out_type, target):
  strategy = _op.OpStrategy()
  strategy.add_implementation(
    wrap_compute_topk(topi.topk),
    wrap_topi_schedule(topi.generic.schedule_topk),
    name="topk.generic")
  return strategy

# 添加到每个target硬件定义中 (generic下定义策略和通用计算调度, 
# 不同的硬件需要定义不同的 compute 和 schedule 调度过程, 
# 用来对算子实现与硬件对应的autoTVM调度优化和加速)

@topk_strategy.register(["cuda", "gpu"])
def topk_strategy_cuda(attrs, inputs, out_type, target):
  strategy = _op.OpStrategy()
  strategy.add_implementation(
    wrap_compute_my_new_op(topi.cuda.topk),
    wrap_topi_schedule(topi.cuda.schedule_topk),
    name="topk.cuda")
  return strategy

# 策略中可以包含一些 SpecialedCondition, 以及一些第三方库, 
# 只有当特定条件满足时, 策略中的特殊计算和调度过程才会被编译
# 具体参考手册 https://tvm.apache.org/docs/dev/relay_op_strategy.html#write-a-strategy-function 和 TVM 源码
```

###   注册 Strategy Function 到算子中
```python
# 查看TVM源码注册位置 (暂时没研究注册过程)
register_strategy("topk", strategy.topk_strategy)
```
1. `简单算子有一些预定义注册过程`:  
算子是: 内部映射\广播\归约 模式, 调用`register_injective_schedule`, `register_broadcast_schedule`, `register_reduce_schedule` 这些模式的schedule function 针对每个target设备已经被注册了, 可以被用在这些算子中. 我们假定 compute function 对每个target 设备相同, 
2. 对于没有这些 common patterns 的算子, 但对于所有target有相同compute function, 使用`register_schedule` API. 对于只用 schedule function, 写 `FTVMSchedule` 更简单
```python
# add to python/tvm/relay/op/strategy/generic.py

@generic_func
def schedule_pool(attrs, outs, target):
  with target:
    return topi.generic.schedule_pool(outs, attrs.layout)

# add to each target file in python/tvm/relay/op/strategy. eg. x86.py, cuda.py, etc.
@schedule_pool.register("cpu")
def schedule_pool(attrs, outs, target):
  ...
```

### 新的target注册Strategy
https://tvm.apache.org/docs/dev/relay_op_strategy.html#write-a-strategy-function