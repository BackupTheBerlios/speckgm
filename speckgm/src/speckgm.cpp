/******************************************************************************
** Spectrogram Viewer
** ---------------------------------------------------------------------------
** File:     speckgm.cpp
** Author:   V.Antonenko
** Version:  $Id: speckgm.cpp,v 1.2 2010/02/11 09:26:28 vladant Exp $
** License:  GNU
**
** This application shows 2D spectrogram of raw audio files.
******************************************************************************/

// For compilers that support precompilation, includes "wx/wx.h".
#include "wx/wxprec.h"

#ifdef __BORLANDC__
    #pragma hdrstop
#endif

// for all others, include the necessary headers (this file is usually all you
// need because it includes almost all "standard" wxWidgets headers)
#ifndef WX_PRECOMP
    #include "wx/wx.h"
#endif

// ----------------------------------------------------------------------------
// resources
// ----------------------------------------------------------------------------

// the application icon (under Windows and OS/2 it is in resources and even
// though we could still include the XPM here it would be unused)
#if !defined(__WXMSW__) && !defined(__WXPM__)
//    #include "speckgm.xpm"
#endif

#include <wx/file.h>
#include <algorithm>
#include "fft.h"

const unsigned int ORDER = 9; // 1 << 9 == 512
const unsigned int SAMPLE_RATE = 8000;

// ----------------------------------------------------------------------------
// private classes
// ----------------------------------------------------------------------------

class BaseView: public wxWindow
{
public:
	enum Align { NO, X, Y, XY };

	BaseView(wxWindow* parent);

	void SelectObject(const wxPen* pen);
	void SelectObject(const wxBrush* brush);
	void MoveTo(int x, int y);
	void LineTo(int x, int y);
	void DrawLine(int x1, int y1, int x2, int y2);
	void SetPixel(int x, int y, const wxColour& color);
	void SetPixel(int x, int y, const wxPen& pen);
	void PolyLine(wxPoint points[], int num_points);
	void Clear();
	void ClearRect(int x, int y, int w, int h);
	void ClearRect(const wxRect& rect);
	void FillRect(int x, int y, int w, int h);
	void FillRect(const wxRect& rect);
	void FillRect(int x, int y, int w, int h, const wxColour& color);
	void FillRect(int x0, int y0, int w, int h, const wxPen& pen);
	bool Scroll(int dx, int dy, const wxRect& rect);
	void TextOut(int x, int y, const wxString& str, Align align = NO);
	void DrawRotatedText(const wxString& text, wxCoord x, wxCoord y, double angle);
	void RePaint();

	int GetWidth() const { return m_rect.width; }
	int GetHeight() const { return m_rect.height; }
	const wxRect& GetDrawRect() const { return m_rect; }
	void GetWindowRect(wxRect& rect) { rect = GetRect(); }

protected:
	inline wxDC* GetDC(void) { return &m_memDC; }

	void OnEraseBackground(wxEraseEvent& WXUNUSED(event)) {}; //stub
	void OnPaint(wxPaintEvent& event);
	void OnSize(wxSizeEvent& event);

private:
	wxMemoryDC m_memDC;  // memory DC
	wxBitmap   m_bitmap; // bitmap associated with m_memDC
	wxBrush    m_brush;  // private brush
	wxPen      m_pen;    // private pen
	wxRect     m_rect;   // drawing rect, m_bitmap/m_memDC size
	wxPoint    m_moveto; // used by MoveTo/LineTo

	DECLARE_EVENT_TABLE()
};

BEGIN_EVENT_TABLE(BaseView, wxWindow)
	EVT_PAINT(BaseView::OnPaint)
	EVT_ERASE_BACKGROUND(BaseView::OnEraseBackground)
	EVT_SIZE(BaseView::OnSize)
END_EVENT_TABLE()


class WaveView : public BaseView
{
public:
	WaveView(wxWindow* pParentWnd);
	~WaveView();

	bool Init(unsigned buff_len);
	void Clear();
	void Draw(float samples[], unsigned size);

protected:
	void Copy(float buf[], unsigned size);
	void OnSize(wxSizeEvent& event);

private:
	wxRect      m_rect;
	wxPen       m_sclPen;
	wxPen       m_sigPen;
	unsigned    m_length;  // signal length
	unsigned    m_psize;   // points array length
	wxPoint     *m_points; // signal points

	DECLARE_EVENT_TABLE()
};

BEGIN_EVENT_TABLE(WaveView, BaseView)
	EVT_SIZE(WaveView::OnSize)
END_EVENT_TABLE()


class AfhView : public BaseView
{
	enum { eLevelScaleHeight = 10 };

public:
	AfhView(wxWindow* pParentWnd);
	~AfhView();

	bool Init(int buff_len);
	void Draw(float* pBuffer);
	void Draw(wxPoint points[], unsigned length);
	void DrawScale(void);
	void Clear(void);
	const wxRect& GetWorkRect() const { return m_rect; }
	void SetCursor(int x, int y);

protected:
	void Copy(float* pBuffer);
	void CopyInter(float* pBuffer);
	void CopyInter2(float* pBuffer);
	void OnLButtonDown(wxMouseEvent& event);
	void OnSize(wxSizeEvent& event);

private:
	wxRect      m_rect;    // working rect
	wxPoint     m_cursor;  // cursor position
	wxPen       m_sclPen;  // scale pen
	wxPen       m_sigPen;  // signal pen
	wxPoint     *m_points; // signal points
	unsigned    m_psize;   // signal points array size
	unsigned    m_length;  // signal length

	DECLARE_EVENT_TABLE()
};

BEGIN_EVENT_TABLE(AfhView, BaseView)
	EVT_LEFT_DOWN(AfhView::OnLButtonDown)
	EVT_SIZE(AfhView::OnSize)
END_EVENT_TABLE()


class DBScaleView : public BaseView
{
public:
	DBScaleView(wxWindow* pParentWnd): BaseView(pParentWnd) {};

protected:
	void Draw();
	void OnSize(wxSizeEvent& event);

private:
	DECLARE_EVENT_TABLE()
};

BEGIN_EVENT_TABLE(DBScaleView, BaseView)
	EVT_SIZE(DBScaleView::OnSize)
END_EVENT_TABLE()


class AmplitudeView : public BaseView
{
	enum {
		eLevelScaleWidth = 40,
		ePitchWidth      = 10,
		eEmptyScaleWidth = 50,
		eTimeScaleHeight = 25
	};

	const static int Amplitude2Db[7][2];

public:
	AmplitudeView(wxWindow* pParentWnd);

	bool Init(int sample_rate);
	void Clear();
	void DrawScale();
	void Draw(float pBuffer[], int size, int step, bool forward);
	void RePaint();
	const wxRect& GetWorkRect() const { return m_rect; }
	inline int GetWorkWidth() const	{ return m_rect.width; }

	inline void SetCursor(int x, int y)
		{ m_cursor.x = eLevelScaleWidth+x, m_cursor.y = y; }

	inline void SetTime( int time ) { m_Time = time; }

protected:
	void DoScroll(int dx);
	void OnPaint(wxPaintEvent& event);
	void OnLButtonDown(wxMouseEvent& event);
	void OnSize(wxSizeEvent& event);

private:
	int		m_Time;   // data for time span
	int     m_sample_rate;
	wxPoint m_cursor; // cursor position
	wxRect  m_rect;   // work rectangle

	DECLARE_EVENT_TABLE()
};

BEGIN_EVENT_TABLE(AmplitudeView, BaseView)
	EVT_PAINT(AmplitudeView::OnPaint)
	EVT_LEFT_DOWN(AmplitudeView::OnLButtonDown)
	EVT_SIZE(AmplitudeView::OnSize)
END_EVENT_TABLE()


class SpectrumView : public BaseView
{
	const int PITCH_WIDTH;
	const int FREQ_SCALE_WIDTH;
	const int LEVL_SCALE_PITCH;
	const int LEVL_SCALE_WIDTH;

public:
	SpectrumView(wxWindow* pParentWnd);
	~SpectrumView();

	void Init(int SampleRate, int n_samples);

	void Clear();
	void Draw0(float* dB, int size, bool forward);
	void Draw(float* dB, int size, bool forward);
	void DrawScale(int rate, int points);
	void DrawScale() { DrawScale(m_sample_rate, m_length); }
	const wxRect& GetWorkRect() const { return m_rect; }

protected:
	void DoScroll(int dx);
	void OnSize(wxSizeEvent& event);

private:
	wxRect   m_rect;
	int      m_length;
	int      m_sample_rate;
	int      m_num_pitch;
	wxString *m_strings;

	DECLARE_EVENT_TABLE()
};

BEGIN_EVENT_TABLE(SpectrumView, BaseView)
	EVT_SIZE(SpectrumView::OnSize)
END_EVENT_TABLE()

class wxQueue: public wxSemaphore
{
public:
	wxQueue();
	wxQueue(unsigned length, unsigned elsize);
	~wxQueue();

	bool Create(unsigned length, unsigned elsize);
	bool Get(void* ptr);
	bool Put(void* ptr);

private:
	wxCriticalSection cs;
	unsigned qsize;
	unsigned dsize;
	char     *data;
	unsigned nelem;
	unsigned nget;
	unsigned nput;
};

wxQueue::wxQueue(): wxSemaphore(0,1), qsize(0), dsize(0),
	data(NULL), nelem(0), nget(0), nput(0) {}

wxQueue::wxQueue(unsigned length, unsigned elsize): wxSemaphore(0,1),
	qsize(length), dsize(elsize), data(NULL), nelem(0), nget(0), nput(0)
{
	data = new char[qsize*dsize];
}

wxQueue::~wxQueue()
{
	if(data) delete[] data;
}

bool wxQueue::Create(unsigned length, unsigned elsize)
{
	qsize = length;
	dsize = elsize;

	data = new char[qsize*dsize];

	return data != NULL;
}

bool wxQueue::Get(void* ptr)
{
	cs.Enter();

	if (!nelem) {
		cs.Leave();
		return false;
	}

	memcpy(ptr, data + nget, dsize);
	--nelem; nget += dsize;
	if (nget >= qsize) nget = 0;

	cs.Leave();
	return true;
}

