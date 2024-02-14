// NSFN106Panel.cpp
//

#include "stdafx.h"
#include "NSFN106Panel.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

using namespace xgm;

/////////////////////////////////////////////////////////////////////////////
// NSFN106Panel


NSFN106Panel::NSFN106Panel(CWnd* pParent /*=NULL*/)
	: CDialog(NSFN106Panel::IDD, pParent)
{
	//{{AFX_DATA_INIT(NSFN106Panel)
	m_serial = FALSE;
	m_phase_read_only = FALSE;
	m_limit_wavelength = FALSE;
	//}}AFX_DATA_INIT
}


void NSFN106Panel::UpdateNSFPlayerConfig(bool b)
{
  NSFDialog::UpdateNSFPlayerConfig(b);

  if(!m_hWnd) return ;

  if(b)
  {
    m_serial = pm->cf->GetDeviceOption(N106,NES_N106::OPT_SERIAL).GetInt();
    m_phase_read_only = pm->cf->GetDeviceOption(N106,NES_N106::OPT_PHASE_READ_ONLY).GetInt();
    m_limit_wavelength = pm->cf->GetDeviceOption(N106,NES_N106::OPT_LIMIT_WAVELENGTH).GetInt();
    UpdateData(FALSE);
  }
  else
  {
    UpdateData(TRUE);
	pm->cf->GetDeviceOption(N106, NES_N106::OPT_SERIAL) = m_serial;
	pm->cf->GetDeviceOption(N106, NES_N106::OPT_PHASE_READ_ONLY) = m_phase_read_only;
	pm->cf->GetDeviceOption(N106, NES_N106::OPT_LIMIT_WAVELENGTH) = m_limit_wavelength;
    pm->cf->Notify(N106);
  }
}

void NSFN106Panel::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(NSFN106Panel)
	DDX_Check(pDX, IDC_SERIAL, m_serial);
	DDX_Check(pDX, IDC_PHASE_READ_ONLY, m_phase_read_only);
	DDX_Check(pDX, IDC_LIMIT_WAVELENGTH, m_limit_wavelength);
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(NSFN106Panel, CDialog)
	//{{AFX_MSG_MAP(NSFN106Panel)
	ON_BN_CLICKED(IDC_SERIAL, OnSerial)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// NSFN106Panel

void NSFN106Panel::OnSerial()
{
	//SetModified(true);
}

BOOL NSFN106Panel::OnInitDialog()
{
  __super::OnInitDialog();

  UpdateNSFPlayerConfig(TRUE);

  return TRUE;  // return TRUE unless you set the focus to a control
}
