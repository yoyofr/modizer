#if !defined(AFX_NSFAPUPANEL_H__1CDA36BC_EB7A_44A9_A188_3F9CFD53A92C__INCLUDED_)
#define AFX_NSFAPUPANEL_H__1CDA36BC_EB7A_44A9_A188_3F9CFD53A92C__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000
// NSFApuPanel.h : �w�b�_�[ �t�@�C��
//
#include "NSFDialog.h"

/////////////////////////////////////////////////////////////////////////////
// NSFApuPanel �_�C�A���O

class NSFApuPanel : public CDialog, public NSFDialog
{
// �R���X�g���N�V����
public:
	NSFApuPanel(CWnd* pParent = NULL);   // �W���̃R���X�g���N�^

// �_�C�A���O �f�[�^
	//{{AFX_DATA(NSFApuPanel)
	enum { IDD = IDD_APUPANEL };
	BOOL	m_phase_refresh;
	BOOL	m_unmute_on_reset;
	BOOL	m_nonlinear_mixer;
	BOOL	m_duty_swap;
	BOOL	m_sweep_init;
	//}}AFX_DATA

public:
  void UpdateNSFPlayerConfig(bool b);

// �I�[�o�[���C�h
	// ClassWizard �͉��z�֐��̃I�[�o�[���C�h�𐶐����܂��B
	//{{AFX_VIRTUAL(NSFApuPanel)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV �T�|�[�g
	//}}AFX_VIRTUAL

// �C���v�������e�[�V����
protected:

	// �������ꂽ���b�Z�[�W �}�b�v�֐�
	//{{AFX_MSG(NSFApuPanel)
	afx_msg void OnPhaseRefresh();
	afx_msg void OnUnmuteOnReset();
	afx_msg void OnFreqLimit();
	afx_msg void OnNonlinearMixer();
	afx_msg void OnDutySwap();
	afx_msg void OnSweepInit();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()

public:
	virtual BOOL OnInitDialog();
};

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ �͑O�s�̒��O�ɒǉ��̐錾��}�����܂��B

#endif // !defined(AFX_NSFAPUPANEL_H__1CDA36BC_EB7A_44A9_A188_3F9CFD53A92C__INCLUDED_)
