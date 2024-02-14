#pragma once
#include "afxwin.h"


// CvcmCheck �_�C�A���O

class CvcmCheck : public CvcmCtrl
{
	DECLARE_DYNAMIC(CvcmCheck)

public:
  vcm::VT_CHECK *m_vt;
  CvcmCheck(const std::string &id, vcm::VT_CHECK *vt, CWnd* pParent=NULL );
	virtual ~CvcmCheck();

  void ReadWork();
  void WriteWork();

// �_�C�A���O �f�[�^
	enum { IDD = IDD_CHECK };

protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV �T�|�[�g

	DECLARE_MESSAGE_MAP()
public:
  virtual BOOL OnInitDialog();
  CButton m_check;
  afx_msg void OnBnClickedCheck();
};