bool wxQueue::Put(void* ptr)
{
	cs.Enter();

	if (nelem == qsize) {
		cs.Leave();
		return false;
	}

	memcpy(data + nput, ptr, dsize);
	++nelem; nput += dsize;
	if (nput >= qsize) nput = 0;

	cs.Leave();
	return true;
}


// Define a new frame type: this is going to be our main frame
class DxViewFrame : public wxFrame, wxThread
{
	enum { Unsigned8bit, Signed16bit, Signed16bitBigEndian, Float32bit };
	struct dxEvent {
		int scroll;
	};

public:
    DxViewFrame(const wxString& title);

protected:
    // event handlers (these functions should _not_ be virtual)
    void OnQuit(wxCommandEvent& event);
    void OnAbout(wxCommandEvent& event);
	void OnHelp(wxCommandEvent& WXUNUSED(event)) {};
    void OnTest(wxCommandEvent& event);
    void OnScroll(wxCommandEvent& event);
	void OnSetFFTwin(wxCommandEvent& event);
	void OnOpen(wxCommandEvent& event);
	void OnStart(wxCommandEvent& WXUNUSED(event)) {};
	void OnNext(wxCommandEvent& event);
	void OnNext2(wxCommandEvent& event);
	void OnPrev(wxCommandEvent& event);
	void OnPrev2(wxCommandEvent& event);
	void OnBtZoomIn(wxCommandEvent& event);
	void OnBtZoomOut(wxCommandEvent& event);
	void OnLButtonDown(wxMouseEvent& event);
	void OnSize(wxSizeEvent& event);
	void OnUpdateUI(wxUpdateUIEvent& evnet);

	virtual long OnDxEvent(unsigned long evhandle) { return 0; };
	virtual int OnDxWrite(char *vox, char *cas, unsigned int size) { return 0; };
	virtual int OnDxRead(char *ptr, unsigned int size ) { return 0; };
	virtual long OnDxSeek(long pos, int flag) { return 0; };

	void RedrawAll();
	void DxScroll(int scroll);

	static void ConvertU8(float *dst, unsigned char *src, unsigned size);
	static void ConvertS16(float *dst, unsigned char *src, unsigned size);
	static void ConvertS16BE(float *dst, unsigned char *src, unsigned size);
	static void ConvertF32(float *dst, unsigned char *src, unsigned size);

	void SetFileFormat(int format);
	int ReadAndFft(int position);
	void FFT();

	virtual void* Entry(); // second thread

	inline void ENTER_FILE_CS() { m_hFileCS.Enter(); }
	inline void EXIT_FILE_CS()  { m_hFileCS.Leave(); }

	// samples conversion callback, size - src[] size in bytes
	void            (*cbConvertSamples) (float dst[], unsigned char src[], unsigned size);
	wxButton        *startButton;
	wxTextCtrl      *ShowScale;
	wxTextCtrl      *ShowTime;
	wxTextCtrl      *ShowAmplitude;
	wxTextCtrl      *ShowSpecAmp;
	wxTextCtrl      *ShowFreq;
	wxTextCtrl      *ShowFreqOfMax;
	wxTextCtrl      *ShowMaxSpecAmp;
	wxChoice        *setFFTwindow;
	wxChoice        *setFFTsize;
	wxFlexGridSizer *Sizer;

	SpectrumView    *spectrumView;
	AfhView         *afhView;
	AmplitudeView   *ampView;
	WaveView        *waveView;

	wxFile          m_file;
	wxCriticalSection m_hFileCS;
	wxQueue	        m_hHaveData;

	bool	IsStart;
	bool	m_run;  // to run thread

	unsigned char *m_buffer;

	float	*m_fwindow;  // FFT window coefs
	float	*m_fbuffer;  // normalized samples
	float	*m_fbuffer1; // FFT real buffer
	float	*m_fbuffer2; // FFT imaginary buffer
	float	*m_fdB;      // amplitude/frequency

	unsigned m_BiPS;    // bits per sample
	unsigned m_ByPS;    // bytes per sample
	unsigned m_buf_size;
	unsigned m_order;   // FFT order
	unsigned m_length;  // FFT size
	unsigned m_rd_size; // read-step size

	int		m_FilePosition;
	int		m_ampl_x;
	int     m_afc_freq;
	//int		m_spec_x;
	//int		m_spec_y;
	//WaveDev m_WaveDev;

	// any class wishing to process wxWidgets events must use this macro
    DECLARE_EVENT_TABLE()
};


// Define a new application type, each program should derive a class from wxApp
class DxViewApp : public wxApp
{
public:
    // override base class virtuals
    // ----------------------------

    // this one is called on application startup and is a good place for the app
    // initialization (doing it here and not in the ctor allows to have an error
    // return: if OnInit() returns false, the application terminates)
    virtual bool OnInit();
};

IMPLEMENT_APP(DxViewApp)

// 'Main program' equivalent: the program execution "starts" here
bool DxViewApp::OnInit()
{
    // call the base class initialization method, currently it only parses a
    // few common command-line options but it could be do more in the future
    if ( !wxApp::OnInit() )
        return false;

    // create the main application window
    DxViewFrame *frame = new DxViewFrame(_T("Spectrogram Viewer"));

    // and show it (the frames, unlike simple controls, are not shown when
    // created initially)
    frame->Show(true);

    // success: wxApp::OnRun() will be called which will enter the main message
    // loop and the application will run. If we returned false here, the
    // application would exit immediately.
    return true;
}

// ----------------------------------------------------------------------------
// constants
// ----------------------------------------------------------------------------

// IDs for the controls and the menu commands
enum
{
	// menu items
    ID_Test,
    ID_Scroll,
	ID_FFTwin,
	ID_FFTsize,
	ID_OnNext,
	ID_OnNext2,
	ID_OnPrev,
	ID_OnPrev2,
	ID_OnAfterSize,
	ID_OnAfterSome,
    ID_Help = wxID_HELP,
	ID_OnBtZoomIn = wxID_ZOOM_IN,
	ID_OnBtZoomOut = wxID_ZOOM_OUT,
    ID_Quit = wxID_EXIT,
    ID_Open = wxID_OPEN,
    ID_New = wxID_NEW,

    // it is important for the id corresponding to the "About" command to have
    // this standard value as otherwise it won't be handled properly under Mac
    // (where it is special and put into the "Apple" menu)
    ID_About = wxID_ABOUT
};

BEGIN_EVENT_TABLE(DxViewFrame, wxFrame)
    EVT_MENU(wxID_EXIT,   DxViewFrame::OnQuit)
    EVT_MENU(wxID_ABOUT,  DxViewFrame::OnAbout)
    EVT_MENU(ID_Test,   DxViewFrame::OnTest)
    EVT_MENU(ID_Scroll, DxViewFrame::OnScroll)

	EVT_BUTTON(wxID_EXIT, DxViewFrame::OnQuit)
	EVT_BUTTON(wxID_OPEN, DxViewFrame::OnOpen)
	EVT_BUTTON(wxID_NEW,  DxViewFrame::OnStart)
	EVT_BUTTON(wxID_HELP, DxViewFrame::OnHelp)
	EVT_BUTTON(ID_OnNext,  DxViewFrame::OnNext)
	EVT_BUTTON(ID_OnNext2, DxViewFrame::OnNext2)
	EVT_BUTTON(ID_OnPrev,  DxViewFrame::OnPrev)
	EVT_BUTTON(ID_OnPrev2, DxViewFrame::OnPrev2)
	EVT_BUTTON(wxID_ZOOM_IN,  DxViewFrame::OnBtZoomIn)
	EVT_BUTTON(wxID_ZOOM_OUT, DxViewFrame::OnBtZoomOut)
	EVT_CHOICE(ID_FFTwin, DxViewFrame::OnSetFFTwin)

	EVT_LEFT_DOWN(DxViewFrame::OnLButtonDown)
	EVT_SIZE(DxViewFrame::OnSize)
	EVT_UPDATE_UI_RANGE(ID_OnAfterSize, ID_OnAfterSome, DxViewFrame::OnUpdateUI)
END_EVENT_TABLE()

// ----------------------------------------------------------------------------
// main frame
// ----------------------------------------------------------------------------

const wxString sWinName(_T("Spectrogram Viewer - "));
const wxString file_name(_T("out.pcm"));

