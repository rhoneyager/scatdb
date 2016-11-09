#include "defs.h"

#ifdef __cplusplus
extern "C" {
#endif

int little_endian();
void reverse(void*,int);
DLEXPORT_SDBR void liu_scatdb_(float*,float*,int*,float*,float*,float*,float*,float*,float*,float*,int*,int*);
DLEXPORT_SDBR int liu_scatdb(float,float,int,float,float*,float*,float*,float*,float*,float*,int*);
float linear_interp(float,float,float,float,float);
DLEXPORT_SDBR const char* _get_scatdb_location();
DLEXPORT_SDBR void _set_scatdb_location(const char*);
/*
static float fs[NFREQ], ts[NTEMP], szs[NSIZE][NSHAP], abss[NFREQ][NTEMP][NSHAP][NSIZE],
scas[NFREQ][NTEMP][NSHAP][NSIZE],
bscs[NFREQ][NTEMP][NSHAP][NSIZE], gs[NFREQ][NTEMP][NSHAP][NSIZE],
reff[NSIZE][NSHAP], pqs[NFREQ][NTEMP][NSHAP][NSIZE][NQ];
static int shs[NSHAP], mf, mt, msh, msz[NSHAP];
*/
DLEXPORT_SDBR int liu_scatdb_onlyread(
	float *o_fs, float *o_ts, float *o_szs, float* o_abss, float* o_scas,
	float *o_bscs, float* o_gs, float* o_reff, float* o_pqs,
	int* o_shs, int* o_msz, int* o_mf, int* o_mt, int* o_msh);
#ifdef __cplusplus
};
#endif


#define NSHAP		11   /* number of shapes */
#define NTEMP		5    /* number of temps  */
#define NFREQ		22   /* number of frequencies */
#define NSIZE		20   /* max number of sizes */
#define NQ		37   /* number of anges in PF */
