/*******************************************************************************
Open Source
MIT License
********************************************************************************
*
* NAME: NLP Engine
* FILE: engine.cpp
* CR:   09/14/20
* SUBJ: Creates a simple function to the analyzer
*
*******************************************************************************/

#include "nlp_engine.h"

#ifdef LINUX
#include <unistd.h>
#endif

////////// NLP_INIT /////////////
// CR:  09/23/20 AM.
// DESC:    Initialize the NLP framework.
// NOTE:    Uses the global variables VTRun_ptr and VTRun_count.
//          Create RFA and RFB analyzers, ie, generate the NLP++ parser.
//          (data/Rfb/spec folder holds definition of NLP++ parser.)
//          Create the list manager.  And whatever else the VTRUN
//          framework manages.
/////////////////////////////////
/////////////////////////////////

////////// NLP_FIN /////////////
// CR:  09/23/20 AM.
// DESC:    Shut down the NLP framework.
// NOTE:    Uses the global variables VTRun_ptr and VTRun_count.
/////////////////////////////////
/////////////////////////////////



NLP_ENGINE::NLP_ENGINE(
    _TCHAR *workingFolder,
	bool silent
	)
{
    static _TCHAR logfile[MAXSTR];
    static _TCHAR rfbdir[MAXSTR];
    if (workingFolder) {
        _stprintf(logfile,"%s%s",workingFolder,_T("vtrun_logfile.out"));
        _stprintf(rfbdir,"%sdata/rfb/spec",workingFolder);
    } else {
        _stprintf(logfile,"%s",_T("vtrun_logfile.out"));
        _stprintf(rfbdir,"data/rfb/spec");
    }
    _t_cout << _T("[logfile: ") << logfile << _T("]") << endl;
 
    _t_cout << _T("[rfbdir: ") << rfbdir << _T("]") << endl;
 
 if (!VTRun_Ptr)
    {
        m_vtrun = VTRun::makeVTRun(                               // 07/21/03 AM.
            logfile,                // Verbose/error log file.         // 08/28/02 AM.
            rfbdir,                 // VisualText dir for RFB spec.    // 08/28/02 AM.
            silent                      // Build silently.                 // 08/28/02 AM.
            );
    }
}

NLP_ENGINE::~NLP_ENGINE()
{
    close();    // 09/27/20 AM.
}

void NLP_ENGINE::zeroInit()
{ 
    m_input = 0;
    m_output = 0;
    m_sequence = 0;

    m_anadir[0] = '\0';
    m_ananame[0] = '\0';
//  m_rfbdir[0] = '\0';
    m_rfbdir[0] = '\0';
    m_logfile[0] = '\0';	
    m_specdir[0] = '\0';
    m_infile[0] = '\0';
    m_outdir[0] = '\0';
    m_outfile[0] = '\0';
    m_seqfile[0] = '\0';

    m_vtrun = 0;
    m_nlp = 0;
    m_cg = 0;

    #ifdef TEST_RUG_
    m_gram = 0;
    m_rug = 0;
    #endif
}
 
