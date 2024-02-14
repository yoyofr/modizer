// NSFApuPanel.cpp : �C���v�������e�[�V���� �t�@�C��
//

#include "stdafx.h"
#include "NSFApuPanel.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// NSFApuPanel �_�C�A���O

using namespace xgm;

NSFApuPanel::NSFApuPanel(CWnd* pParent /*=NULL*/)
	: CDialog(NSFApuPanel::IDD, pParent)
{
	//{{AFX_DATA_INIT(NSFApuPanel)
	m_phase_refresh = TRUE;
	m_unmute_on_reset = TRUE;
	m_nonlinear_mixer = TRUE;
	m_duty_swap = FALSE;
	m_sweep_init = FALSE;
	//}}AFX_DATA_INIT
}


void NSFApuPanel::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(NSFApuPanel)
	DDX_Check(pDX, IDC_PHASE_REFRESH, m_phase_refresh);
	DDX_Check(pDX, IDC_UNMUTE_ON_RESET, m_unmute_on_reset);
	DDX_Check(pDX, IDC_NONLINEAR_MIXER, m_nonlinear_mixer);
	DDX_Check(pDX, IDC_DUTY_SWAP, m_duty_swap);
	DDX_Check(pDX, IDC_SWEEP_INIT, m_sweep_init);
	//}}AFX_DATA_MAP
}

void NSFApuPanel::UpdateNSFPlayerConfig(bool b)
{
  NSFDialog::UpdateNSFPlayerConfig(b);

  if(!m_hWnd) return ;

  if(b)
  {
    m_unmute_on_reset = pm->cf->GetDeviceOption(APU, NES_APU::OPT_UNMUTE_ON_RESET).GetInt()
                      | pm->cf->GetDeviceOption(DMC, NES_DMC::OPT_UNMUTE_ON_RESET).GetInt();
    m_phase_refresh   = pm->cf->GetDeviceOption(APU, NES_APU::OPT_PHASE_REFRESH).GetInt();
    m_nonlinear_mixer = pm->cf->GetDeviceOption(APU, NES_APU::OPT_NONLINEAR_MIXER).GetInt();
    m_duty_swap       = pm->cf->GetDeviceOption(APU, NES_APU::OPT_DUTY_SWAP).GetInt();
    m_sweep_init      = pm->cf->GetDeviceOption(APU, NES_APU::OPT_NEGATE_SWEEP_INIT).GetInt();
    UpdateData(FALSE);
  }
  else
  {
    UpdateData(TRUE);
    pm->cf->GetDeviceOption(APU, NES_APU::OPT_UNMUTE_ON_RESET  ) = m_unmute_on_reset;
    pm->cf->GetDeviceOption(DMC, NES_DMC::OPT_UNMUTE_ON_RESET  ) = m_unmute_on_reset;
    pm->cf->GetDeviceOption(APU, NES_APU::OPT_PHASE_REFRESH    ) = m_phase_refresh;
    pm->cf->GetDeviceOption(APU, NES_APU::OPT_NONLINEAR_MIXER  ) = m_nonlinear_mixer;
    pm->cf->GetDeviceOption(APU, NES_APU::OPT_DUTY_SWAP        ) = m_duty_swap;
    pm->cf->GetDeviceOption(APU, NES_APU::OPT_NEGATE_SWEEP_INIT) = m_sweep_init;
    pm->cf->Notify(APU);
  }
}

BEGIN_MESSAGE_MAP(NSFApuPanel, CDialog)
	//{{AFX_MSG_MAP(NSFApuPanel)
	ON_BN_CLICKED(IDC_PHASE_REFRESH, OnPhaseRefresh)
	ON_BN_CLICKED(IDC_UNMUTE_ON_RESET, OnUnmuteOnReset)
	ON_BN_CLICKED(IDC_NONLINEAR_MIXER, OnNonlinearMixer)
	ON_BN_CLICKED(IDC_DUTY_SWAP, OnDutySwap)
	ON_BN_CLICKED(IDC_SWEEP_INIT, OnSweepInit)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// NSFApuPanel ���b�Z�[�W �n���h��

void NSFApuPanel::OnPhaseRefresh() 
{
  //dynamic_cast<CPropertyPage*>(GetParent())->SetModified(true);	
}

void NSFApuPanel::OnUnmuteOnReset() 
{
  //dynamic_cast<CPropertyPage*>(GetParent())->SetModified(true);
}

void NSFApuPanel::OnFreqLimit() 
{
  //dynamic_cast<CPropertyPage*>(GetParent())->SetModified(true);
}

void NSFApuPanel::OnNonlinearMixer() 
{
  //dynamic_cast<CPropertyPage*>(GetParent())->SetModified(true);
}

void NSFApuPanel::OnDutySwap() 
{
  //dynamic_cast<CPropertyPage*>(GetParent())->SetModified(true);
}

void NSFApuPanel::OnSweepInit() 
{
  //dynamic_cast<CPropertyPage*>(GetParent())->SetModified(true);
}

BOOL NSFApuPanel::OnInitDialog()
{
  __super::OnInitDialog();

  // TODO :  �����ɏ�������ǉ����Ă�������
  UpdateNSFPlayerConfig(TRUE);

  return TRUE;  // return TRUE unless you set the focus to a control
  // ��O : OCX �v���p�e�B �y�[�W�͕K�� FALSE ��Ԃ��܂��B
}
