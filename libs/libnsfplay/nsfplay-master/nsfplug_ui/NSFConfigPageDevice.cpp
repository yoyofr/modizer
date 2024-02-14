// NSFConfigPageDevice.cpp : �C���v�������e�[�V���� �t�@�C��
//

#include "stdafx.h"
#include "NSFConfigPageDevice.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

using namespace xgm;

/////////////////////////////////////////////////////////////////////////////
// NSFConfigPageDevice �_�C�A���O


IMPLEMENT_DYNCREATE(NSFConfigPageDevice, CPropertyPage)

NSFConfigPageDevice::NSFConfigPageDevice() : CPropertyPage(NSFConfigPageDevice::IDD)
{
	//{{AFX_DATA_INIT(NSFConfigPageDevice)
		// ���� - ClassWizard �͂��̈ʒu�Ƀ}�b�s���O�p�̃}�N����ǉ��܂��͍폜���܂��B
	//}}AFX_DATA_INIT
  //audioPanel = new NSFAudioPanel(this);
  mainPanel = NULL;
}

NSFConfigPageDevice::~NSFConfigPageDevice()
{
  //delete audioPanel;
}

void NSFConfigPageDevice::DoDataExchange(CDataExchange* pDX)
{
	CPropertyPage::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(NSFConfigPageDevice)
		// ���� - ClassWizard �͂��̈ʒu�Ƀ}�b�s���O�p�̃}�N����ǉ��܂��͍폜���܂��B
	//}}AFX_DATA_MAP
}

void NSFConfigPageDevice::SetDeviceID(int id)
{
  device_id = id;
  //audioPanel->SetDeviceID(device_id);
  m_psp.dwFlags |= PSP_USETITLE;
  m_psp.pszTitle = NSFPlayerConfig::dname[device_id];
}

void NSFConfigPageDevice::SetPanel(NSFDialog *p, int id)
{
  mainPanel = p;
  mainPanel_id = id;
}

void NSFConfigPageDevice::SetDialogManager(NSFDialogManager *p)
{
  NSFDialog::SetDialogManager(p);
  //audioPanel->SetDialogManager(p);
  if(mainPanel)
    mainPanel->SetDialogManager(p);
}

void NSFConfigPageDevice::UpdateNSFPlayerConfig(bool b)
{
  NSFDialog::UpdateNSFPlayerConfig(b);

  //audioPanel->UpdateNSFPlayerConfig(b);
  if(mainPanel)
    mainPanel->UpdateNSFPlayerConfig(b);
}

BEGIN_MESSAGE_MAP(NSFConfigPageDevice, CPropertyPage)
	//{{AFX_MSG_MAP(NSFConfigPageDevice)
	ON_WM_CREATE()
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// NSFConfigPageDevice ���b�Z�[�W �n���h��

BOOL NSFConfigPageDevice::OnInitDialog() 
{
	CPropertyPage::OnInitDialog();

  int x = 16, y = 56;
  //RECT rect1;
  //audioPanel->Create(IDD_AUDIOPANEL, this);
  //dynamic_cast<CDialog *>(audioPanel)->GetClientRect(&rect1);
  //int cx = rect1.right, cy = rect1.bottom;
  //audioPanel->SetWindowPos(0,x,y,0,0,SWP_NOSIZE|SWP_NOZORDER);
  //audioPanel->ShowWindow(SW_SHOW);
  int cx = x, cy = y;

  if(mainPanel)
  {
    RECT rect2;

    dynamic_cast<CDialog *>(mainPanel)->Create(mainPanel_id, this);
    dynamic_cast<CDialog *>(mainPanel)->GetClientRect(&rect2);
    dynamic_cast<CDialog *>(mainPanel)->SetWindowPos(0,x,y+cy,0,0,SWP_NOSIZE|SWP_NOZORDER);
    dynamic_cast<CDialog *>(mainPanel)->ShowWindow(SW_SHOW);
    cy += rect2.bottom;
    cx = cx<rect2.right?rect2.right:cx;
  }

  SetWindowPos(0,0,0,cx+x,cy+y,SWP_NOMOVE|SWP_NOZORDER);

  UpdateNSFPlayerConfig(TRUE);

	return TRUE;  // �R���g���[���Ƀt�H�[�J�X��ݒ肵�Ȃ��Ƃ��A�߂�l�� TRUE �ƂȂ�܂�
	              // ��O: OCX �v���p�e�B �y�[�W�̖߂�l�� FALSE �ƂȂ�܂�
}


int NSFConfigPageDevice::OnCreate(LPCREATESTRUCT lpCreateStruct) 
{
	if (CPropertyPage::OnCreate(lpCreateStruct) == -1)
		return -1;
	
	return 0;
}

BOOL NSFConfigPageDevice::OnApply() 
{
	UpdateNSFPlayerConfig(FALSE);
	return CPropertyPage::OnApply();
}
