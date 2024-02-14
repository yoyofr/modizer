// NSFMixerDialog.cpp : �C���v�������e�[�V���� �t�@�C��
//

#include "stdafx.h"
#include "NSFDialogs.h"
#include "NSFMixerDialog.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

using namespace xgm;

/////////////////////////////////////////////////////////////////////////////
// NSFMixerDialog �_�C�A���O

NSFMixerDialog::NSFMixerDialog(CWnd* pParent /*=NULL*/)
	: CDialog(NSFMixerDialog::IDD, pParent)
{
	//{{AFX_DATA_INIT(NSFMixerDialog)
		// ���� - ClassWizard �͂��̈ʒu�Ƀ}�b�s���O�p�̃}�N����ǉ��܂��͍폜���܂��B
	//}}AFX_DATA_INIT
}

void NSFMixerDialog::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(NSFMixerDialog)
		// ���� - ClassWizard �͂��̈ʒu�Ƀ}�b�s���O�p�̃}�N����ǉ��܂��͍폜���܂��B
	//}}AFX_DATA_MAP
}

void NSFMixerDialog::SetDialogManager(NSFDialogManager *p)
{
  for(int i=0;i<NES_DEVICE_MAX;i++)
  {
    panel[i].SetDialogManager(p);
    panel[i].SetDeviceID(i);
  }
  master_panel.SetDialogManager(p);
  master_panel.SetDeviceID(-1);
  NSFDialog::SetDialogManager(p);
}

void NSFMixerDialog::UpdateNSFPlayerConfig(bool b)
{
  NSFDialog::UpdateNSFPlayerConfig(b);

  for(int i=0;i<NES_DEVICE_MAX;i++)
    panel[i].UpdateNSFPlayerConfig(b);
  master_panel.UpdateNSFPlayerConfig(b);
}

BEGIN_MESSAGE_MAP(NSFMixerDialog, CDialog)
	//{{AFX_MSG_MAP(NSFMixerDialog)
	ON_COMMAND(IDM_RESET, OnReset)
	//}}AFX_MSG_MAP
  ON_COMMAND(ID_UPALL, OnUpall)
  ON_COMMAND(ID_DOWNALL, OnDownall)
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// NSFMixerDialog ���b�Z�[�W �n���h��

BOOL NSFMixerDialog::OnInitDialog() 
{
	CDialog::OnInitDialog();

  HICON hIcon = AfxGetApp()->LoadIcon(IDI_MIXER);
  SetIcon(hIcon, TRUE);
  
  RECT rect;
  int x = 0;

  master_panel.Create(IDD_MIXER,this);
  master_panel.GetClientRect(&rect);
  master_panel.SetWindowPos(NULL,x,0,rect.right,rect.bottom,SWP_NOZORDER|SWP_SHOWWINDOW);

  for(int i=0;i<NES_DEVICE_MAX; i++)
  {
    x += rect.right;
    panel[i].Create(IDD_MIXER,this);
    panel[i].GetClientRect(&rect);
    panel[i].SetWindowPos(NULL,x,0,rect.right,rect.bottom,SWP_NOZORDER|SWP_SHOWWINDOW);
  }
  x += rect.right;

  int y = rect.bottom;
  GetWindowRect(&rect);
  int th = rect.bottom - rect.top;
  GetClientRect(&rect);
  th -= rect.bottom;
  SetWindowPos(NULL,0,0,x+6,y+th, SWP_NOMOVE|SWP_NOZORDER);

	return TRUE;  // �R���g���[���Ƀt�H�[�J�X��ݒ肵�Ȃ��Ƃ��A�߂�l�� TRUE �ƂȂ�܂�
	              // ��O: OCX �v���p�e�B �y�[�W�̖߂�l�� FALSE �ƂȂ�܂�
}

void NSFMixerDialog::OnReset() 
{
  for(int i=0;i<NES_DEVICE_MAX; i++)
    panel[i].SetVolume(128);
  master_panel.SetVolume(128);
}

void NSFMixerDialog::OnUpall() 
{
  for(int i=0;i<NES_DEVICE_MAX; i++)
    panel[i].SetVolume(panel[i].GetVolume()+8);
}

void NSFMixerDialog::OnDownall() 
{
  for(int i=0;i<NES_DEVICE_MAX; i++)
    panel[i].SetVolume(panel[i].GetVolume()-8);
}
