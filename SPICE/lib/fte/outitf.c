/**********
Copyright 1990 Regents of the University of California.  All rights reserved.
Author: 1988 Wayne A. Christopher, U. C. Berkeley CAD Group 
**********/

/*
 * This module replaces the old "writedata" routines in nutmeg.
 * Unlike the writedata routines, the OUT routines are only called by
 * the simulator routines, and only call routines in nutmeg.  The rest
 * of nutmeg doesn't deal with OUT at all.
 */

#include "spice.h"
#include "misc.h"
#include "cpdefs.h"
#include "ftedefs.h"
#include "ftedata.h"
#include "fteconst.h"
#include "inpdefs.h"        /* for INPtables */
#include "ifsim.h"
#include "jobdefs.h"
#include "iferrmsg.h"
#include "suffix.h"

extern void gr_end_iplot();
extern SPICEanalysis *analInfo[];

#define DOUBLE_PRECISION    15

typedef struct dataDesc {
    char *name;     /* The name of the vector. */
    int type;       /* The type. */
    int	gtype;		/* default plot scale */
    bool regular;       /* Is this given to us? */
    int outIndex;       /* If regular then the index. */
    char *specName;     /* The device name if special. */
    char *specParamName;    /* The parameter name if special. */
    int specIndex;      /* For sensitivity, if special. */
    int specType;
    GENERIC *specFast;
    int refIndex;       /* The index of our ref vector. */
    struct dvec *vec;
} dataDesc;

typedef struct runDesc {
    GENERIC *analysis;
    GENERIC *circuit;
    char *name;
    char *type;
    int numData;
    int refIndex;
    dataDesc *data;
    bool writeOut;
    bool windowed;
    bool binary;
    struct plot *runPlot;
    FILE *fp;
    long pointPos;          /* where to write pointCount */
    int pointCount;
    int isComplex;
    int windowCount;
} runDesc;

static int beginPlot();
static int addDataDesc();
static int addSpecialDesc();
static void fileInit();
static void fileInit_pass2( );
static void fileStartPoint();
static void fileAddRealValue();
static void fileAddComplexValue();
static void fileEndPoint();
static void fileEnd();
static void plotInit();
static void plotAddRealValue();
static void plotAddComplexValue();
static void plotEnd();
static bool parseSpecial();
static bool name_eq();
static bool getSpecial();
static void freeRun();

static bool shouldstop = false; /* Tell simulator to stop next time it asks. */
static bool printinfo = false;	/* Print informational "error messages". */



/*
	This function describes the plot to be produced to the output package. ckt is the circuit pointer
	used by the front end to identify the circuit to the simulator. analUid and analysisPtr should serve to uniquely identify
	the analysis to the front end and thus allow access to any additional information it may have
	about the plot. A NULL IFuid (i.e. (IFuid) 0) indicates that the corresponding data point is not interesting and should not be saved The IFuid refName MAY
	appear in the dataNames array, but does not need to. If refName does appear in dataNames, then the
	two values provided for it in the OUTpData call MUST be the same or the results will be unpredictable.
	dataType. 
*/
///<param name ="*ckt"> The circuit to be operated on <param>
///<param name = "*analysisPtr"> An analysis pointer as returned to the front end in a previous call to the simulator�s newhalysis function </param>
///<param name = "analUid"> The unique identifier for the analysis that analysisPtr refes to </param>
///<param name = "refName"> The IFuid of the slowest varying reference or independent variable of the plot, or is NULL in the case of a single point output, such as an operating point </param>
///<param name = "refType"> The type (as used in the IFparm structure) of the reference variable </param>
///<param name = "numNames"> The number of variables named in the dataNames array </param>
///<param name = "*dataNames"> An array of the unique identifiers which are associated with the data to be output in the vectors handed to OUTpData </param>
///<param name = "dataType"> The type (as used in the Warm mcmre) of all of the values to be retumed by OUTpData(without the IF_VECTOR attribute) </param>
///<param name = "**plotPtr"> An identifier created by the back end to identify the plot, and will be supplied by the simulator in all future OUT* calls regarding this plot </param>
int OUTpBeginPlot(GENERIC *circuitPtr, GENERIC *analysisPtr, IFuid analName, IFuid refName, int refType, int numNames, IFuid *dataNames, int dataType, GENERIC **plotPtr)
{
	char *name;

	if (ft_curckt->ci_ckt == circuitPtr)
		name = ft_curckt->ci_name;
	else
		name = "circuit name";

	return (beginPlot(analysisPtr, circuitPtr, name,
		(char *)analName, (char *)refName, refType, numNames,
		(char **)dataNames, dataType, false,
		(runDesc **)plotPtr));
}




