## Adaptive Write-Update and Write-Invalidate Cache Coherence Protocols for Producer-Consumer Sharing

This is a final research project for the class of Computer Architecture ([15-740](http://www.cs.cmu.edu/afs/cs/academic/class/15740-s17/www/)), Spring 2017 in the School of Computer Science at Carnegie Mellon University. 

Authors: Bangjie Liu, Hao Li

---
### How to run

``` cd src/SASCacheSim_DirBased ```

``` sudo chmod +x run.sh && ./run.sh ```


---

### Cache Simulator

In this project, we import a third-party [cache simulator](https://github.com/lihao98722/15740-Computer-Architecture-Final-Project/tree/master/src/SASCacheSim_DirBased)[1] and develop our adaptive cache coherence protocol and pattern detector based on that. It is a Pin-based[2] cache simulation tool that is able to detect cache instructions, memory loads/stores and simulate different cache coherence protocols(Valid/Invalid, MSI, MESI). It supports both bus-based and directory-based write strategies and provides configurable cache structure(L1, L2, L3). By analyzing cache/memory access traces, we can calculate cache hit/miss for load/store and estimate cost by CPU cycles across different cache structures with different coherence protocols and write strategies.

---

### Evaluation

#### Producer-consumer Application

In this project, we use a [producer-consumer application](https://github.com/lihao98722/15740-Computer-Architecture-Final-Project/tree/master/src/App) to evaluate the performance of directory-based protocols. It is a typical and simplified producer-consumer application in which one thread keeps writing to a shared memory location while several threads keep reading it. And it is expected to produce a large amount of traffics to ensure cache coherence. Note that the shared memory location is "atomic" and the application has **one** writing thread and **(#hyperthreads - 1)** reader threads.


---

### Reference

[1] Dynamic Cache Simulator for Shared Address Space (SAS) Multiprocessor Architectures, Boise State University http://cs.boisestate.edu/~uh/CachePintool.htm)
[2] Pin: https://software.intel.com/en-us/articles/pin-a-dynamic-binary-instrumentation-tool
