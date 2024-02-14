#pragma once
#include "afxcmn.h"


// CvcmTree �_�C�A���O

class CvcmTree : public CDialog
{
	DECLARE_DYNAMIC(CvcmTree)

public:
  vcm::Configuration *m_pConfig;
  std::list<vcm::ConfigGroup *> m_pGroups;

  CvcmTree( CWnd* pParent = NULL );   // �W���R���X�g���N�^
	virtual ~CvcmTree();

  void AttachConfig( vcm::Configuration & );
  void AttachGroup( vcm::ConfigGroup & );
  void UpdateConfig( bool b );

// �_�C�A���O �f�[�^
	enum { IDD = IDD_TREE_DIALOG };

protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV �T�|�[�g
  void AdjustSize( int cx, int cy );
  void CreateItem( HTREEITEM, vcm::ConfigGroup * );

	DECLARE_MESSAGE_MAP()
public:
  CTreeCtrl m_tree;
  CvcmDlg m_dlg;
  CPoint m_dlgPoint;
  virtual BOOL OnInitDialog();
  afx_msg void OnDestroy();
  afx_msg void OnTvnSelchangedTree(NMHDR *pNMHDR, LRESULT *pResult);
  afx_msg void OnSize(UINT nType, int cx, int cy);
};