/*
	This function begins a windowed output plot. The arguments are the same as for OUTpBegin-
	Plot, but the data for the plot will be delivered using the windowing output functions instead of the
	pointwise functions. The vector dataNames is provided for the convenience of the output package, so
	that all the dependent variables are known in advance. Also, the dependent variables are referred to
	using their index in this vector by calls to the function OWData
*/
int OUTwBeginPlot(GENERIC *circuitPtr, GENERIC *analysisPtr, IFuid analName, IFuid refName, int refType, int numNames, IFuid *dataNames, int dataType, GENERIC **plotPtr)
{
	return (beginPlot(analysisPtr, circuitPtr, "circuit name",
		(char *)analName, (char *)refName, refType, numNames,
		(char **)dataNames, dataType, true,
		(runDesc **)plotPtr));
}

static int
beginPlot(analysisPtr, circuitPtr, cktName, analName, refName, refType,
        numNames, dataNames, dataType, windowed, runp)
    GENERIC *analysisPtr;
    GENERIC *circuitPtr;
    char *cktName;
    char *analName;
    char *refName;
    int refType;
    int numNames;
    char **dataNames;
    int dataType;
    bool windowed;
    runDesc **runp;
{
    runDesc *run;
    struct save_info *saves;
    bool *savesused = NULL;
    int numsaves;
    int i, j, depind;
    char namebuf[BSIZE_SP], parambuf[BSIZE_SP], depbuf[BSIZE_SP];
    bool saveall = true;
    char *an_name;

    /* Check to see if we want to print informational data. */
    if (cp_getvar("printinfo", VT_BOOL, (char *) &printinfo))
	fprintf(cp_err, "(debug printing enabled)\n");

    *runp = run = alloc(struct runDesc);

    /* First fill in some general information. */
    run->analysis = analysisPtr;
    run->circuit = circuitPtr;
    run->name = copy(cktName);
    run->type = copy(analName);
    run->windowed = windowed;
    run->numData = 0;

    an_name = analInfo[((JOB *) analysisPtr)->JOBtype]->public.name;

    /* Now let's see which of these things we need.  First toss in the
     * reference vector.  Then toss in anything that getSaves() tells
     * us to save that we can find in the name list.  Finally unpack
     * the remaining saves into parameters.
     */
    numsaves = ft_getSaves(&saves);
    if (numsaves) {
        savesused = (bool *) tmalloc(sizeof (bool) * numsaves);
        saveall = false;
        for (i = 0; i < numsaves; i++) {
            if (saves[i].analysis && !cieq(saves[i].analysis, an_name)) {
		/* ignore this one this time around */
		savesused[i] = true;
		continue;
	    }
	    if (cieq(saves[i].name, "all")) {
                saveall = true;
                savesused[i] = true;
		saves[i].used = 1;
                continue;
            }
	}
    }

    /* Pass 0. */
    if (refName) {
        addDataDesc(run, refName, refType, -1);
        for (i = 0; i < numsaves; i++)
            if (!savesused[i] && name_eq(saves[i].name, refName)) {
                savesused[i] = true;
		saves[i].used = 1;
            }
    } else {
        run->refIndex = -1;
    }


    /* Pass 1. */
    if (numsaves && !saveall) {
        for (i = 0; i < numsaves; i++) {
	    if (!savesused[i]) {
		for (j = 0; j < numNames; j++) {
		    if (name_eq(saves[i].name, dataNames[j])) {
			addDataDesc(run, dataNames[j], dataType, j);
			savesused[i] = true;
			saves[i].used = 1;
			break;
		    }
		}
	    }
        }
    } else {
        for (i = 0; i < numNames; i++)
            if (!refName || !name_eq(dataNames[i], refName)) {
                addDataDesc(run, dataNames[i], dataType, i);
            }
    }

    /* Pass 2. */
    for (i = 0; i < numsaves; i++) {
        if (savesused[i])
            continue;
        if (!parseSpecial(saves[i].name, namebuf, parambuf, depbuf)) {
	    if (saves[i].analysis)
		fprintf(cp_err, "Warning: can't parse '%s': ignored\n",
			saves[i].name);
            continue;
        }
        /* Now, if there's a dep variable, do we already have it? */
        if (*depbuf) {
            for (j = 0; j < run->numData; j++)
                if (name_eq(depbuf, run->data[j].name))
                    break;
            if (j == run->numData) {
                /* Better add it. */
                for (j = 0; j < numNames; j++)
                    if (name_eq(depbuf, dataNames[j]))
                        break;
                if (j == numNames) {
                    fprintf(cp_err,
            "Warning: can't find '%s': value '%s' ignored\n",
                        depbuf, saves[i].name);
                    continue;
                }
                addDataDesc(run, dataNames[j], dataType, j);
                savesused[i] = true;
		saves[i].used = 1;
                depind = j;
            } else
                depind = run->data[j].outIndex;
        }
        addSpecialDesc(run, saves[i].name, namebuf, parambuf, depind);
    }

    if (numsaves) {
	for (i = 0; i < numsaves; i++) {
	    tfree(saves[i].analysis);
	    tfree(saves[i].name);
	}
        tfree(savesused);
    }

    if (numNames && (run->numData == 1 && run->refIndex != -1
	    || run->numData == 0 && run->refIndex == -1))
    {
	fprintf(cp_err, "Error: no data saved for %s; analysis not run\n",
	    analInfo[((JOB *) analysisPtr)->JOBtype]->public.description);
	return E_NOTFOUND;
    }
    
    /* Now that we have our own data structures built up, let's see what
     * nutmeg wants us to do.
     */
    run->writeOut = ft_getOutReq(&run->fp, &run->runPlot, &run->binary,
            run->type, run->name);

    if (run->writeOut)
        fileInit(run);
    else {
        plotInit(run);
	if (refName)
	    run->runPlot->pl_ndims = 1;
    }

    return (OK);
}

