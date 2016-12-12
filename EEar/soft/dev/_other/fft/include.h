
#include <windows.h>
#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <string>
#include <vector>
#include "fdetector.h"

#define MAX(a,b)   ((a)>(b)?(a):(b))
#define MIN(a,b)   ((a)<(b)?(a):(b))

#define CLEAROBJ(o)   memset(&(o),0,sizeof(o))

#define NNS(str)   ((str)?(str):"")

#define SAFEDELETE(obj)   { if ( obj ) delete obj; obj = NULL; }

