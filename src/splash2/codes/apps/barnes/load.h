#line 185 "/afs/andrew.cmu.edu/usr11/bangjiel/private/15740-Computer-Architecture-Final-Project/src/splash2/codes/null_macros/c.m4.null.POSIX_BARRIER"

#line 1 "load.H"
#ifndef _LOAD_H_
#define _LOAD_H_

void maketree(long ProcessId);
cellptr InitCell(cellptr parent, long ProcessId);
leafptr InitLeaf(cellptr parent, long ProcessId);
void printtree(nodeptr n);
nodeptr loadtree(bodyptr p, cellptr root, long ProcessId);
bool intcoord(long xp[NDIM], vector rp);
long subindex(long x[NDIM], long l);
void hackcofm(long ProcessId);
cellptr SubdivideLeaf(leafptr le, cellptr parent, long l, long ProcessId);
cellptr makecell(long ProcessId);
leafptr makeleaf(long ProcessId);


#endif