// frame constructor
DxViewFrame::DxViewFrame(const wxString& title)
	: wxFrame(NULL, wxID_ANY, title), wxThread(wxTHREAD_JOINABLE)
{
	m_rd_size = 8;
	m_order  = ORDER;
	m_length = 1 << m_order;
	m_buf_size = m_length * sizeof(float);
	m_buffer   = new unsigned char[m_buf_size];
	m_fwindow  = new float[m_length];
	m_fbuffer  = new float[m_length];
	m_fbuffer1 = new float[m_length+2];
	m_fbuffer2 = new float[m_length+2];
	m_fdB      = new float[m_length/2];

	if(NULL == m_fbuffer || NULL == m_fbuffer2 || NULL == m_fdB)
	{
		wxLogTrace(wxTRACE_MemAlloc, "  memory allocation problem\n");
		wxLogTrace(wxTRACE_MemAlloc, "  can't allocate m_fxxxx[]\n");
	}

	// create window for FFT
	dsp_window(m_fwindow, m_length, RECTANGULAR);
	for (unsigned i = 0; i < m_length; i++) m_fbuffer[i] = 0.0f;
	FFT();

	/* strcpy +
	if( theApp.m_lpCmdLine[0] != '\0' )
	{
		for( int i(0), k(0); theApp.m_lpCmdLine[i]; i++ )
			// copy all except quotes
			if( theApp.m_lpCmdLine[i] != '"' )
				file_name[k++] = theApp.m_lpCmdLine[i];
		file_name[k] = '\0';
	}*/

	SetFileFormat(Signed16bit);

	// open the default file
	if (m_file.Exists(file_name))
		m_file.Open(file_name);

	IsStart = false;
	m_FilePosition = 0;
	//-------------------------------------------------------------------------

	// set the frame icon
//    SetIcon(wxICON(sample));

#if wxUSE_MENUS
    // create a menu bar
    wxMenu *fileMenu = new wxMenu;
    // the "About" item should be in the help menu
    wxMenu *helpMenu = new wxMenu;

	helpMenu->Append(wxID_HELP,  _T("&Help\tF1"),      _T("Show help"));
    helpMenu->Append(wxID_ABOUT, _T("&About...\tF2"),  _T("Show about dialog"));
    fileMenu->Append(wxID_OPEN,  _T("&Open\tAlt-O"),   _T("Open audio file"));
    fileMenu->Append(wxID_NEW,   _T("&New\tAlt-N"),    _T("Record new audio file"));
    fileMenu->Append(wxID_EXIT,  _T("E&xit\tAlt-X"),   _T("Quit this program"));
#if __WXDEBUG__
	fileMenu->Append(ID_Test,    _T("&Test\tAlt-T"),   _T("The Test"));
    fileMenu->Append(ID_Scroll,  _T("&Scroll\tAlt-S"), _T("The Scroll Test"));
#endif
    // now append the freshly created menu to the menu bar...
    wxMenuBar *menuBar = new wxMenuBar();
    menuBar->Append(fileMenu, _T("&File"));
    menuBar->Append(helpMenu, _T("&Help"));
    // ... and attach this menu bar to the frame
    SetMenuBar(menuBar);
#endif // wxUSE_MENUS

#if wxUSE_STATUSBAR
    // create a status bar just for fun (by default with 1 pane only)
    CreateStatusBar(2);
    SetStatusText(_T("Welcome to Spectrogram Viewer!"));
#endif // wxUSE_STATUSBAR

	spectrumView = new SpectrumView(this);
	spectrumView->Init(SAMPLE_RATE, m_length);

	afhView = new AfhView(this);
	afhView->Init(m_length/2);
	m_afc_freq = 0;

	ampView = new AmplitudeView(this);
	ampView->Init(SAMPLE_RATE);
	m_ampl_x = ampView->GetWorkWidth()-1;
	ampView->SetCursor(m_ampl_x, 0);

	waveView = new WaveView(this);
	waveView->Init(m_length);

	wxSizerFlags sFlags, sFlags2;

	wxBoxSizer *buttonSizer = new wxBoxSizer(wxVERTICAL);
	sFlags.Proportion(0).Border(wxALL,5).Center();

	//buttonSizer->Add(new wxButton(this, wxID_EXIT), sFlags);
	buttonSizer->Add(new wxButton(this, wxID_OPEN), sFlags);
	buttonSizer->Add(startButton = new wxButton(this, wxID_NEW ), sFlags);
	//buttonSizer->Add(new wxButton(this, wxID_HELP), sFlags);

	setFFTwindow = new wxChoice(this, ID_FFTwin);
	setFFTwindow->Append(_T("Rectangular"));
	setFFTwindow->Append(_T("Bartlett"));
	setFFTwindow->Append(_T("Hamming"));
	setFFTwindow->Append(_T("Hanning"));
	setFFTwindow->Append(_T("Blackman"));
	setFFTwindow->Append(_T("Welch"));
	setFFTwindow->SetSelection(RECTANGULAR);
	buttonSizer->Add(new wxStaticText(this, wxID_ANY, _T("FFT Window")), wxSizerFlags().Center());
	buttonSizer->Add(setFFTwindow, wxSizerFlags(0).Border(wxLEFT|wxRIGHT,5).Center());

	setFFTsize = new wxChoice(this, ID_FFTsize);
	setFFTsize->Append(_T("2048"));
	setFFTsize->Append(_T("1024"));
	setFFTsize->Append(_T("512"));
	setFFTsize->Append(_T("256"));
	setFFTsize->Append(_T("128"));
	setFFTsize->Append(_T("64"));
	setFFTsize->SetSelection(2);
	buttonSizer->Add(new wxStaticText(this, wxID_ANY, _T("FFT Size")), wxSizerFlags().Center());
	buttonSizer->Add(setFFTsize, wxSizerFlags(0).Border(wxLEFT|wxRIGHT,5).Center());

	ShowSpecAmp    = new wxTextCtrl(this, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, wxTE_READONLY|wxTE_CENTER);
	ShowFreq       = new wxTextCtrl(this, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, wxTE_READONLY|wxTE_CENTER);

	sFlags2.Proportion(0).Border(wxUP,1).Center();
	sFlags.Proportion(0).Border(wxDOWN|wxLEFT|wxRIGHT,1).Center();
	buttonSizer->Add(new wxStaticText(this, wxID_ANY, _T("Spectra Amp:")), sFlags2);
	buttonSizer->Add(ShowSpecAmp,    sFlags);
	buttonSizer->Add(new wxStaticText(this, wxID_ANY, _T("Frequency:")), sFlags2);
	buttonSizer->Add(ShowFreq,       sFlags);

	wxBoxSizer *navySizer = new wxBoxSizer(wxHORIZONTAL);

	ShowScale = new wxTextCtrl(this, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, wxTE_READONLY|wxTE_CENTRE);
	wxButton *btZoomIn  = new wxButton(this, wxID_ZOOM_IN,  _T("-"));
	wxButton *btZoomOut = new wxButton(this, wxID_ZOOM_OUT, _T("+"));
	wxButton *btPrev2   = new wxButton(this, ID_OnPrev2, _T("<<"));
	wxButton *btPrev    = new wxButton(this, ID_OnPrev,  _T("<" ));
	wxButton *btNext    = new wxButton(this, ID_OnNext,  _T(">" ));
	wxButton *btNext2   = new wxButton(this, ID_OnNext2, _T(">>"));

	wxSize sz;
	sz = ShowScale->GetEffectiveMinSize();
	sz.x = sz.y + sz.y/2;
	ShowScale->SetMinSize(sz);

	sz = btZoomIn->GetEffectiveMinSize();
	sz.x = sz.y + sz.y/2;
	btZoomIn->SetMinSize(sz);
	btZoomOut->SetMinSize(sz);
	btPrev2->SetMinSize(sz);
	btPrev->SetMinSize(sz);
	btNext->SetMinSize(sz);
	btNext2->SetMinSize(sz);

	sFlags.Proportion(0).Border(wxALL,3).Left();

	navySizer->Add(btZoomIn,  sFlags);
	navySizer->Add(ShowScale, sFlags);
	navySizer->Add(btZoomOut, sFlags);
	navySizer->AddStretchSpacer();
	sFlags.Right();
	navySizer->Add(btPrev2,   sFlags);
	navySizer->Add(btPrev,    sFlags);
	navySizer->Add(btNext,    sFlags);
	navySizer->Add(btNext2,   sFlags);

	wxBoxSizer *showSizer = new wxBoxSizer(wxVERTICAL);

	ShowTime       = new wxTextCtrl(this, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, wxTE_READONLY|wxTE_CENTER);
	ShowAmplitude  = new wxTextCtrl(this, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, wxTE_READONLY|wxTE_CENTER);
	ShowFreqOfMax  = new wxTextCtrl(this, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, wxTE_READONLY|wxTE_CENTER);
	ShowMaxSpecAmp = new wxTextCtrl(this, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, wxTE_READONLY|wxTE_CENTER);

	sFlags.Proportion(0).Border(wxDOWN|wxLEFT|wxRIGHT,1).Center();
	sFlags2.Proportion(0).Border(wxUP,1).Center();

	showSizer->Add(new wxStaticText(this, wxID_ANY, _T("Time:")), sFlags2);
	showSizer->Add(ShowTime,       sFlags);
	showSizer->Add(new wxStaticText(this, wxID_ANY, _T("Amplitude:")), sFlags2);
	showSizer->Add(ShowAmplitude,  sFlags);
	showSizer->Add(new wxStaticText(this, wxID_ANY, _T("Max.Spec.Amp:")), sFlags2);
	showSizer->Add(ShowMaxSpecAmp, sFlags);
	showSizer->Add(new wxStaticText(this, wxID_ANY, _T("Its Freq:")), sFlags2);
	showSizer->Add(ShowFreqOfMax,  sFlags);

	Sizer = new wxFlexGridSizer(3, 3, 3, 3);
	Sizer->SetFlexibleDirection(wxBOTH);
	sFlags.Border(wxALL,1).Expand();
	Sizer->Add(spectrumView, sFlags);
	Sizer->Add(afhView,      sFlags);
	Sizer->Add(buttonSizer,  sFlags);
	Sizer->Add(navySizer,    sFlags);
	Sizer->Add(new DBScaleView(this), sFlags);
	Sizer->AddStretchSpacer();
	Sizer->Add(ampView,      sFlags);
	Sizer->Add(waveView,     sFlags);
	Sizer->Add(showSizer,    sFlags);
	Sizer->AddGrowableCol(0);
	Sizer->AddGrowableCol(1);
	Sizer->AddGrowableRow(0);
	Sizer->AddGrowableRow(2);

	SetSizerAndFit(Sizer);
	SetAutoLayout(true);

	wxAcceleratorEntry entries[12];
	entries[ 0].Set(wxACCEL_CTRL,   (int) 'E',  wxID_EXIT);
	entries[ 1].Set(wxACCEL_CTRL,   (int) 'O',  wxID_OPEN);
	entries[ 2].Set(wxACCEL_CTRL,   (int) 'N',  wxID_NEW);
	entries[ 3].Set(wxACCEL_CTRL,   (int) 'H',  wxID_HELP);
	entries[ 4].Set(wxACCEL_CTRL,   (int) '-',  wxID_ZOOM_IN);
	entries[ 5].Set(wxACCEL_NORMAL, WXK_DOWN,   wxID_ZOOM_IN);
	entries[ 6].Set(wxACCEL_CTRL,   (int) '+',  wxID_ZOOM_OUT);
	entries[ 7].Set(wxACCEL_NORMAL, WXK_UP,     wxID_ZOOM_OUT);
	entries[ 8].Set(wxACCEL_NORMAL, WXK_LEFT,   ID_OnPrev);
	entries[ 9].Set(wxACCEL_CTRL,   WXK_LEFT,   ID_OnPrev2);
	entries[10].Set(wxACCEL_NORMAL, WXK_RIGHT,  ID_OnNext);
	entries[11].Set(wxACCEL_CTRL,   WXK_RIGHT,  ID_OnNext2);
	wxAcceleratorTable accel(12, entries);
	SetAcceleratorTable(accel);

	SetName(sWinName+file_name);

	wxString str;
	str.Printf(_T("%d"), m_rd_size);
	ShowScale->ChangeValue(str);

	//m_hHaveData.Create(16, sizeof(int));

	//m_run = true;
	//wxThread::Create();
	//wxThread::Run();
}


