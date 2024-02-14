#pragma once

#include "NSFDialog.h"
#include "afxcmn.h"
#include "afxwin.h"

// NSFTrackSetupDialog �_�C�A���O

class NSFTrackSetupDialog : public CDialog
{
	DECLARE_DYNAMIC(NSFTrackSetupDialog)

public:
	NSFTrackSetupDialog(CWnd* pParent = NULL);   // �W���R���X�g���N�^
	virtual ~NSFTrackSetupDialog();

// �_�C�A���O �f�[�^
	enum { IDD = IDD_TRKINFO_SETUP };

protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV �T�|�[�g

	DECLARE_MESSAGE_MAP()
public:
  CSliderCtrl m_delay;
  CSliderCtrl m_freq;
  virtual BOOL OnInitDialog();
  afx_msg void OnShowWindow(BOOL bShow, UINT nStatus);
  afx_msg void OnHScroll(UINT nSBCode, UINT nPos, CScrollBar* pScrollBar);
  int m_freq_value;
  int m_delay_value;
  CStatic m_freq_text;
  CStatic m_delay_text;
  BOOL m_graphic_mode;
  BOOL m_freq_mode;
};
