#line 185 "/afs/andrew.cmu.edu/usr11/bangjiel/private/15740-Computer-Architecture-Final-Project/src/splash2/codes/null_macros/c.m4.null.POSIX_BARRIER"

#line 1 "decs.H"
/*************************************************************************/
/*                                                                       */
/*  Copyright (c) 1994 Stanford University                               */
/*                                                                       */
/*  All rights reserved.                                                 */
/*                                                                       */
/*  Permission is given to use, copy, and modify this software for any   */
/*  non-commercial purpose as long as this copyright notice is not       */
/*  removed.  All other uses, including redistribution in whole or in    */
/*  part, are forbidden without prior written permission.                */
/*                                                                       */
/*  This software is provided with absolutely no warranty and no         */
/*  support.                                                             */
/*                                                                       */
/*************************************************************************/

#define MASTER            0
#define RED_ITER          0
#define BLACK_ITER        1
#define UP                0
#define DOWN              1
#define LEFT              2
#define RIGHT             3
#define UPLEFT            4
#define UPRIGHT           5
#define DOWNLEFT          6
#define DOWNRIGHT         7
#define PAGE_SIZE      4096

struct multi_struct {
   double err_multi;
};

extern struct multi_struct *multi;

struct global_struct {
   long id;
   long starttime;
   long trackstart;
   double psiai;
   double psibi;
};

extern struct global_struct *global;

extern double eig2;
extern double ysca;
extern long jmm1;
extern double pi;
extern double t0;

extern double ****psi;
extern double ****psim;
extern double ***psium;
extern double ***psilm;
extern double ***psib;
extern double ***ga;
extern double ***gb;
extern double ****work1;
extern double ***work2;
extern double ***work3;
extern double ****work4;
extern double ****work5;
extern double ***work6;
extern double ****work7;
extern double ****temparray;
extern double ***tauz;
extern double ***oldga;
extern double ***oldgb;
extern double *f;
extern double ****q_multi;
extern double ****rhs_multi;

struct locks_struct {
   pthread_mutex_t (idlock);
   pthread_mutex_t (psiailock);
   pthread_mutex_t (psibilock);
   pthread_mutex_t (donelock);
   pthread_mutex_t (error_lock);
   pthread_mutex_t (bar_lock);
};

extern struct locks_struct *locks;

struct bars_struct {
#if defined(MULTIPLE_BARRIERS)
   
#line 87
pthread_barrier_t	(iteration);
#line 87

   
#line 88
pthread_barrier_t	(gsudn);
#line 88

   
#line 89
pthread_barrier_t	(p_setup);
#line 89

   
#line 90
pthread_barrier_t	(p_redph);
#line 90

   
#line 91
pthread_barrier_t	(p_soln);
#line 91

   
#line 92
pthread_barrier_t	(p_subph);
#line 92

   
#line 93
pthread_barrier_t	(sl_prini);
#line 93

   
#line 94
pthread_barrier_t	(sl_psini);
#line 94

   
#line 95
pthread_barrier_t	(sl_onetime);
#line 95

   
#line 96
pthread_barrier_t	(sl_phase_1);
#line 96

   
#line 97
pthread_barrier_t	(sl_phase_2);
#line 97

   
#line 98
pthread_barrier_t	(sl_phase_3);
#line 98

   
#line 99
pthread_barrier_t	(sl_phase_4);
#line 99

   
#line 100
pthread_barrier_t	(sl_phase_5);
#line 100

   
#line 101
pthread_barrier_t	(sl_phase_6);
#line 101

   
#line 102
pthread_barrier_t	(sl_phase_7);
#line 102

   
#line 103
pthread_barrier_t	(sl_phase_8);
#line 103

   
#line 104
pthread_barrier_t	(sl_phase_9);
#line 104

   
#line 105
pthread_barrier_t	(sl_phase_10);
#line 105

   
#line 106
pthread_barrier_t	(error_barrier);
#line 106

#else
   
#line 108
pthread_barrier_t	(barrier);
#line 108

#endif
};

extern struct bars_struct *bars;

extern double factjacob;
extern double factlap;

struct Global_Private {
  char pad[PAGE_SIZE];
  long *rel_num_x;
  long *rel_num_y;
  long *eist;
  long *ejst;
  long *oist;
  long *ojst;
  long *rlist;
  long *rljst;
  long *rlien;
  long *rljen;
  long rownum;
  long colnum;
  long neighbors[8];
  double multi_time;
  double total_time;
};

extern struct Global_Private *gp;

extern double *i_int_coeff;
extern double *j_int_coeff;
extern long xprocs;
extern long yprocs;

extern long numlev;
extern long *imx;
extern long *jmx;
extern double *lev_res;
extern double *lev_tol;
extern double maxwork;
extern long *xpts_per_proc;
extern long *ypts_per_proc;
extern long minlevel;
extern double outday0;
extern double outday1;
extern double outday2;
extern double outday3;

extern long nprocs;
extern double h1;
extern double h3;
extern double h;
extern double lf;
extern double res;
extern double dtau;
extern double f0;
extern double beta;
extern double gpr;
extern long im;
extern long jm;
extern long do_stats;
extern long do_output;
extern long *multi_times;
extern long *total_times;

/*
 * jacobcalc.C
 */
void jacobcalc(double ***x, double ***y, double ***z, long pid, long firstrow, long lastrow, long firstcol, long lastcol);

/*
 * jacobcalc2.C
 */
void jacobcalc2(double ****x, double ****y, double ****z, long psiindex, long pid, long firstrow, long lastrow, long firstcol, long lastcol);

/*
 * laplacalc.C
 */
void laplacalc(long procid, double ****x, double ****z, long psiindex, long firstrow, long lastrow, long firstcol, long lastcol);

/*
 * linkup.C
 */
void link_all(void);
void linkup(double **row_ptr);
void link_multi(void);

/*
 * main.C
 */
long log_2(long number);
void printerr(char *s);

/*
 * multi.C
 */
void multig(long my_id);
void relax(long k, double *err, long color, long my_num);
void rescal(long kf, long my_num);
void intadd(long kc, long my_num);
void putz(long k, long my_num);
void copy_borders(long k, long pid);
void copy_rhs_borders(long k, long procid);
void copy_red(long k, long procid);
void copy_black(long k, long procid);

/*
 * slave1.C
 */
void slave(void);

/*
 * slave2.C
 */
void slave2(long procid, long firstrow, long lastrow, long numrows, long firstcol, long lastcol, long numcols);

/*
 * subblock.C
 */
void subblock(void);