// event handlers

void DxViewFrame::OnQuit(wxCommandEvent& WXUNUSED(event))
{
	m_run = false; // try to stop thread;
	//wxThread::Wait();

	if (m_file.IsOpened()) m_file.Close();
	delete[] m_buffer;

	if(m_fwindow) {
		wxLogTrace(wxTRACE_MemAlloc, "delete m_fwindow [0x%08X]\n",m_fwindow);
		delete[] m_fwindow;
	}

	if(m_fbuffer) {
		wxLogTrace(wxTRACE_MemAlloc, "delete m_fbuffer [0x%08X]\n",m_fbuffer);
		delete[] m_fbuffer;
	}

	if(m_fbuffer1) {
		wxLogTrace(wxTRACE_MemAlloc, "delete m_fbuffer1 [0x%08X]\n",m_fbuffer1);
		delete[] m_fbuffer1;
	}

	if(m_fbuffer2) {
		wxLogTrace(wxTRACE_MemAlloc, "delete m_fbuffer2 [0x%08X]\n",m_fbuffer2);
		delete[] m_fbuffer2;
	}

	if(m_fdB) {
		wxLogTrace(wxTRACE_MemAlloc, "delete m_fdB [0x%08X]\n",m_fdB);
		delete[] m_fdB;
	}

	// true is to force the frame to close
	Close(true);
}

void DxViewFrame::OnAbout(wxCommandEvent& WXUNUSED(event))
{
    wxMessageBox(wxString::Format(
                    _T("Welcome to %s!\n")
                    _T("\n")
                    _T("This is the Spectrum View\n")
                    _T("running under %s."),
                    wxVERSION_STRING,
                    wxGetOsDescription().c_str()
                 ),
                 _T("About Spectrum View"),
                 wxOK | wxICON_INFORMATION,
                 this);
}

void DxViewFrame::OnTest(wxCommandEvent& WXUNUSED(event))
{
/*
	FILE *file = fopen("test2.txt", "w+");
	for (unsigned i = 0; i < m_length/2; i++) {
		fprintf(file, "%f\n", m_fbuffer[i]);
	}
	fclose(file);
*/
	spectrumView->Draw(m_fdB, m_length, 1);
	spectrumView->Refresh(false);
}

void DxViewFrame::OnScroll(wxCommandEvent& WXUNUSED(event))
{
	waveView->Scroll(10, 0, waveView->GetRect());
	waveView->RePaint();
}

void DxViewFrame::OnSetFFTwin(wxCommandEvent& WXUNUSED(event))
{
	int n = setFFTwindow->GetSelection();
	dsp_window(m_fwindow, m_length, n);
}

void DxViewFrame::OnOpen(wxCommandEvent& WXUNUSED(event))
{
	wxFileDialog fileDlg(this);

	fileDlg.SetWildcard(_T(
		"Raw 8bit,unsigned PCM (*.pcm)|*.pcm|"
		"Raw 16bit,signed PCM (*.pcm)|*.pcm|"
		"Raw 16bit,signed,BE PCM (*.pcm)|*.pcm|"
		"Raw Float,32bit PCM (*.pcm)|*.pcm|"
		"|"));

	if( fileDlg.ShowModal() == wxID_OK )
	{
		if (m_file.IsOpened()) m_file.Close();

		SetFileFormat(fileDlg.GetFilterIndex());

		if(!m_file.Open(fileDlg.GetPath()))
			wxMessageBox(_T("Cannot open the file"), _T("Error"), wxICON_ERROR, this);

		m_FilePosition = 0;
		spectrumView->Clear();
		spectrumView->RePaint();
		ampView->Clear();
		//ampView->SetCursor(m_ampl_x, 0);
		ampView->SetTime(m_FilePosition);
		ampView->RePaint();
		waveView->Clear();
		waveView->RePaint();
		afhView->Clear();
		afhView->RePaint();

		SetName(sWinName+fileDlg.GetFilename());
	}
}

void DxViewFrame::OnNext(wxCommandEvent& WXUNUSED(event))
{
	if( !IsStart )
	{
		DxScroll(1);
	}
}

void DxViewFrame::OnNext2(wxCommandEvent& WXUNUSED(event))
{
	if( !IsStart )
	{
		DxScroll(10);
	}
}

void DxViewFrame::OnPrev(wxCommandEvent& WXUNUSED(event))
{
	if( !IsStart )
	{
		DxScroll(-1);
	}
}

void DxViewFrame::OnPrev2(wxCommandEvent& WXUNUSED(event))
{
	if( !IsStart )
	{
		DxScroll(-10);
	}
}

// Zoom in
void DxViewFrame::OnBtZoomIn(wxCommandEvent& WXUNUSED(event))
{
	if( m_rd_size > 8 )
	{
		m_rd_size /= 2;

		if( IsStart ) {
			spectrumView->Clear();
			ampView->Clear();
		}
		else {
			RedrawAll();
		}

		wxString str;
		str.Printf(_T("%d"), m_rd_size);
		ShowScale->ChangeValue(str);
	}
}

// Zoom out
void DxViewFrame::OnBtZoomOut(wxCommandEvent& WXUNUSED(event))
{
	if( m_rd_size < m_length )
	{
		m_rd_size *= 2;

		if( IsStart ) {
			spectrumView->Clear();
			ampView->Clear();
		}
		else {
			RedrawAll();
		}

		wxString str;
		str.Printf(_T("%d"), m_rd_size);
		ShowScale->ChangeValue(str);
	}
}

// Mouse left button click handling
void DxViewFrame::OnLButtonDown(wxMouseEvent& event)
{
	/*
	** Here we handle unusual mouse events: event positions
	** are given not in this window coordinates but in a child
	** coordinates. This is because these events are resent
	** from the corresponding child window. It is impossible
	** to distinguish this event by its position so we compare
	** the event object pointer with a child window pointer
	** in such a way finding the event source.
	*/

	if( !IsStart && (event.GetEventObject() == ampView) ) {
		const wxPoint& point = event.GetPosition();
		const wxRect& rect = ampView->GetWorkRect();

		m_ampl_x = point.x - rect.x;
		ampView->SetCursor(m_ampl_x, 0);
		DxScroll(0);
	}

	if( event.GetEventObject() == afhView ) {
		const wxPoint& point = event.GetPosition();
		const wxRect& rect = afhView->GetWorkRect();

		int spec_x = point.x - rect.x;
		int spec_y = point.y - rect.y;
		// current AFC frequence (0..Fn/2)->(0..m_length/2)
		m_afc_freq = (rect.GetHeight() - (point.y - rect.y))*m_length/(2*rect.GetHeight());

		afhView->SetCursor(spec_x, spec_y);

		if( !IsStart ) {
			DxScroll(0);
		}
	}
}

void DxViewFrame::OnSize(wxSizeEvent& event)
{
	event.Skip();

	if (IsShown()) {
		// to send UpdateUI
		wxUpdateUIEvent ev(ID_OnAfterSize);
		AddPendingEvent(ev);
	}
}

void DxViewFrame::OnUpdateUI(wxUpdateUIEvent& event)
{
	switch (event.GetId())
	{
	case ID_OnAfterSize:
		RedrawAll();
		break;
	case ID_OnAfterSome:
		break;
	}
}

// redraw all drawing windows
void DxViewFrame::RedrawAll()
{
	spectrumView->DrawScale();
	ampView->DrawScale();
	afhView->DrawScale();

	// put the cursor in the utmost right position
	m_ampl_x = ampView->GetWorkWidth()-1;
	ampView->SetCursor(m_ampl_x, 0);

	if( !IsStart && m_file.IsOpened())
	{
		unsigned int count = ampView->GetWorkWidth()/2;

		//ampView->SetTime(m_FilePosition - (count-1)*m_rd_size);

		while(count-->0)
		{
			const int pos = m_FilePosition - count*m_rd_size;
			const int res = ReadAndFft(pos-m_length/2);

			if( res < 0 ) break;

			ampView->SetTime(pos);
			ampView->Draw(m_fbuffer, m_length, (res>0)? m_rd_size: 0, true);
			spectrumView->Draw(m_fdB, m_length, true);
		}

		spectrumView->Refresh(false);//RePaint();
		ampView->Refresh(false);//RePaint();
		afhView->Refresh(false);//RePaint();

		DxScroll(0);
	}
}

void DxViewFrame::DxScroll(int scroll)
{
	int      pos; // ATTENTION! 'pos' could be uninitialized
	unsigned nsteps;
	bool     forward;

	if (scroll >= 0) {
		forward = true; nsteps = scroll;
	} else {
		forward = false; nsteps = -scroll;
	}

	while (nsteps-->0) {
		if (forward)
			pos = m_FilePosition + m_rd_size;
		else // work width should be an even number...
			pos = m_FilePosition - ampView->GetWorkWidth()*m_rd_size/2;

		if (ReadAndFft(pos-m_length/2) <= 0) break;

		// update the position only if ReadAndFft returned OK
		m_FilePosition += (forward)? int(m_rd_size): -int(m_rd_size);

		ampView->Draw(m_fbuffer, m_length, m_rd_size, forward);
		spectrumView->Draw(m_fdB, m_length, forward);
	}

	if (scroll) {
		spectrumView->Refresh(false);
		ampView->Refresh(false);
	} else {
		ampView->RePaint();
	}

	/* show data under the cursor */
	if( !IsStart ) {
		pos = m_FilePosition - (ampView->GetWorkWidth()-1-m_ampl_x)*m_rd_size/2;
		if( ReadAndFft(pos-m_length/2) < 0 ) return;
	}

	wxString str;
	str.Printf(_T("%.3f s"), float(pos)/SAMPLE_RATE);
	ShowTime->ChangeValue(str);

	int max_amp = (*std::max_element(m_fbuffer, m_fbuffer+m_length))*100;
	str.Printf(_T("%d %%"), max_amp);
	ShowAmplitude->ChangeValue(str);

	str.Printf(_T("%d Hz"), SAMPLE_RATE*m_afc_freq/m_length);
	ShowFreq->ChangeValue(str);
	str.Printf(_T("%.2f dB"), m_fdB[m_afc_freq]);
	ShowSpecAmp->ChangeValue(str);

	// index in the array
	int max_spec_amp = std::max_element(m_fdB, m_fdB+m_length/2)-m_fdB;
	int freq_of_max = SAMPLE_RATE*max_spec_amp/m_length;

	str.Printf(_T("%d Hz"), freq_of_max);
	ShowFreqOfMax->ChangeValue(str);
	str.Printf(_T("%.2f dB"), m_fdB[max_spec_amp]);
	ShowMaxSpecAmp->ChangeValue(str);

	waveView->Draw(m_fbuffer, m_length);
	afhView->Draw(m_fdB);
}

