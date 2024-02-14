#if !defined(AFX_NSFCONFIGPAGEDEVICE_H__85114CA7_F58B_4317_A254_A70516EDB80C__INCLUDED_)
#define AFX_NSFCONFIGPAGEDEVICE_H__85114CA7_F58B_4317_A254_A70516EDB80C__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000
// NSFConfigPageDevice.h : �w�b�_�[ �t�@�C��
//
#include "NSFDialog.h"
//#include "NSFAudioPanel.h"

/////////////////////////////////////////////////////////////////////////////
// NSFConfigPageDevice �_�C�A���O

class NSFConfigPageDevice : public CPropertyPage, public NSFDialog
{
	DECLARE_DYNCREATE(NSFConfigPageDevice)

protected:

// �R���X�g���N�V����
public:
	NSFConfigPageDevice();
  ~NSFConfigPageDevice();

// �_�C�A���O �f�[�^
	//{{AFX_DATA(NSFConfigPageDevice)
	enum { IDD = IDD_DEVICE };
		// ����: ClassWizard �͂��̈ʒu�Ƀf�[�^ �����o��ǉ����܂��B
	//}}AFX_DATA

// ���[�U�[�ǉ�
public:
  int device_id;
  //NSFAudioPanel *audioPanel;
  NSFDialog *mainPanel;
  int mainPanel_id;
  void SetPanel(NSFDialog *p, int id);
  void SetDeviceID(int id);
  void UpdateNSFPlayerConfig(bool b);
  void SetDialogManager(NSFDialogManager *p);

// �I�[�o�[���C�h
	// ClassWizard �͉��z�֐��̃I�[�o�[���C�h�𐶐����܂��B
	//{{AFX_VIRTUAL(NSFConfigPageDevice)
	public:
	virtual BOOL OnApply();
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV �T�|�[�g
	//}}AFX_VIRTUAL

// �C���v�������e�[�V����
protected:

	// �������ꂽ���b�Z�[�W �}�b�v�֐�
	//{{AFX_MSG(NSFConfigPageDevice)
	virtual BOOL OnInitDialog();
	afx_msg int OnCreate(LPCREATESTRUCT lpCreateStruct);
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ �͑O�s�̒��O�ɒǉ��̐錾��}�����܂��B

#endif // !defined(AFX_NSFCONFIGPAGEDEVICE_H__85114CA7_F58B_4317_A254_A70516EDB80C__INCLUDED_)
