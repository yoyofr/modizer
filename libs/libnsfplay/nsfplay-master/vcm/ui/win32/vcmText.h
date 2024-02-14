#pragma once
#include "afxwin.h"

// CvcmText �_�C�A���O

class CvcmText : public CvcmCtrl
{
	DECLARE_DYNAMIC(CvcmText)

public:
  vcm::VT_TEXT *m_vt;
  CvcmText( const std::string &id, vcm::VT_TEXT *vt, CWnd* pParent=NULL);
	virtual ~CvcmText();
  void ReadWork();
  void WriteWork();

// �_�C�A���O �f�[�^
	enum { IDD = IDD_TEXT };

protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV �T�|�[�g

	DECLARE_MESSAGE_MAP()
public:
  virtual BOOL OnInitDialog();
  afx_msg void OnEnKillfocusText();
  CStatic m_label;
  CEdit m_edit;
};