// Thread
void* DxViewFrame::Entry()
{
	while( m_run )
	{
		int scroll;

		if( m_hHaveData.WaitTimeout(1000) != wxSEMA_NO_ERROR )
			continue;

		while (m_hHaveData.Get(&scroll))
		{
			DxScroll(scroll);
			//wxCommandEvent event(wxEVT_COMMAND_MENU_SELECTED, ID_Test);
			//AddPendingEvent(event);
		}
	}

	return NULL;
}


void DxViewFrame::FFT()
{
	dsp_window_apply(m_fbuffer1, m_fbuffer, m_fwindow, m_length);
	dsp_realfft(m_fbuffer1, m_fbuffer2, m_length, 1);
	dsp_rect2polar(m_fbuffer1, m_fbuffer2, m_length);

	const unsigned length2 = m_length/2;
	const float    k = 1.0f/length2;

	for(unsigned i = 0; i < length2; i++) {// something is wrong in the calculations...
		float dB = 20.0f * (float)log10(m_fbuffer1[i] * k);
		m_fdB[i] = (dB < -100.0f)? -100.0f: dB;
	}
}

// Reading from a file + doing FFT
// i.e. making all necessary data to show
int DxViewFrame::ReadAndFft(int pos)
{
	int res;

	if (!m_file.IsOpened()) return 0;

	/* a little complicated situation when pos is less than zero */
	if( pos < 0 ) {
		if( pos < -int(m_length) ) pos = -int(m_length);
		int datapos = pos*m_ByPS; // position * bytes_per_sample

		ENTER_FILE_CS();
		res = m_file.Seek(0, wxFromStart);
		if( res < 0 ) {
			EXIT_FILE_CS();
			return res;
		}
		res = m_file.Read(m_buffer - datapos, m_buf_size + datapos);
		EXIT_FILE_CS();

		if( res < 0 ) return res;

		// fill empty space in buffer
		for(int i = 0; i < -pos; i++) m_fbuffer[i] = 0.0f;
		// convert/normalist samples
		cbConvertSamples(m_fbuffer-pos, m_buffer-datapos, res);
	}
	else {
		ENTER_FILE_CS();
		int datapos = pos*m_ByPS; // position * bytes_per_sample

		res = m_file.Seek(datapos, wxFromStart);
		if( res < 0 ) {
			EXIT_FILE_CS();
			return res;
		}
		res = m_file.Read(m_buffer, m_buf_size);
		EXIT_FILE_CS();

		if( res < 0 ) return res;

		if( IsStart && res < int(m_buf_size) ) return 0;

		if( res < int(m_buf_size) ) {
			for(unsigned i = res/m_ByPS; i < m_length; i++)
				m_fbuffer[i] = 0.0f;
		}

		// convert/normalist samples
		cbConvertSamples(m_fbuffer, m_buffer, res);
	}

	// FFT - analysis
	FFT();

	return res;
}

// size - buffer length
void DxViewFrame::ConvertU8(float dst[], unsigned char src[], unsigned size)
{
	for(unsigned i = 0; i < size; i++)
		dst[i] = (float)(src[i]-128)/127.0f;
}

void DxViewFrame::ConvertS16(float dst[], unsigned char src[], unsigned size)
{
	unsigned size2 = size/sizeof(short);
	short    *src2 = (short*) src;

	for(unsigned i = 0; i < size2; i++)
		dst[i] = float(src2[i])/32767.0f;
}

void DxViewFrame::ConvertS16BE(float dst[], unsigned char src[], unsigned size)
{
	unsigned size2 = size/sizeof(short);
	//short    *src2 = (short*) src;

	for(unsigned i = 0; i < size2; i++)
		dst[i] = float((short)((src[i*2] << 8) | src[i*2+1]))/32767.0f;
}

void DxViewFrame::ConvertF32(float dst[], unsigned char src[], unsigned size)
{
	unsigned size2 = size/sizeof(float);
	float    *src2 = (float*) src;

	for(unsigned i = 0; i < size2; i++)
		dst[i] = src2[i];
}

void DxViewFrame::SetFileFormat(int format)
{
	switch(format)
	{
	case Unsigned8bit:
		m_BiPS = 8; // bits per sample
		m_ByPS = 1;  // bytes per sample
		cbConvertSamples = ConvertU8;
		break;

	case Signed16bit:
		m_BiPS = 16; // bits per sample
		m_ByPS = 2;  // bytes per sample
		cbConvertSamples = ConvertS16;
		break;

	case Signed16bitBigEndian:
		m_BiPS = 16; // bits per sample
		m_ByPS = 2;  // bytes per sample
		cbConvertSamples = ConvertS16BE;
		break;

	case Float32bit:
		m_BiPS = 32; // bits per sample
		m_ByPS = 4;  // bytes per sample
		cbConvertSamples = ConvertF32;
		break;
	}

	m_buf_size = m_length * m_ByPS;
}


#define FACTOR 100

/******************************************************************************
**  BaseView class
**  --------------------------------------------------------------------------
**  BaseView class is derived from wxWindow.
**  Provides a functionality for easy drawing in the client area, and will be
**  used in wave, AFH, amplitude and spectra drawing classes.
**  It draws in memory DC then copy its content into window in OnPaint().
**
**  Parameters:
**      pParentWnd - parent window;
**
******************************************************************************/
BaseView::BaseView(wxWindow* parent):
	wxWindow(parent, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxBORDER_SIMPLE)
{
	m_rect = GetClientRect();

	m_bitmap.Create(m_rect.width, m_rect.height);
	m_memDC.SelectObject(m_bitmap);

	m_memDC.SetPen(*wxWHITE_PEN);
	m_memDC.SetBrush(*wxBLACK_BRUSH);
	m_memDC.SetFont(*wxNORMAL_FONT);
	m_memDC.SetBackground(*wxBLACK_BRUSH);
	m_memDC.SetBackgroundMode(wxTRANSPARENT);
	m_memDC.SetTextForeground(*wxBLACK);
	//m_memDC.SetClippingRegion(m_rect);

	m_memDC.Clear();
}


void BaseView::Clear(void)
{
	m_memDC.Clear();
}

void BaseView::ClearRect(int x, int y, int w, int h)
{
	// clear with background brush
	m_memDC.Blit(x, y, w, h, &m_memDC, 0, 0, wxCLEAR);
}

void BaseView::ClearRect(const wxRect& rect)
{
	ClearRect(rect.x, rect.y, rect.width, rect.height);
}

void BaseView::SelectObject(const wxPen* pen)
{
	m_memDC.SetPen(*pen);
}


void BaseView::SelectObject(const wxBrush* brush)
{
	m_memDC.SetBrush(*brush);
}


void BaseView::MoveTo(int x, int y)
{
	m_moveto = wxPoint(x,y);
}


void BaseView::LineTo(int x, int y)
{
	m_memDC.DrawLine(m_moveto.x, m_moveto.y, x, y);
	MoveTo(x, y);
}

void BaseView::DrawLine(int x1, int y1, int x2, int y2)
{
	m_memDC.DrawLine(x1, y1, x2, y2);
}

void BaseView::SetPixel(int x, int y, const wxColour& color)
{
	m_pen.SetColour(color);

	m_memDC.SetPen(m_pen);
	m_memDC.DrawPoint(x, y);
}

void BaseView::SetPixel(int x, int y, const wxPen& pen)
{
	m_memDC.SetPen(pen);
	m_memDC.DrawPoint(x, y);
}


void BaseView::PolyLine(wxPoint points[], int num_points)
{
	m_memDC.DrawLines(num_points, points);
}


void BaseView::FillRect(int x, int y, int w, int h)
{
	m_memDC.DrawRectangle(x, y, w, h);
}

void BaseView::FillRect(const wxRect& rect)
{
	m_memDC.DrawRectangle(rect);
}

void BaseView::FillRect(int x0, int y0, int w, int h, const wxColour& color)
{
	m_pen.SetColour(color);
	m_brush.SetColour(color);

	m_memDC.SetPen(m_pen);
	m_memDC.SetBrush(m_brush);
	m_memDC.DrawRectangle(x0, y0, w, h);
}

void BaseView::FillRect(int x0, int y0, int w, int h, const wxPen& pen)
{
	m_brush.SetColour(pen.GetColour());

	m_memDC.SetPen(pen);
	m_memDC.SetBrush(m_brush);
	m_memDC.DrawRectangle(x0, y0, w, h);
}


bool BaseView::Scroll(int dx, int dy, const wxRect& rect)
{
	wxCoord dstx,dsty;
	wxCoord srcx,srcy;
	wxCoord w,h;

	if (dx < 0) {
		dstx = rect.x;
		srcx = rect.x-dx;
		w = rect.width+dx;
	} else {
		dstx = rect.x+dx;
		srcx = rect.x;
		w = rect.width-dx;
	}

	if (dy < 0) {
		dsty = rect.y;
		srcy = rect.y+dy;
		h = rect.height+dy;
	} else {
		dsty = rect.y+dy;
		srcy = rect.y;
		h = rect.height-dy;
	}

	return m_memDC.Blit(dstx, dsty, w, h, &m_memDC, srcx, srcy);
	//return m_memDC.Blit(rect.x, rect.y, rect.width, rect.height, &m_memDC, rect.x-dx, rect.y-dy);
}