static int
addDataDesc(run, name, type, ind)
    runDesc *run;
    char *name;
    int type;
    int ind;
{
    dataDesc *data;

    if (!run->numData)
        run->data = (dataDesc *) tmalloc(sizeof (dataDesc));
    else
        run->data = (dataDesc *) trealloc((char *) run->data,
                sizeof (dataDesc) * (run->numData + 1));
    data = &run->data[run->numData];
    /* so freeRun will get nice NULL pointers for the fields we don't set */
    bzero(data, sizeof(dataDesc));

    data->name = copy(name);
    data->type = type;
    data->gtype = GRID_LIN;
    data->regular = true;
    data->outIndex = ind;

    if (ind == -1) {
        /* It's the reference vector. */
        run->refIndex = run->numData;
    }

    run->numData++;

    return (OK);
}

static int
addSpecialDesc(run, name, devname, param, depind)
    runDesc *run;
    char *name;
    char *devname;
    char *param;
    int depind;
{
    dataDesc *data;
    char *unique;       /* unique char * from back-end */

    if (!run->numData)
        run->data = (dataDesc *) tmalloc(sizeof (dataDesc));
    else
        run->data = (dataDesc *) trealloc((char *) run->data,
                sizeof (dataDesc) * (run->numData + 1));
    data = &run->data[run->numData];
    /* so freeRun will get nice NULL pointers for the fields we don't set */
    bzero(data, sizeof(dataDesc));

    data->name = copy(name);

    unique = devname;
    INPinsert(&unique, (INPtables *) ft_curckt->ci_symtab);
    data->specName = unique;

    data->specParamName = copy(param);

    data->specIndex = depind;
    data->specType = -1;
    data->specFast = NULL;
    data->regular = false;

    run->numData++;

    return (OK);
}



