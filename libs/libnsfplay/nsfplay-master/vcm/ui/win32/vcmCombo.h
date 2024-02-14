#pragma once
#include "afxwin.h"


// CvcmCombo �_�C�A���O

class CvcmCombo : public CvcmCtrl
{
	DECLARE_DYNAMIC(CvcmCombo)

public:
  vcm::VT_COMBO *m_vt;
  CvcmCombo(const std::string &id, vcm::VT_COMBO *vt, CWnd* pParent = NULL);   // �W���R���X�g���N�^
	virtual ~CvcmCombo();
  void WriteWork();
  void ReadWork();

// �_�C�A���O �f�[�^
	enum { IDD = IDD_COMBO };

protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV �T�|�[�g

	DECLARE_MESSAGE_MAP()
public:
  virtual BOOL OnInitDialog();
  CStatic m_label;
  CComboBox m_combo;
  afx_msg void OnCbnSelchangeCombo();
};