void BaseView::OnPaint(wxPaintEvent& WXUNUSED(event))
{
	wxPaintDC DC(this);

	wxRegionIterator upd(GetUpdateRegion()); // get the update rect list

	do {
		wxRect rect(upd.GetRect());

		DC.Blit(rect.x, rect.y, rect.width, rect.height, &m_memDC, rect.x, rect.y, wxCOPY);

		upd++;
	}
	while (upd);
}


void BaseView::RePaint()
{
	wxClientDC DC(this);
	DC.Blit(0, 0, m_rect.width, m_rect.height, &m_memDC, 0, 0, wxCOPY);
}


void BaseView::OnSize(wxSizeEvent& event)
{
	m_rect = GetClientRect();

	m_bitmap.Create(m_rect.GetWidth(), m_rect.GetHeight());
	m_memDC.SelectObject(m_bitmap);
}


void BaseView::TextOut(int x, int y, const wxString& str, Align align)
{
	if(align != NO)
	{
		wxSize te = m_memDC.GetTextExtent(str);
		if (align & X) x -= te.x/2;
		if (align & Y) y -= te.y/2;
	}

	m_memDC.DrawText(str, x, y);
}

void BaseView::DrawRotatedText(const wxString& str, wxCoord x, wxCoord y, double angle)
{
	m_memDC.DrawRotatedText(str, x, y, angle);
}

/******************************************************************************
**  WaveView class
**  --------------------------------------------------------------------------
**  WaveView class is derived from BaseView. It draws a part of audio wave
**  in the real scale.
**
**  Parameters:
**      pParentWnd - parent window;
**
******************************************************************************/
WaveView::WaveView(wxWindow* pParentWnd): BaseView(pParentWnd), m_points(NULL)
{
}

WaveView::~WaveView()
{
	if(m_points) delete[] m_points;
}

bool WaveView::Init(unsigned num_points)
{
	m_sclPen.SetColour(0,128,0);
	m_sigPen.SetColour(0,255,0);

	if(m_points) {
		delete[] m_points;
		m_points = NULL;
	}

	m_length = 0;
	m_psize = num_points;
	m_points = new wxPoint[m_psize];

	if(!m_points)
		return false;

	// set window size
	SetMinSize(wxSize(10, 10));
	//SetMaxSize(wxSize(50, 50));
	Clear();

	m_rect = BaseView::GetDrawRect();

	return true;
}


void WaveView::Copy(float samples[], unsigned nsamples)
{
	const unsigned height2 = m_rect.height/2;
	unsigned width = m_rect.width;
	unsigned xstep = 1; // X-axis step size

	// if array is bigger than we can store in m_points[]
	if (nsamples > m_psize) {
		samples += (nsamples - m_psize)/2;
		nsamples = m_psize;
	}

	// scaling by X-axis if window is wider than diagram
	if (nsamples < width) {
		xstep += width/nsamples;
		width /= xstep;
	}

	// adjust samples ptr. so we will show the data in the center
	if (nsamples > width) {
		samples += (nsamples - width)/2;
		nsamples = width;
	}

	m_length = nsamples;

	for(unsigned i = 0, x = m_rect.x; i < nsamples; i++, x += xstep) {
		m_points[i].x = x;
		m_points[i].y = (1.0f-samples[i])*height2;
	}
}

void WaveView::Draw(float samples[], unsigned nsamples)
{
	Clear();
	Copy(samples, nsamples);
	SelectObject(&m_sigPen);
	PolyLine(m_points, m_length);
	Refresh(false);
}

void WaveView::Clear()
{
	//m_rect = GetClientRect();

	BaseView::ClearRect(m_rect);

	SelectObject(&m_sclPen);

	MoveTo(m_rect.x, m_rect.height / 2);
	LineTo(m_rect.width, m_rect.height / 2);

	MoveTo(m_rect.x, m_rect.height / 4);
	LineTo(m_rect.width, m_rect.height / 4);

	MoveTo(m_rect.x, m_rect.height * 3 / 4);
	LineTo(m_rect.width, m_rect.height * 3 / 4);

	MoveTo((m_rect.width / 2) + m_rect.x, 0);
	LineTo((m_rect.width / 2) + m_rect.x, m_rect.height);

	MoveTo((m_rect.width / 4) + m_rect.x, 0);
	LineTo((m_rect.width / 4) + m_rect.x, m_rect.height);

	MoveTo((m_rect.width * 3 / 4) + m_rect.x, 0);
	LineTo((m_rect.width * 3 / 4) + m_rect.x, m_rect.height);
}

void WaveView::OnSize(wxSizeEvent& event)
{
	BaseView::OnSize(event);

	m_rect = BaseView::GetDrawRect();

	Clear();
	Refresh(false);
}

/******************************************************************************
**  AfhView
**  --------------------------------------------------------------------------
**  AfhView class is derived from BaseView. It draws the amplitude-frequency
**  characteristic (AFC) of an audio wave.
**
**  Parameters:
**      pParentWnd - parent window;
**
******************************************************************************/
const float AFH_DB_SCALE = 100; // number of dBs shown in the window

const float i2dB[] =
{
	-45,
    -40,
    -35,
    -30,
    -25,
    -20,
    -15,
	0
};

// dB to color mapping
const wxColour dBtoColor[] =
{
	wxColour(0,0,0),
	wxColour(64,64,64),
	wxColour(0,0,128),
	wxColour(0,0,255),
	wxColour(255,0,0),
	wxColour(0,255,0),
	wxColour(255,255,0),
	wxColour(255,255,255)
};

const wxColour& MapColor(float dB)
{
	if(dB < -45)      return dBtoColor[0];
    else if(dB < -40) return dBtoColor[1];
    else if(dB < -35) return dBtoColor[2];
    else if(dB < -30) return dBtoColor[3];
    else if(dB < -25) return dBtoColor[4];
    else if(dB < -20) return dBtoColor[5];
    else if(dB < -15) return dBtoColor[6];
    else              return dBtoColor[7];
}

const wxPen dBtoPen[] =
{
	wxPen(dBtoColor[0]),
	wxPen(dBtoColor[1]),
	wxPen(dBtoColor[2]),
	wxPen(dBtoColor[3]),
	wxPen(dBtoColor[4]),
	wxPen(dBtoColor[5]),
	wxPen(dBtoColor[6]),
	wxPen(dBtoColor[7])
};

const wxPen& MapPen(float dB)
{
	if(dB < -45)      return dBtoPen[0];
    else if(dB < -40) return dBtoPen[1];
    else if(dB < -35) return dBtoPen[2];
    else if(dB < -30) return dBtoPen[3];
    else if(dB < -25) return dBtoPen[4];
    else if(dB < -20) return dBtoPen[5];
    else if(dB < -15) return dBtoPen[6];
    else              return dBtoPen[7];
}

const wxPen& MapPen2(const float dB)
{
	if(dB < i2dB[0]) return dBtoPen[0];
	if(dB < i2dB[1]) return dBtoPen[1];
	if(dB < i2dB[2]) return dBtoPen[2];
	if(dB < i2dB[3]) return dBtoPen[3];
	if(dB < i2dB[4]) return dBtoPen[4];
	if(dB < i2dB[5]) return dBtoPen[5];
	if(dB < i2dB[6]) return dBtoPen[6];
	return dBtoPen[7];
}

AfhView::AfhView(wxWindow* pParentWnd): BaseView(pParentWnd), m_points(NULL)
{
}


AfhView::~AfhView()
{
	if(m_points) delete[] m_points;
}


/******************************************************************************
**  AfhView::Initialize
**  --------------------------------------------------------------------------
**  Copy data array into internal point array.
**  Correct array size should given in Initialize() method.
**  AFH_DB_SCALE constant sets dB resolution.
**
**  Parameters:
**              buff_len - DFT data lenght;
******************************************************************************/
bool AfhView::Init(int buff_len)
{
	m_length = m_psize = buff_len;

	m_sclPen.SetColour(0,128,0);
	m_sigPen.SetColour(0,255,0);

	/* adjust working rect to buffer size
	if (m_rect.height > buff_len) {
		m_rect.y += m_rect.height - buff_len;
		m_rect.height -= m_rect.y;
	}*/

	m_cursor.x = 0;
	m_cursor.y = m_rect.y + m_rect.height - 1;

	// delete old point array if it exists
	if(m_points) {
		delete[] m_points;
		m_points = NULL;
	}

	// allocate new point array
	m_points = new wxPoint[m_psize];

	if(!m_points)
		return false;

	// set window size
	SetMinSize(wxSize(100,256+2));
	//SetMaxSize(wxSize(300,2560+2));

	Clear();

	return true;
}

/******************************************************************************
**  AfhView::Copy
**  --------------------------------------------------------------------------
**  Copy data array into internal point array.
**  Correct array size should given in Initialize() method.
**  AFH_DB_SCALE constant sets dB resolution.
**
**  Parameters:
**              pBuffer - amplitude-frequency diagram data;
******************************************************************************/
void AfhView::Copy(float* dB)
{
	const int x0 = m_rect.x + m_rect.width;
	const float scale = float(-m_rect.width)/AFH_DB_SCALE;

	// to draw AFC form right-down to left-top
	const int y0 = m_rect.y + m_rect.height;
	const float d = float(m_rect.height)/(m_length-1);

	for(unsigned i = 0; i < m_length; i++) {
		m_points[i].x = x0 - int(dB[i]*scale+0.5f);
		m_points[i].y = y0 - int(i*d+0.5f);
	}
}