/*
	This function actually delivers the data described in OUTpBeginPlot for a single value of the
	reference variable. For multi-dimensional plots (more than one independent variable), refValue will be the value for the
	independent variable which is changing MOST rapidly, values for the other independent variables are
	obtained from previous calls to OUTbegrnDomain. If refUid was given as the reserved IFuid 0, then
	the refvalue is uninteresting and should be ignored. The length of the vector valuePtr MUST be the same as the value of the numNames parameter passed to
	OUTpBegmPlot.
*/
///<param name = "*plotPtr"> The pointer returned by OUTpBegmPlot above and is used to ensure that plot data doesn�t get mixed up between plots since multiple plots may be active at once. </param>
///<param name = "*refValue"> The value (of the type indicated by reffype in the OUTpBegmPlot call) of the reference vector </param>
///<param name = "*valuePtr"> A pointer to an IFvalue structure of type IF_VECTOR of whatever type was given for the dataType in the call to OUTpBegmPlot </param>
int OUTpData(GENERIC *plotPtr, IFvalue *refValue, IFvalue *valuePtr)
{
    runDesc *run = (runDesc *) plotPtr;
    IFvalue val;
    int i;

    run->pointCount++;

    if (run->writeOut) 
	{
	if (run->pointCount == 1)
	    fileInit_pass2(plotPtr);
        fileStartPoint(run->fp, run->binary, run->pointCount);

        if (run->refIndex != -1) {
          if (run->isComplex)
            fileAddComplexValue(run->fp, run->binary, refValue->cValue);
          else
            fileAddRealValue(run->fp, run->binary, refValue->rValue);
        }

        for (i = 0; i < run->numData; i++) {
            /* we've already printed reference vec first */
            if (run->data[i].outIndex == -1) continue;

            if (run->data[i].regular) {
                if(run->data[i].type == IF_REAL)
                  fileAddRealValue(run->fp, run->binary,
                    valuePtr->v.vec.rVec
                    [run->data[i].outIndex]);
                else if (run->data[i].type == IF_COMPLEX)
                  fileAddComplexValue(run->fp, run->binary,
                    valuePtr->v.vec.cVec
                    [run->data[i].outIndex]);
                else
                  fprintf(stderr, "OUTpData: unsupported data type\n");
            } else {
                /* should pre-check instance */
                if (!getSpecial(&run->data[i], run, &val))
                    continue;
                if (run->data[i].type == IF_REAL)
                  fileAddRealValue(run->fp, run->binary,
                     val.rValue);
                else if (run->data[i].type == IF_COMPLEX)
                  fileAddComplexValue(run->fp, run->binary,
                     val.cValue);
                else
                  fprintf(stderr, "OUTpData: unsupported data type\n");
            }
        }
        fileEndPoint(run->fp, run->binary);
    } else {
        for (i = 0; i < run->numData; i++) {
            if (run->data[i].outIndex == -1) {
                if (run->data[i].type == IF_REAL)
                    plotAddRealValue(&run->data[i],
                            refValue->rValue);
                else if (run->data[i].type == IF_COMPLEX)
                    plotAddComplexValue(&run->data[i],
                            refValue->cValue);
            } else if (run->data[i].regular) {
                if (run->data[i].type == IF_REAL)
                    plotAddRealValue(&run->data[i],
                        valuePtr->v.vec.rVec
                        [run->data[i].outIndex]);
                else if (run->data[i].type == IF_COMPLEX)
                    plotAddComplexValue(&run->data[i],
                        valuePtr->v.vec.cVec
                        [run->data[i].outIndex]);
            } else {
                /* should pre-check instance */
                if (!getSpecial(&run->data[i], run, &val))
                    continue;
                if (run->data[i].type == IF_REAL)
                    plotAddRealValue(&run->data[i],
                            val.rValue);
                else if (run->data[i].type == IF_COMPLEX)
                    plotAddComplexValue(&run->data[i],
                            val.cValue);
                else 
                    fprintf(stderr, "OUTpData: unsupported data type\n");
            }
        }
        gr_iplot(run->runPlot);
    }

    if (ft_bpcheck(run->runPlot, run->pointCount) == false)
        shouldstop = true;

    return (OK);
}



/*
	This routine outputs a window�s worth of the reference vector. Several calls will usually be
	made to this routine with overlapping values when different nodes have different time scales. 
*/
///<param name = "*plotPtr"> Specifies the analysis this applies to </param>
///<param name = "*valuePtr"> Provides a vector of the type given in OUTwBeginPlot of data convering one window of values for the reference variable </param>
///<param name = "**refPtr"> Used to return to the simulator a pointer it will use to refer to this particular reference vector during the remainder of this window </param>
int OUTwReference(GENERIC *plotPtr, IFvalue *valuePtr, GENERIC **refPtr)
{
	return (OK);
}



/*
	Supplies output data for a single signal for an entire window. 
	The number of elements in the vector in valuePtr 
	and the number of elements in the vector provided to OUTwReference when the given refPtr was
	returned must match.
*/
///<param name = "*plotPtr"> Gives the analysis this data is for, the indes of the output vector being provided within the dataNames array provided in the call to OUTwBeginPlot </param>
///<param name = "dataindex"> This does something </param>
///<param name = "*valuePtr"> A vector of whatever type was specified in OUTwBeginPlot providing the values </param>
///<param name = "*refPtr"> This does something </param>
int OUTwData(GENERIC *plotPtr, int dataIndex, IFvalue *valuePtr, GENERIC *refPtr)
{
    return (OK);
}



/*
	This call indicates that all data for the current window is complete and the output system should
	prepare to receive data for the next window. This function may optionally be called at the end of the
	plot or a domain, but is not needed at those points since OUTendDomain and OUTendPlot both imply
	the end of a window.
*/
///<param name = "*plotPtr"> This does something </param>
int OUTwEnd(GENERIC *plotPtr)
{
    return (OK);
}



/*
	This call indicates to the output system that all the data for the specified plot has been delivered
	already and any clean-up operations may be performed.
*/
///<param name = "*plotPtr"> This does something </param>
int OUTendPlot(GENERIC *plotPtr)
{
	runDesc *run = (runDesc *)plotPtr;

	if (run->writeOut)
		fileEnd(run);
	else
	{
		gr_end_iplot();
		plotEnd(run);
	}

	freeRun(run);

	return (OK);
}