int NLP_ENGINE::init(
    _TCHAR *analyzer,
    _TCHAR *workingFolder,
	bool develop,
	bool silent,
    bool compiled
)
{   
     NLP_ENGINE::zeroInit();
 
    // Convenience ptr.
    m_vtrun = VTRun_Ptr;    // 09/27/20 AM.

    m_analyzer = analyzer;
    m_develop = develop;
    m_silent = silent;
    m_compiled = compiled;

    struct stat st;
    char cwd[MAXSTR] = _T("");

    if (m_workingFolder[0] != '\0') {
        strcpy(cwd, m_workingFolder);
        _stprintf(m_anadir, _T("%s%sanalyzers%s%s"),cwd,DIR_STR,DIR_STR,analyzer);
        _stprintf(m_ananame, _T("%s"),analyzer);
    } else if (stat(analyzer,&st) != 0) {
	#ifdef LINUX
        if (getcwd(cwd, sizeof(cwd)) != NULL) {
            _t_cout << _T("[current directory: ") << cwd << _T("]") << endl;
            _stprintf(m_anadir, _T("%s%sanalyzers%s%s"),cwd,DIR_STR,DIR_STR,analyzer);
            _stprintf(m_ananame, _T("%s"),analyzer);
        } else {
            _t_cerr << _T("[couldn't get current directory") << analyzer << _T("]") << endl;
            return 0;
        }
	#endif
    } else {
        _stprintf(m_anadir, _T("%s"),analyzer);	
        _TCHAR *ana = _tcsrchr(m_anadir,_T('/'));
        ++ana;
        _stprintf(m_ananame, _T("%s"), ana); 
    }

	_t_cout << _T("[analyzer directory: ") << m_anadir << _T("]") << endl;
    _t_cout << _T("[analyzer name: ") << m_ananame << _T("]") << endl; 

    _stprintf(m_rfbdir, _T("%s/data%crfb%cspec"),cwd, DIR_CH,DIR_CH);  
    _t_cout << _T("[rfb file: ") << m_rfbdir << _T("]") << endl;

    _TCHAR *tmp = _T("./tmp");
    _stprintf(m_logfile, _T("%s%cvisualtext.log"),tmp,DIR_CH);
    NLP_ENGINE::createDir(tmp);
    _t_cout << _T("[log file: ") << m_logfile << _T("]") << endl;

    _stprintf(m_specdir, _T("%s%sspec"), m_anadir, DIR_STR);
    _t_cout << _T("[spec directory: ") << m_specdir << _T("]") << endl;

    _stprintf(m_seqfile, _T("%s%sanalyzer.seq"),m_specdir,DIR_STR);
    _t_cout << _T("[spec file: ") << m_seqfile << _T("]") << endl;
 
    _stprintf(m_outdir, _T("%s%s%s"), m_anadir,DIR_STR,_T("output"));
    NLP_ENGINE::createDir(m_outdir);
    _t_cout << _T("[output directory: ") << m_outdir << _T("]") << endl;

    /////////////////////////////////////////////////
    // INITIALIZE VISUALTEXT RUNTIME SYSTEM                        // 08/27/02 AM.
    /////////////////////////////////////////////////

    /////////////////////////////////////////////////
    // INITIALIZE ANALYZER RUNTIME ENGINE
    /////////////////////////////////////////////////
    if (m_nlp = VTRun_Ptr->findAna(analyzer))
        {
        _t_cout << _T("Analyzer found: ") << analyzer << analyzer << endl;
        _t_cout << _T("[TODO: RELOAD ANALYZER (NLP) INTO NLPENGINE HERE.]") << analyzer << endl;
        // return 1;    // If reloading same analyzer, done init....
        m_cg = m_nlp->getCG();

 
        //    _t_cout << _T("Analyzer not found: ") << analyzer << endl;
        }
    else
        {
        // Create and initialize an NLP object to manage text analysis.
        // NOTE: This init will dynamically load the user extensions dll at
        // appdir\user\debug\user.dll
        m_nlp = m_vtrun->makeNLP(m_anadir,m_analyzer,m_develop,m_silent,m_compiled);  // 07/21/03 AM.

        VTRun_Ptr->addAna(m_nlp);   // Add ana to runtime manager.  // 09/27/20 AM.


        /////////////////////////////////////////////////
        // SET UP THE KNOWLEDGE BASE
        /////////////////////////////////////////////////

         m_cg = m_vtrun->makeCG(                                        // 07/21/03 AM.
                m_anadir,
                true,      // LOAD COMPILED KB IF POSSIBLE.
                m_nlp);      // Associated analyzer object.              // 07/21/03 AM.


        if (!m_cg)                                                       // 07/21/03 AM.
        {
            _t_cerr << _T("[Couldn't make knowledge base.]") << endl;  // 07/21/03 AM.
            m_vtrun->rmAna(m_nlp);  // 09/27/20 AM.
            m_vtrun->deleteNLP(m_nlp);                                     // 07/21/03 AM.
    //        VTRun::deleteVTRun(m_vtrun);                                 // 07/21/03 AM.
            return -1;
        }

        _t_cerr << _T("[Loaded knowledge base.]") << endl;             // 02/19/19 AM.

        /////////////////////////////////////////////////
        // BUILD ANALYZER APPLICATION
        /////////////////////////////////////////////////
        // Create an analyzer dynamically using the sequence file and rules files
        // under appdir\spec.

        if (!m_nlp->make_analyzer(m_seqfile, m_anadir, m_develop,
            silent,              // Debug/log file output.             // 06/16/02 AM.
            0,
            false,               // false == Don't compile during load.
            compiled))           // Compiled/interp analyzer.
            {
            _t_cerr << _T("[Couldn't build analyzer.]") << endl;
            m_vtrun->rmAna(m_nlp);  // 09/27/20 AM.
            m_vtrun->deleteNLP(m_nlp);                                     // 07/21/03 AM.
    //        VTRun::deleteVTRun(m_vtrun);                                 // 07/21/03 AM.
            return -1;
            }
        }   // end ELSE...

    /////////////////////////////
    // TEST RULE GENERATION.
    /////////////////////////////
    #ifdef TEST_RUG_
    CONCEPT *root = VTRun::getKBroot(cg);
    m_gram = cg->findConcept(root, _T("gram"));
    if (!m_gram)
    gram = m_cg->makeConcept(root, _T("gram"));
    RUG::rugInit(gram,m_cg);

    // Execute rule generation.
    m_rug = new RUG(gram, m_nlp, m_anapath, m_cg);
    m_rug->rugGenerate(false,false);      // Generate ALL.
    delete rug;
    rug = 0;
    #endif

    return 0;
}
 
