#pragma once

#include "KeyWindow.h"
#include "KeyHeader.h"

// KeyDialog �_�C�A���O

class KeyDialog : public CDialog
{
	DECLARE_DYNAMIC(KeyDialog)

public:
	KeyDialog(CWnd* pParent = NULL);   // �W���R���X�g���N�^
	virtual ~KeyDialog();
  
  // �`��J�n
  virtual void Start(int);
  // �`���~
  virtual void Stop();

  virtual void Reset();

  inline int MinWidth(){ return m_keywindow.MinWidth(); }
  inline int MaxWidth(){ return m_keywindow.MaxWidth(); }

  // ���Օ���
  KeyWindow m_keywindow;

  // �w�b�_����
  KeyHeader m_keyheader;

  bool m_bInit;
  UINT_PTR m_nTimer;

// �_�C�A���O �f�[�^
	enum { IDD = IDD_KEYDIALOG };

protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV �T�|�[�g

	DECLARE_MESSAGE_MAP()
public:
  afx_msg void OnPaint();
  virtual BOOL OnInitDialog();
  afx_msg void OnSize(UINT nType, int cx, int cy);
  afx_msg void OnTimer(UINT nIDEvent);
};