/*
	When a plot has two or more independent variables, it is assumed that they will be swept in the
	fashion of nested loops, with one swept through its entire range before the next one is changed. This
	function specifies to the output package that this sort of sweeping is taking place. Each call to OUTbe-
	grnDomain indicates a variable that will be swept and must match up with a corresponding call to
	OUTendDomain. The most recent OUTbepDomain not yet closed by the corresponding OUTend-
	Domain specifies the only reference variable which may change, with the values of all less rapidly
	changing reference variables specified by the intervening OUTbeginDomain calls. An arbitrary nesting
	level is permitted. There is an implied OUTbegmDomWOUTendDomain pair in the OUTwBepPlot
	or OUTpBegmPlot and OUTendPlot function calls, so an explicit call is not required for the outermost
	reference variable or the only reference variable in simple plots. Note that this structure requires that
	all OUTwBeginPlot or OUTpBegmPlot calls within one plot occur at exactly the same nesting level
	and with exactly the same set of reference variables active at each call.
*/
///<param name = "*plotPtr""> This does something </param>
///<param name = "*refName"> This does something </param>
///<param name = "refType"> This does something </param>
///<param name = "*outerRefValue"> This does something </param>
int OUTbeginDomain(GENERIC *plotPtr, char *refName, int refType, IFvalue *outerRefValue)
{
    return (OK);
}



/*
	Closes the innermost pending OUTbeginDomain call, thus allowing the reference variable at the
	next higher nesting level to change.
*/
///<param name = "*plotPtr"> This does something </param>
int OUTendDomain(GENERIC *plotPtr)
{
    return (OK);
}



/*
	This function allows the simulator to provide additional infomation to the output system about
	the plot in general or about specific variables in the plot. If varUid is NULL, then the data is a default
	for this plot which ovemdes the defaults described here only for this plot. If varUid is not NULL,
	then it is the unique identifier of one of the variables in the plot specified by plotPtr. and provides
	ovemding values for just that variable. The possible values for the parameters and their meanings
	are:

	parameter			Value type				value meaning										default
	---------			----------				-------------										-------
	OUT_SUBTITLE		char *					An optional one line descriptive title				none
												Only applicable to the entire plot

	OUT_INTERP_POLY		int						The number of points used to interpolate			2
												values using polynomial interpolation.
												A value of zero implies interpolation
												is not appropriate. For values of 1,
												2, 3, 4 specify respectively piecewise
												constant, linear, quadratic, or cubic
												interpolation.

	OUT_INTERP_SINE		int						The number of points used to interpolate			?
												values using sinusoidal interpolation.
												The sinusoid used should be contiguous
												harmonics of a fundamental, whose
												frequency equals the reciprocal of the 
												difference between the maximum and 
												minimum values of the independent
												variable.

	OUT_INTERP_EXP		int						The number of points used to interpolate			?
												values using exponential interpolation

	OUT_UNITS			char *					The name of the units the data is in				none

	OUT_SCALE_LIN		none					The ouput should be plotted on a linear scale		?

	OUT_SCALE_LOG		none					The ouput should be plotted on a log scale			?

	OUT_GRID_RECT		none					The output should be plotted on a					*
												rectangular grid
	
	OUT_GRID_SMITH		none					The output should be plotted on a					?
												smith chart

	OUT_GRID_POLAR		none					The output should be plotted on a					?
												polar grid	

	OUT_STYLE_POINTS	none					The plot should show only points					?
												without a connecting curve

	OUT_STYLE_CURVES	none					The plot should have a smooth curve					* 
												fitted through the points based on
												the interpolation parameter above.

	OUT_STYLE_COMB		none					The plot should be a comb or						?
												bar-graph type plot

	OUT_RELTOL			double					The relative precision of a variable.				0
												Values of a variable smaller than this
												threshold times the maximum value of the
												variable are considered noise

	OUT_ABSTOL			double					The absolute precision of a variable.				0
												Values of a variable smaller than this 
												threshold are considered noise

	The interpolation attributes are usually specified for the independent variables only. When
	given for a particular independent variable, they are applied when interpolating any dependent variable
	over intervals of that independent variable.

	Calls to this function must follow the call to OUTwBeginPlot or OUTpBegmPlot immediately,
	and precede all calls to outBeginDomain or any data output functions. Note that this function may
	thus provide attributes for a reference variable which has not yet been encountered in an OUTbeginDomain 
	call.

	For consistency, it is recommended that the value of OUT - UNl"S be in a standard form. For
	simple Units, the standard symbol should be used. Below is a partial list of standard symbols, with
	some alterations to avoid unusual characters such as Greek letters.

	Abbreviation		Meaning
	------------		-------
		V				Volt
		A				Ampere
		W				Watt
		J				Joule
		ohm				ohm (resistance)
		S				Siemens
		F				Farad
		H				Henry
		s				second
		Hz				Hertz (frequency)
		K				degree Kelvin
		C				degree Celsius
		Coul			Coulomb (charge)
		Wb				Weber (magnetic field strength?)
		T				Tesla
		m				meter
		in				in

	For more complicated units, the standard symbols should be combined according to a fixed set of
	rules whenever possible: a product of units is formed by placing a hyphen between them; a ratio of
	units is formed by placing a slash between them, and only one slash is allowed in the entire units
	string; a power of a unit is represented by a caret and a number following the unit: as special cases of
	powers, the square root of a unit is written as sqrt(unit) and the cube root of a unit is written as
	cbrt(units); parentheses can be used for grouping; no spaces should appear in the units string.
	Exponentiatian has the highest precedence, followed by multiplication, the division. Some examples
	of units strings formed according to these rules are:

	V/sqrt(Hz)
	ohm^2-v/(m - K)^3

	A clever plotting package can take advantage of this standardization to do some formatting, such as
	raising the exponents or displaying a ratio with the numerator over the denominator.
*/
///<param name = "*plotPtr"> This does something </param>
///<param name = "*varName"> This does something </param>
///<param name = "param"> This does something </param>
///<param name = "*value"> This does something </param>
int OUTattributes(GENERIC *plotPtr, char *varName, int param, IFvalue *value)
{
	runDesc *run = (runDesc *)plotPtr;
	struct dvec *d;
	struct dataDesc *dd;
	int type;
	int i;

	if (param == OUT_SCALE_LIN)
		type = GRID_LIN;
	else if (param == OUT_SCALE_LOG)
		type = GRID_XLOG;
	else
		return E_UNSUPP;

	if (run->writeOut)
	{
		if (varName)
		{
			for (i = 0; i < run->numData; i++)
			{
				if (!strcmp(varName, run->data[i].name))
					run->data[i].gtype = type;
			}
		}
		else
		{
			run->data[run->refIndex].gtype = type;
		}
	}
	else
	{
		if (varName)
		{
			for (d = run->runPlot->pl_dvecs; d; d = d->v_next)
			{
				if (!strcmp(varName, d->v_name))
					d->v_gridtype = type;
			}
		}
		else
		{
			run->runPlot->pl_scale->v_gridtype = type;
		}
	}

	return (OK);
}