int NLP_ENGINE::analyze(
    _TCHAR *analyzer,
    _TCHAR *infile,
    _TCHAR *outfile,
    _TCHAR *workingFolder,
	bool develop,
	bool silent,
    bool compiled
	)
{   
 
    NLP_ENGINE::init(analyzer,workingFolder,develop,silent,compiled);

    struct stat st;
    if (stat(infile,&st) == 0)
        _stprintf(m_infile, _T("%s"),infile);
    else
        _stprintf(m_infile, _T("%s%sinput%s%s"),m_anadir,DIR_STR,DIR_STR,infile);
    _t_cout << _T("[infile path: ") << m_infile << _T("]") << endl;
    _stprintf(m_outfile, _T("%s%soutput.txt"),m_anadir,DIR_STR);
    _t_cout << _T("[outfile path: ") << m_outfile << _T("]") << endl;

    // Analyzer can output to a stream.
    _TCHAR ofstr[MAXSTR];
    #ifdef LINUX
    _stprintf(ofstr,_T("./dummy.txt"));
    #else
    _stprintf(ofstr,_T("e:\\dummy.txt"));
    #endif
    _t_ofstream os(TCHAR2CA(ofstr), ios::out);						// 08/07/02 AM.

    // Testing output to buffer.
    _TCHAR obuf[MAXSTR];

    m_nlp->analyze(m_infile, m_outfile, m_anadir, m_develop,
        m_silent,        // Debug/log output files.                  // 06/16/02 AM.
        0,             // Outdir.
        0,           // Input buffer.
        0,        // Length of input buffer, or 0.
        m_compiled,      // If running compiled analyzer.
        &os,	   // Rebind cout output stream in analyzer    // 08/07/02 AM.
        obuf,       // 05/11/02 AM.
        MAXSTR	   // 05/11/02 AM.
        );

//    NLP_ENGINE::close();  // NO.  // 09/27/20 AM.
    return 0;
}

int NLP_ENGINE::analyze(
    _TCHAR *analyzer,
    _TCHAR *inbuf,
    long len,
    _TCHAR *outbuf,
    long outlen,
    _TCHAR *workingFolder,
	bool develop,
	bool silent,
    bool compiled
	)
{
 
    NLP_ENGINE::init(analyzer,workingFolder,develop,silent,compiled);

    // Analyzer can output to a stream.
    _TCHAR ofstr[MAXSTR];
    #ifdef LINUX
    _stprintf(ofstr,_T("./dummy.txt"));
    #else
    _stprintf(ofstr,_T("e:\\dummy.txt"));
    #endif
    _t_ofstream os(TCHAR2CA(ofstr), ios::out);
    
    _t_cout << _T("[infile path: ") << m_infile << _T("]") << endl;
    _t_cout << _T("[outfile path: ") << m_outfile << _T("]") << endl;

//_t_cout << _T("BEFORE ANALYSIS: ") << endl; // 09/27/20 AM.
//object_counts();    // TESTING analysis cleanup.    // 09/27/20 AM.

    m_nlp->analyze(m_infile, m_outfile, m_anadir, m_develop,
        m_silent,        // Debug/log output files.                  // 06/16/02 AM.
        0,             // Outdir.
        inbuf,           // Input buffer.
        len,        // Length of input buffer, or 0.
        m_compiled,      // If running compiled analyzer.
        &os,	   // Rebind cout output stream in analyzer    // 08/07/02 AM.
        outbuf,       // 05/11/02 AM.
        outlen	   // 05/11/02 AM.
        );

//_t_cout << _T("AFTER ANALYSIS: ") << endl; // 09/27/20 AM.
//object_counts();    // TESTING analysis cleanup.    // 09/27/20 AM.
    return 0;
}

int NLP_ENGINE::close()
{
    /////////////////////////////////////////////////
    // CLEANUP VISUALTEXT RUNTIME
    /////////////////////////////////////////////////


    // Compiled analyzers: need to close the user.dll for the application also.
    // Shutdown the runtime manager.

    VTRun::deleteVTRun(VTRun_Ptr);                      // 9/27/20 AM.
    VTRun_Ptr = 0;      // Clear out static var.        // 09/27/20 AM.
    _t_cout << _T("[AFTER VTRUN DELETE: ]") << endl;    // 09/27/20 AM.

    // Report memory leaks to standard output.
    object_counts();    // 09/27/20 AM.

    return 0;
}

// Remove a single named analyzer.
int NLP_ENGINE::close(_TCHAR *analyzer)
{
    /////////////////////////////////////////////////
    // CLEANUP VISUALTEXT RUNTIME
    /////////////////////////////////////////////////

    //NLP_ENGINE::close();

    // This will close the user.dll for the application also.
 //   m_vtrun->deleteNLP(m_nlp);                                         // 07/21/03 AM.
 //   VTRun::deleteVTRun(m_vtrun);                                     // 07/21/03 AM.
    m_nlp = VTRun_Ptr->findAna(analyzer);
    m_vtrun->rmAna(m_nlp);  // 09/27/20 AM.
    m_vtrun->deleteNLP(m_nlp);   // Remove analyzer from manager.    // 09/27/20 AM.

    return 0;
}

int NLP_ENGINE::createDir(_TCHAR *dirPath) {
    struct stat st;
    if (stat(dirPath,&st) != 0) {
#ifdef LINUX
	    mkdir(dirPath, 777);
#else
	    CreateDirectory(dirPath,NULL);
#endif
	    _t_cout << _T("[Creating output directory: ") << dirPath << _T("]") << endl;
	}
    return 0;
}