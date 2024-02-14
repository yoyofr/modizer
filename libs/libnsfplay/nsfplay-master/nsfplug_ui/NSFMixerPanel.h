#if !defined(AFX_NSFMIXERPANEL_H__19434FC4_26B8_4CF5_ABEA_74EFFA4D82B5__INCLUDED_)
#define AFX_NSFMIXERPANEL_H__19434FC4_26B8_4CF5_ABEA_74EFFA4D82B5__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000
// NSFMixerPanel.h : �w�b�_�[ �t�@�C��
//
#include "NSFDialog.h"

/////////////////////////////////////////////////////////////////////////////
// NSFMixerPanel �_�C�A���O

class NSFMixerPanel : public CDialog, public NSFDialog
{
// �R���X�g���N�V����
public:
	NSFMixerPanel(CWnd* pParent = NULL);   // �W���̃R���X�g���N�^

  int device_id;
  void SetDeviceID(int id);
  void UpdateNSFPlayerConfig(bool b);
  void SetVolume(int i);
  int GetVolume();

// �_�C�A���O �f�[�^
	//{{AFX_DATA(NSFMixerPanel)
	enum { IDD = IDD_MIXER };
	CButton	m_title;
	CButton	m_mute;
	CSliderCtrl	m_slider;
	//}}AFX_DATA


// �I�[�o�[���C�h
	// ClassWizard �͉��z�֐��̃I�[�o�[���C�h�𐶐����܂��B
	//{{AFX_VIRTUAL(NSFMixerPanel)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV �T�|�[�g
	//}}AFX_VIRTUAL

// �C���v�������e�[�V����
protected:

	// �������ꂽ���b�Z�[�W �}�b�v�֐�
	//{{AFX_MSG(NSFMixerPanel)
	virtual BOOL OnInitDialog();
	afx_msg void OnVScroll(UINT nSBCode, UINT nPos, CScrollBar* pScrollBar);
	afx_msg void OnMute();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ �͑O�s�̒��O�ɒǉ��̐錾��}�����܂��B

#endif // !defined(AFX_NSFMIXERPANEL_H__19434FC4_26B8_4CF5_ABEA_74EFFA4D82B5__INCLUDED_)