void AfhView::CopyInter(float* dB)
{
	const int width = m_rect.width;
	const float scale = float(-width)/AFH_DB_SCALE;

	// to draw AFC form right-down to left-top
	const int height = m_rect.height;
	const float d = float(height)/(m_length-1);
	unsigned i;

	m_points[0].x = width - int(dB[0]*scale);
	m_points[0].y = height;

	for(i = 1; i < m_length-1; i++) {
		float    y = i*d;
		float    iy = floor(0.5f+y); // round to the nearest interger
		unsigned n1 = (iy < y)? i-1: i;
		unsigned n2 = n1 + 1;
		float    l = (n1 < i)? (d-(y-iy)): (iy-y);
		float    db = dB[n1] + (dB[n2]-dB[n1])*l/d;

		m_points[i].x = width - int(db*scale);
		m_points[i].y = height - iy;
		//wxLogDebug(wxT("%d %d, %d"), i, m_points[i].x, m_points[i].y);
	}
	m_points[i].x = width - int(dB[i]*scale);
	m_points[i].y = height - i*d;
}

/******************************************************************************
**  AfhView::Draw
**  --------------------------------------------------------------------------
**  Main drawing method - copy data inside, clear view, plot signal,
**  draw cursor and send an event to the widgets to repaint the window.
**  Correct array size should given in Initialize() method.
**
**  Parameters:
**              pBuffer - amplitude-frequency diagram data;
******************************************************************************/
void AfhView::Draw(float* pBuffer)
{
	Clear();
	Copy(pBuffer);
	//CopyInter(pBuffer);

	SelectObject(&m_sigPen);
	// plot the diagram
	PolyLine(m_points, m_length);

	// draw the cursor
	SelectObject(wxWHITE_PEN);
	DrawLine(0, m_cursor.y, m_rect.width, m_cursor.y);

	Refresh(false);
}

void AfhView::Draw(wxPoint points[], unsigned length)
{
	Clear();

	SelectObject(&m_sigPen);
	// plot the diagram
	PolyLine(points, length);

	// draw the cursor
	SelectObject(wxWHITE_PEN);
	DrawLine(0, m_cursor.y, m_rect.width, m_cursor.y);

	Refresh(false);
}

void AfhView::DrawScale(void)
{
/*
	const int width = GetWidth();
	const float scale = float(width)/AFH_DB_SCALE;

	for(int i = 0, x = 0; i < sizeof(i2dB)/sizeof(i2dB[0]); i++) {
		int x2 = (AFH_DB_SCALE + i2dB[i]) * scale;
		FillRect(x, m_rect.y-eLevelScaleHeight, x2-x, eLevelScaleHeight, MapPen(i2dB[i]-1));
		x = x2;
	}
*/
}

/******************************************************************************
**  AfhView::Clear
**  --------------------------------------------------------------------------
**  Prepare view for signal plotting - clear the working rect and draw grates
**  (#-like picture).
**
**  Parameters: -
******************************************************************************/
void AfhView::Clear(void)
{
	BaseView::ClearRect(m_rect);

	SelectObject(&m_sclPen);

	// horizontal lines:
	// middle line
	MoveTo(m_rect.x, m_rect.y + m_rect.height/2);
	LineTo(m_rect.x + m_rect.width, m_rect.y + m_rect.height/2);
	// left line
	MoveTo(m_rect.x, m_rect.y + m_rect.height/4);
	LineTo(m_rect.x + m_rect.width, m_rect.y + m_rect.height/4);
	// right line
	MoveTo(m_rect.x, m_rect.y + m_rect.height*3/4);
	LineTo(m_rect.x + m_rect.width, m_rect.y + m_rect.height*3/4);

	// vertical lines:
	// middle line
	MoveTo(m_rect.width/2 + m_rect.x, m_rect.y);
	LineTo(m_rect.width/2 + m_rect.x, m_rect.y + m_rect.height);
	// upper line
	MoveTo(m_rect.width/4 + m_rect.x, m_rect.y);
	LineTo(m_rect.width/4 + m_rect.x, m_rect.y + m_rect.height);
	// lower line
	MoveTo(m_rect.width*3/4 + m_rect.x, m_rect.y);
	LineTo(m_rect.width*3/4 + m_rect.x, m_rect.y + m_rect.height);
}

void AfhView::SetCursor(int x, int y)
{
	m_cursor.x = m_rect.x + x;
	m_cursor.y = m_rect.y + y;
}

void AfhView::OnLButtonDown(wxMouseEvent& event)
{
	if (m_rect.Contains(event.GetPosition())) {
		// send event to the parent window
		event.ResumePropagation(1);
		event.Skip();
	}
}

void AfhView::OnSize(wxSizeEvent& event)
{
	BaseView::OnSize(event);

	m_rect = BaseView::GetDrawRect();

	Clear();
	Refresh(false);
}


/******************************************************************************
**  DBScaleView
**  --------------------------------------------------------------------------
**  DBScaleView class draws the DB scale window.
**
******************************************************************************/
void DBScaleView::Draw()
{
	const int width = GetWidth();
	const int height = GetHeight();
	const float scale = float(width)/AFH_DB_SCALE;

	for(int i = 0, x = 0; i < sizeof(i2dB)/sizeof(i2dB[0]); i++) {
		int x2 = (AFH_DB_SCALE + i2dB[i]) * scale;
		FillRect(x, 0, x2-x, height, MapPen(i2dB[i]-1));
		x = x2;
	}
}

void DBScaleView::OnSize(wxSizeEvent& event)
{
	BaseView::OnSize(event);

	Draw();
	Refresh(false);
}


/******************************************************************************
**  AmplitudeView
**  --------------------------------------------------------------------------
**  AmplitudeView class is derived from BaseView. It draws scaled
**  audio wave.
**
**  Parameters:
**      pParentWnd - parent window;
**
******************************************************************************/

const int AmplitudeView::Amplitude2Db[][2] =
{
	{ 36 , -3},
	{ 64 , -6},
	{ 83 , -9},
	{ 174, -9},
	{ 192, -6},
	{ 220, -3},
	{ 0  ,  0}
};

AmplitudeView::AmplitudeView(wxWindow* pParentWnd): BaseView(pParentWnd),
	m_cursor(0,0), m_Time(0)
{
}

bool AmplitudeView::Init(int sample_rate)
{
	m_sample_rate = sample_rate;

	// set window size
	SetMinSize(wxSize(400,50));
	//SetMaxSize(wxSize(-1,100));
	//DrawScale();

	return true;
}

void AmplitudeView::DrawScale()
{
	int width  = GetWidth();
	int height = GetHeight()-eTimeScaleHeight;
	int scale = 255*FACTOR/height;

	ClearRect(GetDrawRect());

	SelectObject(wxWHITE_PEN);
	// draw the zero line
	MoveTo(0, height/2);
	LineTo(width, height/2);

	SelectObject(wxGREY_PEN);
	SelectObject(wxGREY_BRUSH);
	// left scale
	FillRect(0,0, eLevelScaleWidth, height);
	// right margin
	FillRect(width-eEmptyScaleWidth, 0, width, height);
	// bottom scale
	FillRect(0, height, width, GetHeight());

	// draw left scale points
	SelectObject(wxBLACK_PEN);
	MoveTo( eLevelScaleWidth-ePitchWidth, height/2 );
	LineTo( eLevelScaleWidth, height/2 );

	// draw amplitude dB-s on the leftside scale
	wxString str(_T("   Db"));
	TextOut(0, 0, str);
	for(int i = 0; i < Amplitude2Db[i][0]; i++ )
	{
		str.Printf(_T("   %d"), Amplitude2Db[i][1] );
		TextOut(0, Amplitude2Db[i][0]*FACTOR/scale, str, Y);
		MoveTo(eLevelScaleWidth-ePitchWidth, Amplitude2Db[i][0]*FACTOR/scale );
		LineTo(eLevelScaleWidth, Amplitude2Db[i][0]*FACTOR/scale );
	}
}

/******************************************************************************
**  AmplitudeView::Draw
**  --------------------------------------------------------------------------
**  Main drawing method. It does not repaint the whole picture but scrolls
**  the picture in the direction given by 'forward' parameter. The input
**  sample array will be converted into two lines on screen (just two points
**  on X-axis).
**  This function does not make the window to be repainted.
**
**  Parameters:
**              pBuf    - samples;
**              size    - num of samples, usually 512 (FFT size);
**              step    - num of important samples in the center (8-512);
**              forward - time direction: forward/backward;
******************************************************************************/
void AmplitudeView::Draw(float pBuf[], int size, int step, bool forward)
{
	// cut the step in half, each half - 1 pixel on X-axis
	const int pixel = step/2;

	// if we move forward in time - scroll picture left
	DoScroll(forward? -2: 2);
	// and choose position at the right side, vice-versa if backward.
	const int pos = forward? GetWidth()-eEmptyScaleWidth-2 : eLevelScaleWidth;
	const int height = GetHeight()-eTimeScaleHeight;

	ClearRect(pos, 0, 2, height);
	SelectObject(wxWHITE_PEN);

	// parameters for the time scale drawing, p1 represent the time
	// interval for unnumbered scale points, p2 - time interval
	// for the scale points with a number.
	int p1, p2;

	// these numbers actually depend on the sample rate (8kHz)
	switch(pixel)
	{
		case 4:		p1 = 40,	p2 = 400;	break; // 0.5: 5 ,50 ms
		case 8:		p1 = 80,	p2 = 800;	break; // 1  : 10,100 ms
		case 16:	p1 = 160,	p2 = 1600;	break; // 2  : 20,200 ms
		case 32:	p1 = 400,	p2 = 4000;	break; // 4  : 50,500 ms
		case 64:	p1 = 800,	p2 = 8000;	break; // 8  :100,1000 ms
		case 128:	p1 = 1600,	p2 = 16000;	break; // 16 :200,2000 ms
		case 256:	p1 = 4000,	p2 = 32000;	break; // 32 :500,4000 ms
		default:	p1 = 1,		p2 = 1;		break;
	}

	// Drawing method: data block in center is divided by half,
	// every half is displayed by one line. Line length is calulated
	// by taking max and min amplitude values in the data block.
	pBuf += size/2 - pixel;
	for( int i = 0; i < 2; i++, pBuf += pixel )
	{
		float min = *std::min_element(pBuf, pBuf+pixel);
		float max = *std::max_element(pBuf, pBuf+pixel);
		int view_min = height*(1.0f - min)/2;
		int view_max = height*(1.0f - max)/2;

		// if they are equal the line will not be drawn so we correct this
		if (view_max == view_min) view_max += 1;

		// draw line from min amplitude to max
		DrawLine(pos+i, view_min, pos+i, view_max);

		// --------- the time scale drawing  ---------

		// time of the data block beginning
		const int time1 = (forward)? m_Time+i*pixel: m_Time - (GetWorkWidth()+2-i)*pixel;
		// time of the data block end
		const int time2 = time1 + pixel;

		// if during this block time it went across p1 boundary - draw scale point
		if( time1 == 0 || (time2/p1 > time1/p1) )
		{
			MoveTo(pos+i, height);
			if( time1 == 0 || (time2/p2 > time1/p2)) {
				LineTo(pos+i, height+(eTimeScaleHeight/2));
				wxString str;
				str.Printf(_T("%.2f"), float(time2/p2)*p2/m_sample_rate);
				TextOut(pos-10, height+(eTimeScaleHeight/3), str);
			} else {
				LineTo(pos+i, height+(eTimeScaleHeight/4));
			}
		}
	}
	// update left side time
	m_Time += forward? step : -step;
}

