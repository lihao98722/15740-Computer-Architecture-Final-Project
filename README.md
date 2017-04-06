## 15740-Computer-Architecture-Final-Project

---
### Placeholder for Cache Simulator

Put info about cache simulator here!

#### How to run

``` cd src/SASCacheSim_DirBased ```

``` chmod +x run.sh && ./run.sh ```


---

### Evaluation

#### Producer-consumer Application

In this project, we use a [producer-consumer application](https://github.com/lihao98722/15740-Computer-Architecture-Final-Project/tree/master/src/App) to evaluate the performance of directory-based protocols. It is a typical and simplified producer-consumer application in which one thread keeps writing to a shared memory location while several threads keep reading it. And it is expected to produce a large amount of traffics to ensure cache coherence. Note that the shared memory location is "atomic" and the application has **one** writing thread and **(#hyperthreads - 1)** reader threads.


---

### Reference

 This project uses an online cache simulator developed by Boise State University. (Dynamic Cache Simulator for Shared Address Space (SAS) Multiprocessor Architectures http://cs.boisestate.edu/~uh/CachePintool.htm)
