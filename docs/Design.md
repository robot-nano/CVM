# TVM Design

<!-- TOC -->

- [TVM Design](#tvm-design)
  - [three Key modules](#three-key-modules)
  - [Optmizing Computational Graphs](#optmizing-computational-graphs)
  - [Generating Tensor Operations](#generating-tensor-operations)
  - [Automating Optimization](#automating-optimization)
- [AutoTvm](#autotvm)

<!-- /TOC -->

## three Key modules
By combining these three modules, TVM can take model descriptions from existing deep learning frameworks, perform joint high- and low-level optimizations, and generate hardware-specific optimized code for backends, e.g. CPUs, GPUs, and FPGA-based specialized accelerators

1. a tensor expression language  
build operators and provid program `transformation primitives` that generate different versions of the program with various optimizations.(extens Halide's compute/schedule separation concept)by also separating target hardware intrinsics from transformation primitives, which enables support for novel accelerators. Moreover, we introduce new transformation primitives to address a GPU-related challenges and enable deployment to specialized accelerators. We can then apply different sequences of program transformations to form a rich space of valid programs for a given operator declaration.

2. automated program optimization framework  
`find optimized tensor operators`  
The optimizer is guided by an ML-based cost model that adapts and improves as we collect more data from a hardware backend.  
3. a graph rewriter  
`On top of the automatic code generator`  
takes full advantage of high- and operator-level optimizations

## Optmizing Computational Graphs
  High Level Graph Rewriting
  
**Operation Fusion**   
(1) injective (one-to-one map, e.g., add)  
(2) reduction (e.g., sum)  
(3) complexout-fusable (can fuse element-wises map to output, e.g., conv2d)  
(4) opaque (cannot be fused, e.g., sort)

We provide generic rules to fuse these operators, as follows:  
* Multiple injective operators can be fused into another injective operator.  
* A reduction operator can be fused with input injective operators (e.g., fuse scale and sum).
* Operators such as conv2d are complex-out-fusable, we can fuse element-wise operators to its output.    

we can apply these rules to transform the computational graph into a fused version.  

**Data Layout Transformation**
Data layout optimization converts a computational graph into one that can use better internal data layouts for execution on the taraget hardware. It starts by specifying the preferred data layout for each operator given the constraints dictated by memory hierarchies. We then perform the proper layout transformation between a producer and a consumer if their preferred data layouts do not match.

## Generating Tensor Operations
**Tensor Expression and Schedule Space**  
**Nested Parallelism with Cooperation**  
**Tensorization**  
**Explicit Memory Latency Hiding**  

## Automating Optimization
-- automated schedule optimizer  
* a schedule explorer: propose promising new configurations
* a machine learning cost model: predicts the performance of a given configuration

**Schedule Space Specification**  
We build a `schedule template specification` API to let a developer declare knobs in the schedule space. We also created a `generic master template for each hardware back-end` that automatically extracts possible knobs based on the computation description expressed using the tensor expression language.

**ML-Based Cost Model**  


# AutoTvm