void AmplitudeView::DoScroll(int dx)
{
	wxRect rect = GetWorkRect();

	Scroll(dx, 0, rect);

	// scroll the time scale
	rect.x = (dx < 0)? (eLevelScaleWidth/2): (eLevelScaleWidth/4);
	rect.y = GetHeight() - eTimeScaleHeight;
	rect.width = GetWidth() - ((dx < 0)? eEmptyScaleWidth/4: eEmptyScaleWidth/2) - rect.x;
	rect.height = eTimeScaleHeight;
	// as BaseView::Scroll does not fill the emptied space we did
	// a small trick: we set a little wide rect to scroll so the scale
	// background color will fill this space.
	Scroll(dx, 0, rect);
}

void AmplitudeView::Clear(void)
{
	const int height = GetHeight()-eTimeScaleHeight;

	ClearRect(eLevelScaleWidth, 0, GetWorkWidth(), height);
	SelectObject(wxGREY_PEN);
	SelectObject(wxGREY_BRUSH);
	FillRect(0, height, GetWidth(), eTimeScaleHeight);
	// draw zero line
	SelectObject(wxWHITE_PEN);
	MoveTo(eLevelScaleWidth, height/2);
	LineTo(GetWidth()-eEmptyScaleWidth, height/2);
}

void AmplitudeView::OnPaint(wxPaintEvent& event)
{
	wxPaintDC DC(this);

	BaseView::OnPaint(event);
	// As this window content is not redrawn completely but
	// scrolled left/right we cannot draw the cursor by BaseView
	// methods. We add cursor after the whole picture is copied to screen -
	// the result is that cursor is (unfortunately) blinking.
	DC.SetPen(*wxWHITE_PEN);
	DC.DrawLine(m_cursor.x, 0, m_cursor.x, GetHeight()-eTimeScaleHeight);
}


void AmplitudeView::RePaint(void)
{
	wxClientDC DC(this);

	BaseView::RePaint();

	DC.SetPen(*wxWHITE_PEN);
	DC.DrawLine(m_cursor.x, 0, m_cursor.x, GetHeight()-eTimeScaleHeight);
}

void AmplitudeView::OnLButtonDown(wxMouseEvent& event)
{
	wxRect rect = GetWorkRect();

	if (rect.Contains(event.GetPosition())) {
		// send event to the parent window
		event.ResumePropagation(1);
		event.Skip();
	}
}

void AmplitudeView::OnSize(wxSizeEvent& event)
{
	BaseView::OnSize(event);

	m_rect = BaseView::GetDrawRect();

	m_rect.x      += eLevelScaleWidth;
	m_rect.width  -= eLevelScaleWidth + eEmptyScaleWidth;
	m_rect.height -= eTimeScaleHeight;

	//m_cursor.x = m_rect.x+m_rect.width-1;

	DrawScale();
	Refresh(false);
}

/******************************************************************************
**  SpectrumView
**  --------------------------------------------------------------------------
**  SpectrumView class is derived from BaseView. It draws 2D spectrogram.
**
**  Parameters:
**      pParentWnd - parent window;
**
******************************************************************************/
SpectrumView::SpectrumView(wxWindow* pParentWnd): BaseView(pParentWnd),
	m_strings(NULL),
	PITCH_WIDTH(5),	FREQ_SCALE_WIDTH(40),
	LEVL_SCALE_PITCH(8), LEVL_SCALE_WIDTH(50)
{
	m_num_pitch = LEVL_SCALE_PITCH;
	m_sample_rate = SAMPLE_RATE;
}

SpectrumView::~SpectrumView()
{
	if (m_strings) delete[] m_strings;
}


void SpectrumView::Init(int SampleRate, int points)
{
	m_sample_rate = SampleRate;
	m_length = points;

	if (!m_strings) {
		m_strings = new wxString[m_num_pitch];

		m_strings[0] = _T(" 50 dB");
		m_strings[1] = _T(" 45 dB");
		m_strings[2] = _T(" 40 dB");
		m_strings[3] = _T(" 35 dB");
		m_strings[4] = _T(" 30 dB");
		m_strings[5] = _T(" 25 dB");
		m_strings[6] = _T(" 20 dB");
		m_strings[7] = _T(" 15 dB");
	}

	// set window size
	SetMinSize(wxSize(400,256+2));
	//DrawScale(m_sample_rate, points);
}

void SpectrumView::Clear()
{
	BaseView::ClearRect(GetWorkRect());
}

void SpectrumView::Draw0(float *dB, int, bool forward)
{
	//const int width  = GetWidth();
	const int height = GetHeight();
	const int x = forward? GetWidth()-(LEVL_SCALE_WIDTH+2): FREQ_SCALE_WIDTH;

	DoScroll(forward? -2: 2);

	if(m_length == 256) {
		for(int i = 0,n = 0; i < height; i++, n += 2) {
			if(i < m_length/2) {
				const wxColour& color = MapColor(dB[i]);
				SetPixel(x,  height-n-1,color);
				SetPixel(x+1,height-n-1,color);
				SetPixel(x,  height-n-2,color);
				SetPixel(x+1,height-n-2,color);
			}
		}
	} else if(m_length == 512) {
		for(int i = 0; i < height; i++) {
			if(i < m_length/2) {
				const wxPen& pen = MapPen(dB[i]);
				SetPixel(x,  (height-1)-i, pen);
				SetPixel(x+1,(height-1)-i, pen);
			}
		}
	} else if(m_length == 1024) {
		for(int i = 0, n = 0; i < m_length/2; i+=2, n++) {
			if(i < m_length/2) {
				const wxColour& color = MapColor(dB[i]);
				SetPixel(x,  (height-1)-n,color);
				SetPixel(x+1,(height-1)-n,color);
			}
		}
	} else if(m_length == 2048) {
		for(int i = 0, n = 0; i < m_length/2; i+=3, n++) {
			if(i < m_length/2) {
				const wxColour& color = MapColor(dB[i]);
				SetPixel(x,  (height-1)-n,color);
				SetPixel(x+1,(height-1)-n,color);
			}
		}
	}
}

void SpectrumView::Draw(float *dB, int, bool forward)
{
	//const int width  = GetWidth();
	const int height = GetHeight();
	const int x = forward? GetWidth()-(LEVL_SCALE_WIDTH+2): FREQ_SCALE_WIDTH;

	DoScroll(forward? -2: 2);

	const float d = float(m_length)/(2*(height-1));

	for(int i = 0; i < height; i++) {
		unsigned n = i*d;
		const wxPen& pen = MapPen(dB[n]);
		int y = height-1 - i;

		SetPixel(x,   y, pen);
		SetPixel(x+1, y, pen);
	}
}

void SpectrumView::DrawScale(int sample_rate, int points)
{
	int width  = GetWidth();
	int height = GetHeight();
	int delta = (500*height*2)/sample_rate; // scale_step * height / (sample_rate/2)

	ClearRect(GetWorkRect());

	SelectObject(wxGREY_PEN);
	SelectObject(wxGREY_BRUSH);
	FillRect(0, 0, FREQ_SCALE_WIDTH, height);
	FillRect(width-LEVL_SCALE_WIDTH, 0, LEVL_SCALE_WIDTH, height);

	int w = LEVL_SCALE_WIDTH / 3;
	int h = height / m_num_pitch;
	int x = width - LEVL_SCALE_WIDTH;
	int y = height - h * m_num_pitch;

	// draw dB-color table at the right side
	for(int i = 0; i < m_num_pitch; i++, y += h) {
		TextOut(x+w, y, m_strings[i]);
		FillRect(x, y, w, h, dBtoColor[i]);
	}

	// draw frequency scale points
	SelectObject(wxBLACK_PEN);
	wxString str(_T("   Hz"));
	TextOut(PITCH_WIDTH, height-delta/2, str);

	for(int i = delta, n = 1; n < 9; i+=delta, n++)
	{
		str.Printf(_T("%d"), n*500);
		TextOut(PITCH_WIDTH, height-i, str);
		MoveTo(FREQ_SCALE_WIDTH-PITCH_WIDTH, height-i);
		LineTo(FREQ_SCALE_WIDTH, height-i);
	}

	Refresh(false);
}

void SpectrumView::DoScroll(int dx)
{
	wxRect rect;

	rect.x = FREQ_SCALE_WIDTH;
	rect.y = 0;
	rect.width = GetWidth()-LEVL_SCALE_WIDTH - rect.x;
	rect.height = GetHeight() - rect.y;

	Scroll(dx, 0, rect);
}

void SpectrumView::OnSize(wxSizeEvent& event)
{
	BaseView::OnSize(event);

	m_rect = BaseView::GetDrawRect();

	m_rect.x      += FREQ_SCALE_WIDTH;
	m_rect.width  -= FREQ_SCALE_WIDTH + LEVL_SCALE_WIDTH;

	DrawScale();
	Refresh(false);
}