/* The file writing routines. */

static void
fileInit(run)
    runDesc *run;
{
    char buf[513];
    int i;

    /* This is a hack. */
    run->isComplex = false;
    for (i = 0; i < run->numData; i++)
        if (run->data[i].type == IF_COMPLEX)
            run->isComplex = true;

    i = 0;
    sprintf(buf, "Title: %s\n", run->name);
    i += strlen(buf);
    fputs(buf, run->fp);
    sprintf(buf, "Date: %s\n", datestring());
    i += strlen(buf);
    fputs(buf, run->fp);
    sprintf(buf, "Plotname: %s\n", run->type);
    i += strlen(buf);
    fputs(buf, run->fp);
    sprintf(buf, "Flags: %s\n", run->isComplex ? "complex" : "real");
    i += strlen(buf);
    fputs(buf, run->fp);
    sprintf(buf, "No. Variables: %d\n", run->numData);
    i += strlen(buf);
    fputs(buf, run->fp);
    sprintf(buf, "No. Points: ");
    i += strlen(buf);
    fputs(buf, run->fp);

    fflush(run->fp);        /* Gotta do this for LATTICE. */
    if (run->fp == stdout || (run->pointPos = ftell(run->fp)) <= 0)
	run->pointPos = i;
    fprintf(run->fp, "0       \n"); /* Save 8 spaces here. */

    fprintf(run->fp, "Command: version %s\n", ft_sim->version);
    fprintf(run->fp, "Variables:\n");

    return;
}

static void
fileInit_pass2(run)
    runDesc *run;
{
    int i, type;
    char *name, buf[BSIZE_SP];

    for (i = 0; i < run->numData; i++) {
        if (isdigit(*run->data[i].name)) {
        (void) sprintf(buf, "V(%s)", run->data[i].name);
          name = buf;
        } else {
          name = run->data[i].name;
        }
        if (substring("#branch", name))
            type = SV_CURRENT;
        else if (cieq(name, "time"))
            type = SV_TIME;
        else if (cieq(name, "frequency"))
            type = SV_FREQUENCY;
        else 
            type = SV_VOLTAGE;

        fprintf(run->fp, "\t%d\t%s\t%s", i, name,
                ft_typenames(type));
	if (run->data[i].gtype == GRID_XLOG)
	    fprintf(run->fp, "\tgrid=3");
	fprintf(run->fp, "\n");

    }

    fprintf(run->fp, "%s:\n", run->binary ? "Binary" : "Values");

    return;
}

