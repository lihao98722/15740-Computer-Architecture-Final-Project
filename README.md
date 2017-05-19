## Adaptive Write-Update and Write-Invalidate Cache Coherence Protocols for Producer-Consumer Sharing

This is a final research project for the class of Computer Architecture ([15-740](http://www.cs.cmu.edu/afs/cs/academic/class/15740-s17/www/)), Spring 2017 in the School of Computer Science at Carnegie Mellon University. 

Authors: Bangjie Liu, Hao Li

---

### Cache Simulator

In this project, we implement a Pintool-based configurable cache simulator and develop our adaptive cache coherence protocol and pattern detector. It is able to simulate L1 data cache and  directory-based MSI cache coherence protocols. By tracing memory accesses, it provides a detailed profiler for load/store hit/miss, estimated execution cycles, and network messages for each core and the entire program.


### Evaluation

#### Producer-consumer Application

In this project, we use a [producer-consumer application](https://github.com/lihao98722/15740-Computer-Architecture-Final-Project/tree/master/src/App) to evaluate the performance of directory-based protocols. It is a typical and simplified producer-consumer application in which one thread keeps writing to a shared memory location while several threads keep reading it. And it is expected to produce a large amount of traffics to ensure cache coherence. Note that the shared memory location is "atomic" and the application has **one** writing thread and **(#hyperthreads - 1)** reader threads.

#### Splash-2 Benchmarks

http://www.m5sim.org/Splash_benchmarks
http://www.capsl.udel.edu/splash/index.html

---

### Reference

[1] Dynamic Cache Simulator for Shared Address Space (SAS) Multiprocessor Architectures, Boise State University http://cs.boisestate.edu/~uh/CachePintool.htm)

[2] https://software.intel.com/sites/landingpage/pintool/docs/76991/Pin/html/

[3] Cheng, Liqun, John B. Carter, and Donglai Dai. ”An adaptive cache coherence protocol optimized for producer-consumer sharing.”High Performance Computer Architecture, 2007. HPCA 2007. IEEE 13th International Symposium on. IEEE, 2007.

[4] Singh, J. P. Parallel Hierarchical N-body Methods and Their Implications for Multiprocessors. PhD Thesis, Stanford University, February 1993.

[5] Holt, C. and Singh, J. P. Hierarchical N-Body Methods on Shared Address Space Multiprocessors. SIAM Conference on Parallel Processing for Scientific Computing, Feb 1995, to appear.

[6] Brandt, A. Multi-Level Adaptive Solutions to Boundary-Value Problems. Mathematics of Computation, 31(138):333-390, April 1977.

[7] Woo, S. C., Singh, J. P., and Hennessy, J. L. The Perfor- mance Advantages of Integrating Block Data Transfer in Cache- Coherent Multiprocessors. Proceedings of the 6th International Conference on Architectural Support for Programming Lan- guages and Operating Systems (ASPLOS-VI), October 1994.

[8] https://en.wikipedia.org/wiki/Producer-consumer problem

[9] https://en.wikipedia.org/wiki/N–body problem

[10] http://www.capsl.udel.edu/splash/
