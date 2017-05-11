#line 185 "/afs/andrew.cmu.edu/usr11/bangjiel/private/15740-Computer-Architecture-Final-Project/src/splash2/codes/null_macros/c.m4.null.POSIX_BARRIER"

#line 1 "getparam.H"
#ifndef _GETPARAM_H_
#define _GETPARAM_H_

void initparam(string *defv);
string getparam(string name);
long getiparam(string name);
long getlparam(string name);
bool getbparam(string name);
double getdparam(string name);
long scanbind(string bvec[], string name);
bool matchname(string bind, string name);
string extrvalue(string arg);

#endif