static void
fileStartPoint(fp, bin, num)
    FILE *fp;
    bool bin;
    int num;
{
    if (!bin)
        fprintf(fp, "%d\t", num - 1);

    return;
}

static void
fileAddRealValue(fp, bin, value)
    FILE *fp;
    bool bin;
    double value;
{
    if (bin)
        fwrite((char *) &value, sizeof (double), 1, fp);
    else
        fprintf(fp, "\t%.*e\n", DOUBLE_PRECISION, value);
    
    return;
}

static void
fileAddComplexValue(fp, bin, value)
    FILE *fp;
    bool bin;
    IFcomplex value;
{

    if (bin) {
        fwrite((char *) &value.real, sizeof (double), 1, fp);
        fwrite((char *) &value.imag, sizeof (double), 1, fp);
    } else {
        fprintf(fp, "\t%.*e,%.*e\n", DOUBLE_PRECISION, value.real,
                DOUBLE_PRECISION, value.imag);
    }

}

/* ARGSUSED */ /* until some code gets written */
static void
fileEndPoint(fp, bin)
    FILE *fp;
    bool bin;
{
    return;
}

/* Here's the hack...  Run back and fill in the number of points. */

static void
fileEnd(run)
    runDesc *run;
{
    long place;

    if (run->fp != stdout) {
	place = ftell(run->fp);
	fseek(run->fp, run->pointPos, 0);
	fprintf(run->fp, "%d", run->pointCount);
	fseek(run->fp, place, 0);
    } else {
	/* Yet another hack-around */
	fprintf(stderr, "@@@ %ld %d\n", run->pointPos, run->pointCount);
    }
    fflush(run->fp);

    return;
}



/* The plot maintenance routines. */

static void
plotInit(run)
    runDesc *run;
{
    struct plot *pl = plot_alloc(run->type);
    char buf[100];
    struct dvec *v;
    dataDesc *dd;
    int i;

    pl->pl_title = copy(run->name);
    pl->pl_name = copy(run->type);
    pl->pl_date = copy(datestring( ));
    pl->pl_ndims = 0;
    plot_new(pl);
    plot_setcur(pl->pl_typename);
    run->runPlot = pl;

    /* This is a hack. */
    /* if any of them complex, make them all complex */
    run->isComplex = false;
    for (i = 0; i < run->numData; i++) {
      if (run->data[i].type == IF_COMPLEX) run->isComplex = true;
    }

    for (i = 0; i < run->numData; i++) {
        dd = &run->data[i];
        v = alloc(struct dvec);
        if (isdigit(*dd->name)) {
            (void) sprintf(buf, "V(%s)", dd->name);
            v->v_name = copy(buf);
        } else
            v->v_name = copy(dd->name);
        if (substring("#branch", v->v_name))
            v->v_type = SV_CURRENT;
        else if (cieq(v->v_name, "time"))
            v->v_type = SV_TIME;
        else if (cieq(v->v_name, "frequency"))
            v->v_type = SV_FREQUENCY;
        else 
            v->v_type = SV_VOLTAGE;
        v->v_length = 0;
        v->v_scale = NULL;
        if (!run->isComplex) {
            v->v_flags = VF_REAL;
            v->v_realdata = NULL;
        } else {
            v->v_flags = VF_COMPLEX;
            v->v_compdata = NULL;
        }

        v->v_flags |= VF_PERMANENT;

        vec_new(v);
        dd->vec = v;
    }
}

static void
plotAddRealValue(desc, value)
    dataDesc *desc;
    double value;
{
    struct dvec *v = desc->vec;

    if (isreal(v)) {
      v->v_realdata = (double *) trealloc((char *) v->v_realdata,
            sizeof (double) * (v->v_length + 1));
      v->v_realdata[v->v_length] = value;
    } else {
      /* a real parading as a VF_COMPLEX */
      v->v_compdata = (complex *) trealloc((char *) v->v_compdata,
            sizeof (complex) * (v->v_length + 1));
      v->v_compdata[v->v_length].cx_real = value;
      v->v_compdata[v->v_length].cx_imag = (double) 0;
    }
    v->v_length++;

    return;
}

static void
plotAddComplexValue(desc, value)
    dataDesc *desc;
    IFcomplex value;
{
    struct dvec *v = desc->vec;

    v->v_compdata = (complex *) trealloc((char *) v->v_compdata,
            sizeof (complex) * (v->v_length + 1));
    v->v_compdata[v->v_length].cx_real = value.real;
    v->v_compdata[v->v_length].cx_imag = value.imag;
    v->v_length++;

    return;
}

/* ARGSUSED */ /* until some code gets written */
static void
plotEnd(run)
    runDesc *run;
{

    return;
}



/* ParseSpecial takes something of the form "@name[param,index]" and rips
 * out name, param, and index.
 */

static bool
parseSpecial(name, dev, param, ind)
    char *name;
    char *dev;
    char *param;
    char *ind;
{
    char *s;

    *dev = *param = *ind = '\0';

    if (*name != '@')
        return (false);
    name++;
    
    s = dev;
    while (*name && (*name != '['))
        *s++ = *name++;
    *s = '\0';
    if (!*name)
        return (true);
    name++;

    s = param;
    while (*name && (*name != ',') && (*name != ']'))
        *s++ = *name++;
    *s = '\0';
    if (*name == ']')
        return (!name[1] ? true : false);
    else if (!*name)
        return (false);
    name++;

    s = ind;
    while (*name && (*name != ']'))
        *s++ = *name++;
    *s = '\0';
    if (*name && !name[1])
        return (true);
    else
        return (false);
}

/* This routine must match two names with or without a V() around them. */

static bool
name_eq(n1, n2)
    char *n1, *n2;
{
    char buf1[BSIZE_SP], buf2[BSIZE_SP], *s;

    if (s = index(n1, '(')) {
        strcpy(buf1, s);
        if (!(s = index(buf1, ')')))
            return (false);
        *s = '\0';
        n1 = buf1;
    }
    if (s = index(n2, '(')) {
        strcpy(buf2, s);
        if (!(s = index(buf2, ')')))
            return (false);
        *s = '\0';
        n2 = buf2;
    }

    return (strcmp(n1, n2) ? false : true);
}

static bool
getSpecial(desc, run, val)
    dataDesc *desc;
    runDesc *run;
    IFvalue *val;
{
    IFvalue selector;
    struct variable *vv;

    selector.iValue = desc->specIndex;
    if (INPaName(desc->specParamName, val, run->circuit, &desc->specType,
            desc->specName, &desc->specFast, ft_sim, &desc->type,
            &selector) == OK) {
      desc->type &= (IF_REAL | IF_COMPLEX);   /* mask out other bits */
      return(true);
    } else if (vv = if_getstat(run->circuit, &desc->name[1])) {
						/* skip @ sign */
      desc->type = IF_REAL;
      if (vv->va_type == VT_REAL)
	val->rValue = vv->va_real;
      else if (vv->va_type == VT_NUM)
	val->rValue = vv->va_num;
      else if (vv->va_type == VT_BOOL)
	val->rValue = (vv->va_bool ? 1.0 : 0.0);
      else {
	return (false); /* not a real */
      }
      tfree(vv);
      return(true);
    }

    return (false);
}

static void
freeRun(run)
    runDesc *run;
{

    int i;

    for (i=0; i < run->numData; i++) {
/*    vec_free(run->data[i].vec); */ /* kill run, leave plot */
      tfree(run->data[i].name);
      tfree(run->data[i].specParamName);
    }
    tfree(run->data);

/*  killplot(run->runPlot); */ /* kill run, leave plot */

    free(run->type);
    free(run->name);
    free(run);

}

int
OUTstopnow()
{

    if (ft_intrpt || shouldstop) {
        ft_intrpt = shouldstop = false;
        return (1);
    } else
        return (0);

}

/* Print out error messages. */

static struct mesg {
        char *string;
        long flag;
} msgs[] = {
        { "Warning", ERR_WARNING } ,
        { "Fatal error", ERR_FATAL } ,
        { "Panic", ERR_PANIC } ,
        { "Note", ERR_INFO } ,
        { NULL, 0 }
} ;

void
OUTerror(flags,format,names) 
    int flags;
    char *format;
    IFuid *names;
{

    struct mesg *m;
    char buf[BSIZE_SP], *s, *bptr;
    int nindex = 0;

    if ((flags == ERR_INFO) && cp_getvar("printinfo", VT_BOOL,
	    (char *) &printinfo))
	return;

    for (m = msgs; m->flag; m++)
                if (flags & m->flag)
                        fprintf(cp_err, "%s: ", m->string);

    for (s = format, bptr = buf; *s; s++) {
      if (*s == '%' && (s == format || *(s-1) != '%') && *(s+1) == 's') {
	if (names[nindex])
	  strcpy(bptr, names[nindex]);
	else
	  strcpy(bptr, "(null)");
        bptr += strlen(bptr);
        s++;
        nindex++;
      } else {
        *bptr++ = *s;
      }
    }
    *bptr = '\0';
        fprintf(cp_err, "%s\n", buf);
        fflush(cp_err);

}